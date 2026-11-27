#pragma once
#include <string>
#include <vector>
#include "List.h"
#include "BTree.h"
#include "Building.h"
#include "ResourceStore.h"

// Map generation, core, and technology values are configured here.
namespace MapConfig
{
    inline constexpr int Width = 10;
    inline constexpr int Height = 8;
    inline constexpr int CoreHitPoints = 100;
    inline constexpr int CopperDepositCount = 8;
    inline constexpr int CoalDepositCount = 6;
    inline constexpr int GeneratorUnlockCost = 20;
    inline constexpr int TurretUnlockCost = 25;
}

// Represents Mindustry-style technology unlocks:
//   Core (Root) -> Left: Drill -> Left: Generator | Right: Turret
struct TechNode
{
    std::string name;
    int unlockCost;
    bool unlocked;

    TechNode() : name(""), unlockCost(0), unlocked(false) {}
    TechNode(const std::string& aName, int aCost, bool aUnlocked)
        : name(aName), unlockCost(aCost), unlocked(aUnlocked)
    {
    }
};

class Map
{
public:
    static constexpr int WIDTH = MapConfig::Width;
    static constexpr int HEIGHT = MapConfig::Height;

private:
    char fTiles[HEIGHT][WIDTH];      // '.', 'C', 'K', '#' Core, 'S' enemy spawn
    List<Building*> fBuildings;      // Doubly Linked-List of polymorphic buildings
    BTree<TechNode>* fTechRoot;      // Binary Tree for Tech unlocks
    int fCoreHP;
    int fMaxCoreHP;
    Position fCorePosition;
    Position fEnemySpawnPosition;

    void initTiles();
    void initTechTree();
    void clearBuildings();
    BTree<TechNode>* findTechNode(BTree<TechNode>* aNode, const std::string& aName) const;

public:
    Map();
    ~Map();

    Map(const Map&) = delete;
    Map& operator=(const Map&) = delete;

    // Reset map tiles, buildings, Core HP, and Tech Tree
    void reset();

    // Coordinate & tile helpers
    bool isInside(Position aPos) const;
    char getTile(Position aPos) const;
    Position getCorePosition() const;
    Position getEnemySpawnPosition() const;
    std::vector<std::string> getResourceLayout() const;
    bool restoreLayout(Position aCore, Position aSpawn, const std::vector<std::string>& aRows);

    // Building management (using List<Building*> and DoublyLinkedNodeIterator)
    Building* getBuildingAt(Position aPos) const;
    std::vector<Position> findPath(Position aStart, Position aGoal, bool aAllowBuildings = false) const;
    bool canPlaceBuilding(Position aPos, const std::string& aType, std::string& outError) const;
    void addBuilding(Building* aBuilding);
    bool removeBuildingAt(Position aPos);
    const List<Building*>& getBuildings() const;

    // Core health management
    int getCoreHP() const;
    int getMaxCoreHP() const;
    void setCoreHP(int aHP);
    void damageCore(int aAmount);
    bool isCoreDestroyed() const;

    // Concept 3c: Tech Tree operations
    bool isTechUnlocked(const std::string& aTechName) const;
    void setTechUnlocked(const std::string& aTechName, bool aUnlocked);
    bool unlockTech(const std::string& aTechName, ResourceStore& aResources, std::string& outMessage);
    std::string getTechTreeDisplay() const;

    // Returns ASCII grid representation 
    std::string getGridDisplay() const;
};
