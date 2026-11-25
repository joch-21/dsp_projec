#include "Game.h"
#include <algorithm>
#include <sstream>

Game::Game(const std::string& aSaveFile)
    : fGameOver(false), fVictory(false), fSaveFilename(aSaveFile)
{
    startNewGame();
}

void Game::startNewGame()
{
    fMap.reset();
    fResources.clear();
    fResources.set("Copper", GameConfig::StartingCopper);
    fResources.set("Coal", GameConfig::StartingCoal);
    fResources.set("Power", GameConfig::StartingPower);

    fWaveManager.loadDefaultWaves();
    fCommandManager.clear();

    fGameOver = false;
    fVictory = false;
}

bool Game::placeBuilding(Position aPos, const std::string& aType, std::string& outMsg)
{
    if (fGameOver)
    {
        outMsg = "Game is already over.";
        return false;
    }

    if (!fMap.canPlaceBuilding(aPos, aType, outMsg))
    {
        return false;
    }

    int cost = 0;
    Building* newBuilding = nullptr;

    if (aType == "Drill")
    {
        cost = BuildingConfig::DrillCost;
        if (!fResources.spend("Copper", cost))
        {
            outMsg = "Not enough Copper! Drill costs " + std::to_string(cost) + " Copper.";
            return false;
        }
        char tile = fMap.getTile(aPos);
        std::string ore = (tile == 'K') ? "Coal" : "Copper";
        newBuilding = new Drill(aPos, ore);
    }
    else if (aType == "Generator")
    {
        cost = BuildingConfig::GeneratorCost;
        if (!fResources.spend("Copper", cost))
        {
            outMsg = "Not enough Copper! Generator costs " + std::to_string(cost) + " Copper.";
            return false;
        }
        newBuilding = new Generator(aPos);
    }
    else if (aType == "Turret")
    {
        cost = BuildingConfig::TurretCost;
        if (!fResources.spend("Copper", cost))
        {
            outMsg = "Not enough Copper! Turret costs " + std::to_string(cost) + " Copper.";
            return false;
        }
        newBuilding = new Turret(aPos);
    }
    else
    {
        outMsg = "Unknown building type: " + aType;
        return false;
    }

    fMap.addBuilding(newBuilding);
    fCommandManager.recordBuild(aPos, aType, cost);

    std::ostringstream oss;
    oss << "Built " << newBuilding->getName() << " at (" << aPos.x << "," << aPos.y
        << ") for " << cost << " Copper.";
    outMsg = oss.str();
    return true;
}

bool Game::upgradeBuilding(Position aPos, std::string& outMsg)
{
    if (fGameOver)
    {
        outMsg = "Game is already over.";
        return false;
    }

    Building* b = fMap.getBuildingAt(aPos);
    if (b == nullptr)
    {
        outMsg = "No building found at those coordinates.";
        return false;
    }

    int upgradeCost = BuildingConfig::UpgradeCostPerLevel * b->getLevel();
    if (!fResources.spend("Copper", upgradeCost))
    {
        outMsg = "Not enough Copper! Upgrade costs " + std::to_string(upgradeCost) + " Copper.";
        return false;
    }

    b->upgrade();
    std::ostringstream oss;
    oss << "Upgraded " << b->getName() << " to Lv." << b->getLevel()
        << " for " << upgradeCost << " Copper";
    if (b->getType() == "Drill")
    {
        oss << " | Mining: +" << static_cast<Drill*>(b)->getMineRate() << " ore/2s";
    }
    else if (b->getType() == "Generator")
    {
        oss << " | Power: +" << static_cast<Generator*>(b)->getPowerOut() << "/2s";
    }
    else if (b->getType() == "Turret")
    {
        Turret* turret = static_cast<Turret*>(b);
        oss << " | Damage: " << turret->getAttackDmg() << "/shot"
            << " | Range: " << turret->getRangeTiles() << " block(s)";
    }
    oss << '!';
    outMsg = oss.str();
    return true;
}

bool Game::undoLastBuild(std::string& outMsg)
{
    if (fGameOver)
    {
        outMsg = "Game is already over.";
        return false;
    }

    while (fCommandManager.canUndo() &&
        fMap.getBuildingAt(fCommandManager.getTopCommand().pos) == nullptr)
    {
        // Buildings destroyed by enemies invalidate their old build command.
        // Discard those stale entries so the Undo control remains usable.
        fCommandManager.popLastCommand();
    }

    if (!fCommandManager.canUndo())
    {
        outMsg = "Nothing to undo.";
        return false;
    }

    BuildCommand cmd = fCommandManager.popLastCommand();
    if (fMap.removeBuildingAt(cmd.pos))
    {
        fResources.add("Copper", cmd.copperRefund);
        std::ostringstream oss;
        oss << "Undid " << cmd.buildingType << " at (" << cmd.pos.x << "," << cmd.pos.y
            << "). Refunded " << cmd.copperRefund << " Copper.";
        outMsg = oss.str();
        return true;
    }

    outMsg = "Could not undo: building was already removed or destroyed.";
    return false;
}

