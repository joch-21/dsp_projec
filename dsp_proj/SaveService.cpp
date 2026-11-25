#include "SaveService.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <vector>

bool SaveService::saveGame(const std::string& aFilename,
    const Map& aMap,
    const ResourceStore& aResources,
    const WaveManager& aWaves)
{
    std::ofstream outFile(aFilename);
    if (!outFile.is_open())
    {
        return false;
    }

    const Position core = aMap.getCorePosition();
    const Position spawn = aMap.getEnemySpawnPosition();
    outFile << "MAP " << Map::WIDTH << " " << Map::HEIGHT << " "
        << core.x << " " << core.y << " " << spawn.x << " " << spawn.y << "\n";
    outFile << "TILES\n";
    for (const std::string& row : aMap.getResourceLayout()) outFile << row << "\n";
    outFile << "CORE_HP " << aMap.getCoreHP() << "\n";
    outFile << "WAVE " << aWaves.getCurrentWave() << "\n";
    outFile << "RESOURCES "
        << aResources.get("Copper") << " "
        << aResources.get("Coal") << " "
        << aResources.get("Power") << "\n";

    outFile << "TECH "
        << (aMap.isTechUnlocked("Drill") ? 1 : 0) << " "
        << (aMap.isTechUnlocked("Generator") ? 1 : 0) << " "
        << (aMap.isTechUnlocked("Turret") ? 1 : 0) << "\n";

    const List<Building*>& buildings = aMap.getBuildings();
    outFile << "BUILDINGS " << buildings.size() << "\n";

    for (auto it = buildings.getIteratorFront(); it != it.end(); ++it)
    {
        Building* b = *it;
        std::string extra = "None";
        if (b->getType() == "Drill")
        {
            Drill* d = static_cast<Drill*>(b);
            extra = d->getTargetOre();
        }

        outFile << b->getType() << " "
            << extra << " "
            << b->getPos().x << " "
            << b->getPos().y << " "
            << b->getLevel() << " "
            << b->getHP() << "\n";
    }

    outFile.flush();
    return outFile.good();
}

