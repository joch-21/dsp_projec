#include "Map.h"
#include <algorithm>
#include <cstdlib>
#include <queue>
#include <random>
#include <sstream>

// Helper TreeVisitor to format the BTree<TechNode> hierarchy into a string
class TechTreePrintVisitor : public TreeVisitor<TechNode>
{
private:
    mutable std::string fOutput;

public:
    TechTreePrintVisitor() : fOutput("") {}

    void preVisit(const TechNode& aKey) const override
    {
        std::ostringstream oss;
        oss << " - " << aKey.name << " ["
            << (aKey.unlocked ? "UNLOCKED" : "LOCKED - Cost: " + std::to_string(aKey.unlockCost) + " Copper")
            << "]\n";
        fOutput += oss.str();
    }

    const std::string& getOutput() const { return fOutput; }
};

Map::Map() : fTechRoot(nullptr),
    fCoreHP(MapConfig::CoreHitPoints), fMaxCoreHP(MapConfig::CoreHitPoints)
{
    initTiles();
    initTechTree();
}

Map::~Map()
{
    clearBuildings();
    if (fTechRoot != nullptr)
    {
        delete fTechRoot;
        fTechRoot = nullptr;
    }
}

void Map::initTiles()
{
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            fTiles[y][x] = '.';
        }
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> coreX(1, WIDTH - 2);
    std::uniform_int_distribution<int> coreY(1, HEIGHT - 2);
    fCorePosition = { coreX(rng), coreY(rng) };

    std::vector<Position> edgeTiles;
    int farthestDistance = -1;
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            if (x != 0 && x != WIDTH - 1 && y != 0 && y != HEIGHT - 1) continue;
            const int distance = std::abs(x - fCorePosition.x) + std::abs(y - fCorePosition.y);
            if (distance > farthestDistance)
            {
                farthestDistance = distance;
                edgeTiles.clear();
            }
            if (distance == farthestDistance) edgeTiles.push_back({ x, y });
        }
    }
    std::uniform_int_distribution<size_t> spawnChoice(0, edgeTiles.size() - 1);
    fEnemySpawnPosition = edgeTiles[spawnChoice(rng)];
    fTiles[fCorePosition.y][fCorePosition.x] = '#';
    fTiles[fEnemySpawnPosition.y][fEnemySpawnPosition.x] = 'S';

    std::vector<Position> resourceTiles;
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            if (fTiles[y][x] == '.') resourceTiles.push_back({ x, y });
    std::shuffle(resourceTiles.begin(), resourceTiles.end(), rng);

    size_t next = 0;
    for (int i = 0; i < MapConfig::CopperDepositCount; ++i)
    {
        Position pos = resourceTiles[next++];
        fTiles[pos.y][pos.x] = 'C';
    }
    for (int i = 0; i < MapConfig::CoalDepositCount; ++i)
    {
        Position pos = resourceTiles[next++];
        fTiles[pos.y][pos.x] = 'K';
    }
}

void Map::initTechTree()
{
    if (fTechRoot != nullptr)
    {
        delete fTechRoot;
    }

    // Build Mindustry-style Binary Tech Tree:
    // Root: Core -> Left: Drill -> Left: Generator | Right: Turret
    fTechRoot = new BTree<TechNode>(TechNode("Core", 0, true));
    fTechRoot->attachLeft(new BTree<TechNode>(TechNode("Drill", 0, true)));
    fTechRoot->left().attachLeft(new BTree<TechNode>(TechNode("Generator",
        MapConfig::GeneratorUnlockCost, false)));
    fTechRoot->attachRight(new BTree<TechNode>(TechNode("Turret",
        MapConfig::TurretUnlockCost, false)));
}

void Map::clearBuildings()
{
    for (auto it = fBuildings.getIteratorFront(); it != it.end(); ++it)
    {
        delete* it;
    }
    fBuildings.clear();
}

void Map::reset()
{
    clearBuildings();
    fCoreHP = fMaxCoreHP;
    initTiles();
    initTechTree();
}

bool Map::isInside(Position aPos) const
{
    return aPos.x >= 0 && aPos.x < WIDTH && aPos.y >= 0 && aPos.y < HEIGHT;
}

char Map::getTile(Position aPos) const
{
    if (!isInside(aPos))
    {
        return '?';
    }
    return fTiles[aPos.y][aPos.x];
}

