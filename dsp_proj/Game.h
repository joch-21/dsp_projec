#pragma once
#include <string>
#include "Map.h"
#include "ResourceStore.h"
#include "WaveManager.h"
#include "CommandManager.h"
#include "BuildingVisitor.h"
#include "SaveService.h"
#include "Enemy.h"
#include <vector>

// Session defaults and shared simulation cadence live with the Game owner.
namespace GameConfig
{
    inline constexpr int StartingCopper = 65;
    inline constexpr int StartingCoal = 10;
    inline constexpr int StartingPower = 0;
    inline constexpr unsigned int ProductionIntervalMilliseconds = 2000;
}

class Game
{
private:
    Map fMap;
    ResourceStore fResources;
    WaveManager fWaveManager;
    CommandManager fCommandManager;
    bool fGameOver;
    bool fVictory;
    std::string fSaveFilename;

public:
    Game(const std::string& aSaveFile = "savegame.txt");

    // Reset all subsystems for a fresh game
    void startNewGame();

    // Player actions (return true on success, with human-readable status message)
    bool placeBuilding(Position aPos, const std::string& aType, std::string& outMsg);
    bool upgradeBuilding(Position aPos, std::string& outMsg);
    bool undoLastBuild(std::string& outMsg);
    bool unlockTech(const std::string& aTechName, std::string& outMsg);

    // Begin a wave and return the living enemy HP values before any turret attacks.
    std::vector<int> startWave(std::string& outEnemyName, int& outEnemyDamage);
    bool fireTurret(Position aPos, int& outDamage);
    void damageCore(int aAmount);
    void finishWave();
    void runProductionInterval();

    // Run BuildingReportVisitor across all placed buildings
    std::string getBuildingReport() const;

    // File operations via SaveService
    bool save(std::string& outMsg) const;
    bool load(std::string& outMsg);

    // Read-only state accessors for any UI layer (Console, 2D, GUI)
    Map& getMap();
    const Map& getMap() const;
    ResourceStore& getResources();
    const ResourceStore& getResources() const;
    const WaveManager& getWaveManager() const;
    const CommandManager& getCommandManager() const;
    bool isGameOver() const;
    bool isVictory() const;
};