bool SaveService::loadGame(const std::string& aFilename,
    Map& aMap,
    ResourceStore& aResources,
    WaveManager& aWaves)
{
    std::ifstream inFile(aFilename);
    if (!inFile.is_open())
    {
        return false;
    }

    std::string label;
    int coreHP = 0;
    Position corePosition{};
    Position spawnPosition{};
    std::vector<std::string> resourceRows(Map::HEIGHT, std::string(Map::WIDTH, '.'));
    if (!(inFile >> label)) return false;
    if (label == "MAP")
    {
        int width = 0, height = 0;
        if (!(inFile >> width >> height >> corePosition.x >> corePosition.y >>
            spawnPosition.x >> spawnPosition.y) || width != Map::WIDTH || height != Map::HEIGHT)
            return false;
        if (!(inFile >> label) || label != "TILES") return false;
        for (std::string& row : resourceRows)
            if (!(inFile >> row)) return false;
        if (!(inFile >> label >> coreHP) || label != "CORE_HP") return false;
    }
    else if (label == "CORE_HP")
    {
        // Migrate saves made before random layouts were added.
        if (!(inFile >> coreHP)) return false;
        corePosition = { 0, 2 };
        spawnPosition = { Map::WIDTH - 1, Map::HEIGHT - 1 };
        const Position copper[] = { { 1, 1 }, { 1, 2 }, { 2, 0 } };
        const Position coal[] = { { 1, 3 }, { 2, 4 }, { 3, 3 } };
        for (Position pos : copper) resourceRows[pos.y][pos.x] = 'C';
        for (Position pos : coal) resourceRows[pos.y][pos.x] = 'K';
    }
    else return false;

    // Validate the saved geometry before touching the running map.
    Map layoutValidator;
    if (!layoutValidator.restoreLayout(corePosition, spawnPosition, resourceRows)) return false;

    int savedWave = 0;
    int copper = 0, coal = 0, power = 0;
    int tDrill = 0, tGen = 0, tTurret = 0;
    int buildingCount = 0;

    if (!(inFile >> label >> savedWave) || label != "WAVE") return false;
    if (!(inFile >> label >> copper >> coal >> power) || label != "RESOURCES") return false;
    if (!(inFile >> label >> tDrill >> tGen >> tTurret) || label != "TECH") return false;
    if (!(inFile >> label >> buildingCount) || label != "BUILDINGS") return false;

    if (coreHP < 0 || coreHP > aMap.getMaxCoreHP() || savedWave < 0 || savedWave > 4 ||
        copper < 0 || coal < 0 || power < 0 ||
        (tDrill != 0 && tDrill != 1) || (tGen != 0 && tGen != 1) ||
        (tTurret != 0 && tTurret != 1) || tDrill != 1 ||
        (tGen == 1 && tDrill != 1) || buildingCount < 0 || buildingCount > Map::WIDTH * Map::HEIGHT)
    {
        return false;
    }

    struct BuildingRecord
    {
        std::string type;
        std::string extra;
        int x;
        int y;
        int level;
        int hp;
    };
    std::vector<BuildingRecord> records;
    records.reserve(static_cast<size_t>(buildingCount));
    bool occupied[Map::HEIGHT][Map::WIDTH] = {};

    for (int i = 0; i < buildingCount; ++i)
    {
        std::string bType, extra;
        int x, y, level, hp;
        if (!(inFile >> bType >> extra >> x >> y >> level >> hp))
        {
            return false;
        }

        if (x < 0 || x >= Map::WIDTH || y < 0 || y >= Map::HEIGHT ||
            level < 1 || level > 100 || hp < 0 || occupied[y][x] ||
            (x == corePosition.x && y == corePosition.y) ||
            (x == spawnPosition.x && y == spawnPosition.y))
        {
            return false;
        }

        int maxHP = 0;
        if (bType == "Drill")
        {
            char tile = resourceRows[y][x];
            if ((extra != "Copper" && extra != "Coal") ||
                (extra == "Copper" && tile != 'C') || (extra == "Coal" && tile != 'K')) return false;
            maxHP = BuildingConfig::DrillHitPoints +
                BuildingConfig::BaseUpgradeHitPoints * (level - 1);
        }
        else if (bType == "Generator")
        {
            if (extra != "None" || tGen != 1) return false;
            maxHP = BuildingConfig::GeneratorHitPoints +
                BuildingConfig::BaseUpgradeHitPoints * (level - 1);
        }
        else if (bType == "Turret")
        {
            if (extra != "None" || tTurret != 1) return false;
            maxHP = BuildingConfig::TurretHitPoints +
                BuildingConfig::BaseUpgradeHitPoints * (level - 1);
        }
        else
        {
            return false;
        }

        occupied[y][x] = true;
        // Older saves used larger base building HP values; clamp those records
        // to the current configured maximum instead of rejecting the whole save.
        records.push_back({ bType, extra, x, y, level, std::min(hp, maxHP) });
    }

    inFile >> std::ws;
    if (!inFile.eof()) return false;

    // Validation is complete; only now replace the live game state.
    aMap.reset();
    if (!aMap.restoreLayout(corePosition, spawnPosition, resourceRows)) return false;
    aMap.setCoreHP(coreHP);
    aMap.setTechUnlocked("Drill", tDrill == 1);
    aMap.setTechUnlocked("Generator", tGen == 1);
    aMap.setTechUnlocked("Turret", tTurret == 1);
    for (const BuildingRecord& record : records)
    {
        std::unique_ptr<Building> building;
        Position pos{ record.x, record.y };
        if (record.type == "Drill") building = std::make_unique<Drill>(pos, record.extra);
        else if (record.type == "Generator") building = std::make_unique<Generator>(pos);
        else building = std::make_unique<Turret>(pos);

        for (int level = 1; level < record.level; ++level) building->upgrade();
        building->damaged(building->getMaxHP() - record.hp);
        aMap.addBuilding(building.release());
    }

    aWaves.syncToWave(savedWave);
    aResources.set("Copper", copper);
    aResources.set("Coal", coal);
    aResources.set("Power", power);
    return true;
}