Position Map::getCorePosition() const { return fCorePosition; }
Position Map::getEnemySpawnPosition() const { return fEnemySpawnPosition; }

std::vector<std::string> Map::getResourceLayout() const
{
    std::vector<std::string> rows(HEIGHT, std::string(WIDTH, '.'));
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            if (fTiles[y][x] == 'C' || fTiles[y][x] == 'K') rows[y][x] = fTiles[y][x];
    return rows;
}

bool Map::restoreLayout(Position aCore, Position aSpawn, const std::vector<std::string>& aRows)
{
    if (!isInside(aCore) || !isInside(aSpawn) ||
        (aCore.x == aSpawn.x && aCore.y == aSpawn.y) ||
        (aSpawn.x != 0 && aSpawn.x != WIDTH - 1 && aSpawn.y != 0 && aSpawn.y != HEIGHT - 1) ||
        aRows.size() != HEIGHT)
        return false;

    for (const std::string& row : aRows)
    {
        if (row.size() != WIDTH) return false;
        for (char tile : row)
            if (tile != '.' && tile != 'C' && tile != 'K') return false;
    }
    if (aRows[aCore.y][aCore.x] != '.' || aRows[aSpawn.y][aSpawn.x] != '.') return false;

    clearBuildings();
    fCorePosition = aCore;
    fEnemySpawnPosition = aSpawn;
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            fTiles[y][x] = aRows[y][x];
    fTiles[aCore.y][aCore.x] = '#';
    fTiles[aSpawn.y][aSpawn.x] = 'S';
    fCoreHP = fMaxCoreHP;
    return true;
}

Building* Map::getBuildingAt(Position aPos) const
{
    for (auto it = fBuildings.getIteratorFront(); it != it.end(); ++it)
    {
        Building* b = *it;
        if (b->getPos().x == aPos.x && b->getPos().y == aPos.y)
        {
            return b;
        }
    }
    return nullptr;
}