bool Game::unlockTech(const std::string& aTechName, std::string& outMsg)
{
    if (fGameOver)
    {
        outMsg = "Game is already over.";
        return false;
    }
    return fMap.unlockTech(aTechName, fResources, outMsg);
}

std::vector<int> Game::startWave(std::string& outEnemyName, int& outEnemyDamage)
{
    outEnemyName.clear();
    outEnemyDamage = 0;
    if (!fWaveManager.hasNextWave()) return {};

    SpawnEvent event = fWaveManager.spawnNextWave();
    outEnemyName = event.enemyName;
    outEnemyDamage = event.enemyDamage;
    std::vector<int> enemyHealth;
    enemyHealth.reserve(static_cast<size_t>(event.enemyCount));
    for (int i = 0; i < event.enemyCount; ++i)
    {
        enemyHealth.push_back(event.enemyHP);
    }
    return enemyHealth;
}

bool Game::fireTurret(Position aPos, int& outDamage)
{
    outDamage = 0;
    if (fGameOver) return false;
    Building* building = fMap.getBuildingAt(aPos);
    if (building == nullptr || building->getType() != "Turret" || building->isDestroyed()) return false;

    Turret* turret = static_cast<Turret*>(building);
    if (!fResources.spend("Power", turret->getPowerReq())) return false;
    if (!fResources.spend("Copper", turret->getCopperAmmoReq()))
    {
        fResources.add("Power", turret->getPowerReq());
        return false;
    }

    outDamage = turret->getAttackDmg();
    return true;
}

void Game::damageCore(int aAmount)
{
    fMap.damageCore(aAmount);
    if (fMap.isCoreDestroyed())
    {
        fGameOver = true;
        fVictory = false;
    }
}

void Game::finishWave()
{
    if (!fMap.isCoreDestroyed() && !fWaveManager.hasNextWave())
    {
        fGameOver = true;
        fVictory = true;
    }
}

void Game::runProductionInterval()
{
    if (fGameOver) return;

    // Mining runs before generation so newly mined Coal can feed Generators
    // during this same production interval. Turrets fire continuously in GameUI.
    ProductionSummaryVisitor production(fResources);
    const List<Building*>& buildings = fMap.getBuildings();
    for (int phase = 0; phase < 2; ++phase)
    {
        for (auto it = buildings.getIteratorFront(); it != it.end(); ++it)
        {
            Building* building = *it;
            bool process = phase == 0 ? building->getType() == "Drill" :
                building->getType() == "Generator";
            if (process) building->accept(production);
        }
    }
}

std::string Game::getBuildingReport() const
{
    BuildingReportVisitor reportVisitor;
    const List<Building*>& buildings = fMap.getBuildings();

    if (buildings.isEmpty())
    {
        return "No buildings placed yet.\n";
    }

    for (auto it = buildings.getIteratorFront(); it != it.end(); ++it)
    {
        (*it)->accept(reportVisitor);
    }
    return reportVisitor.getReport();
}

bool Game::save(std::string& outMsg) const
{
    if (SaveService::saveGame(fSaveFilename, fMap, fResources, fWaveManager))
    {
        outMsg = "Game saved to " + fSaveFilename + ".";
        return true;
    }
    outMsg = "Failed to open " + fSaveFilename + " for saving.";
    return false;
}

bool Game::load(std::string& outMsg)
{
    if (SaveService::loadGame(fSaveFilename, fMap, fResources, fWaveManager))
    {
        fCommandManager.clear();
        fGameOver = fMap.isCoreDestroyed() || !fWaveManager.hasNextWave();
        fVictory = !fMap.isCoreDestroyed() && !fWaveManager.hasNextWave();
        outMsg = "Game loaded from " + fSaveFilename + ".";
        return true;
    }
    outMsg = "Failed to load " + fSaveFilename + " (file missing or invalid).";
    return false;
}

const Map& Game::getMap() const { return fMap; }
Map& Game::getMap() { return fMap; }
ResourceStore& Game::getResources() { return fResources; }
const ResourceStore& Game::getResources() const { return fResources; }
const WaveManager& Game::getWaveManager() const { return fWaveManager; }
const CommandManager& Game::getCommandManager() const { return fCommandManager; }
bool Game::isGameOver() const { return fGameOver; }
bool Game::isVictory() const { return fVictory; }