std::vector<Position> Map::findPath(Position aStart, Position aGoal, bool aAllowBuildings) const
{
    if (!isInside(aStart) || !isInside(aGoal)) return {};

    bool visited[HEIGHT][WIDTH]{};
    Position parent[HEIGHT][WIDTH]{};
    std::queue<Position> open;
    open.push(aStart);
    visited[aStart.y][aStart.x] = true;

    constexpr int dx[] = { 1, -1, 0, 0 };
    constexpr int dy[] = { 0, 0, 1, -1 };
    while (!open.empty())
    {
        Position current = open.front();
        open.pop();
        if (current.x == aGoal.x && current.y == aGoal.y) break;

        for (int direction = 0; direction < 4; ++direction)
        {
            Position next{ current.x + dx[direction], current.y + dy[direction] };
            if (!isInside(next) || visited[next.y][next.x]) continue;
            if (next.x == aGoal.x && next.y == aGoal.y)
            {
                // The Core is a valid destination, even though it is not walkable.
            }
            else
            {
                char tile = getTile(next);
                if (tile == '#') continue;
                if (!aAllowBuildings && getBuildingAt(next) != nullptr) continue;
            }

            visited[next.y][next.x] = true;
            parent[next.y][next.x] = current;
            open.push(next);
        }
    }

    if (!visited[aGoal.y][aGoal.x]) return {};
    std::vector<Position> path;
    Position current = aGoal;
    path.push_back(current);
    while (current.x != aStart.x || current.y != aStart.y)
    {
        current = parent[current.y][current.x];
        path.push_back(current);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

bool Map::canPlaceBuilding(Position aPos, const std::string& aType, std::string& outError) const
{
    if (!isInside(aPos))
    {
        outError = "Coordinates are outside the map!";
        return false;
    }

    if (!isTechUnlocked(aType))
    {
        outError = aType + " is still locked in the Tech Tree!";
        return false;
    }

    char tile = getTile(aPos);
    if (tile == '#')
    {
        outError = "Cannot build on top of the Core!";
        return false;
    }

    if (tile == 'S')
    {
        outError = "This tile is reserved for enemy spawning.";
        return false;
    }

    if (getBuildingAt(aPos) != nullptr)
    {
        outError = "A building already exists at that position!";
        return false;
    }

    if (aType == "Drill" && tile != 'C' && tile != 'K')
    {
        outError = "Drills must be placed on Copper ('C') or Coal ('K') ore tiles!";
        return false;
    }

    return true;
}

void Map::addBuilding(Building* aBuilding)
{
    if (aBuilding != nullptr)
    {
        fBuildings.pushBack(aBuilding);
    }
}

bool Map::removeBuildingAt(Position aPos)
{
    int idx = 0;
    for (auto it = fBuildings.getIteratorFront(); it != it.end(); ++it, ++idx)
    {
        Building* b = *it;
        if (b->getPos().x == aPos.x && b->getPos().y == aPos.y)
        {
            fBuildings.removeAt(idx);
            delete b;
            return true;
        }
    }
    return false;
}

const List<Building*>& Map::getBuildings() const
{
    return fBuildings;
}

int Map::getCoreHP() const { return fCoreHP; }
int Map::getMaxCoreHP() const { return fMaxCoreHP; }

void Map::setCoreHP(int aHP)
{
    fCoreHP = aHP;
    if (fCoreHP < 0) fCoreHP = 0;
    if (fCoreHP > fMaxCoreHP) fCoreHP = fMaxCoreHP;
}

void Map::damageCore(int aAmount)
{
    if (aAmount > 0)
    {
        fCoreHP -= aAmount;
        if (fCoreHP < 0)
        {
            fCoreHP = 0;
        }
    }
}

bool Map::isCoreDestroyed() const
{
    return fCoreHP <= 0;
}

BTree<TechNode>* Map::findTechNode(BTree<TechNode>* aNode, const std::string& aName) const
{
    if (aNode == nullptr || aNode->isEmpty())
    {
        return nullptr;
    }
    if (aNode->key().name == aName)
    {
        return aNode;
    }
    BTree<TechNode>* leftResult = findTechNode(&aNode->left(), aName);
    if (leftResult != nullptr)
    {
        return leftResult;
    }
    return findTechNode(&aNode->right(), aName);
}

bool Map::isTechUnlocked(const std::string& aTechName) const
{
    BTree<TechNode>* node = findTechNode(fTechRoot, aTechName);
    if (node == nullptr)
    {
        return false;
    }
    return node->key().unlocked;
}

void Map::setTechUnlocked(const std::string& aTechName, bool aUnlocked)
{
    BTree<TechNode>* node = findTechNode(fTechRoot, aTechName);
    if (node != nullptr)
    {
        node->keyMutable().unlocked = aUnlocked;
    }
}

bool Map::unlockTech(const std::string& aTechName, ResourceStore& aResources, std::string& outMessage)
{
    BTree<TechNode>* node = findTechNode(fTechRoot, aTechName);
    if (node == nullptr)
    {
        outMessage = "Unknown technology: " + aTechName;
        return false;
    }
    if (node->key().unlocked)
    {
        outMessage = aTechName + " is already unlocked.";
        return false;
    }

    // Parent prerequisite check: Generator requires Drill
    if (aTechName == "Generator" && !isTechUnlocked("Drill"))
    {
        outMessage = "You must unlock Drill before Generator!";
        return false;
    }

    int cost = node->key().unlockCost;
    if (!aResources.spend("Copper", cost))
    {
        outMessage = "Not enough Copper! Need " + std::to_string(cost) + " Copper.";
        return false;
    }

    node->keyMutable().unlocked = true;
    outMessage = "Unlocked " + aTechName + " for " + std::to_string(cost) + " Copper!";
    return true;
}

std::string Map::getTechTreeDisplay() const
{
    TechTreePrintVisitor visitor;
    if (fTechRoot != nullptr)
    {
        fTechRoot->traverseDepthFirst(visitor);
    }
    return visitor.getOutput();
}

std::string Map::getGridDisplay() const
{
    std::ostringstream oss;
    oss << "   ";
    for (int x = 0; x < WIDTH; ++x)
    {
        oss << x << " ";
    }
    oss << "\n";

    for (int y = 0; y < HEIGHT; ++y)
    {
        oss << y << "  ";
        for (int x = 0; x < WIDTH; ++x)
        {
            Building* b = getBuildingAt(Position{ x, y });
            if (b != nullptr)
            {
                oss << b->getSymbol() << " ";
            }
            else
            {
                oss << fTiles[y][x] << " ";
            }
        }
        oss << "\n";
    }
    return oss.str();
}
