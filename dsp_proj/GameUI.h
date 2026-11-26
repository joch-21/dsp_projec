#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "Game.h"

// Rendering and real-time combat tuning lives beside the UI simulation loop.
namespace GameUIConfig
{
    inline constexpr unsigned int AnimationTickMilliseconds = 75;
    inline constexpr unsigned int WaveSpawnIntervalMilliseconds = 15000;
    inline constexpr double EnemySpeedTilesPerSecond = 0.75;
    inline constexpr int EnemySpawnSpacingTicks = 12;
    inline constexpr int EnemyAttackCooldownTicks = 8;
    inline constexpr const char* WindowTitle = "COPPERLINE - COS30008 Data Structure & Patterns Project (105811045)";
    inline constexpr const char* CopperLabel = "Copper";
    inline constexpr const char* CoalLabel = "Coal";
    inline constexpr const char* PowerLabel = "Power";
    inline constexpr const char* ResourcePanelLabel = "RESOURCES";
    inline constexpr const char* DrillButtonLabel = "Build Drill";
    inline constexpr const char* GeneratorButtonLabel = "Build Generator";
    inline constexpr const char* GeneratorUnlockLabel = "Unlock Generator";
    inline constexpr const char* TurretButtonLabel = "Build Turret";
    inline constexpr const char* TurretUnlockLabel = "Unlock Turret";
    inline constexpr const char* UpgradeButtonLabel = "Upgrade Building";
    inline constexpr const char* ProductionLabel = "Generator burns";
    inline constexpr const char* TurretStatsLabel = "Turret:";
    inline constexpr const char* TurretRangeLabel = "Range:";
}

class GameUI
{
public:
    enum class ToolMode
    {
        None,
        BuildDrill,
        BuildGenerator,
        BuildTurret,
        Upgrade
    };

private:
    Game& fGame;
    sf::RenderWindow fWindow;
    sf::Font fFont;
    sf::Clock fFrameClock;
    float fTickAccumulator;
    ToolMode fSelectedTool;
    std::string fStatusMessage;
    std::vector<int> fEnemyHealth;
    std::vector<int> fEnemyVisualHealth;
    std::vector<double> fEnemyGridX;
    std::vector<double> fEnemyGridY;
    std::vector<bool> fEnemySpawned;
    std::vector<bool> fEnemyCoreHitCounted;
    std::vector<int> fEnemyAttackCooldowns;
    std::vector<Position> fWaveTurretPositions;
    std::vector<int> fWaveTurretCooldowns;
    std::vector<Position> fWaveBeamOrigins;
    std::vector<int> fWaveBeamTargets;
    int fEnemyMaxHP;
    int fWaveCoreDamage;
    int fWaveDefeatedCount;
    int fWaveStep;
    int fWaveHitCount;
    int fWaveNextShot;
    int fWaveAttackDamage;
    int fWaveBeamTicks;
    int fWaveVisualCoreHP;
    int fWaveImpactTicks;
    int fWaveWaveNumber;
    int fElapsedMilliseconds;
    int fProductionAccumulatorMilliseconds;
    int fWaveCountdownMilliseconds;
    bool fWaveActive;
    bool fPaused;
    bool fShowDiagnostics;
    std::string fWaveEnemyName;

    static constexpr int WINDOW_WIDTH = 1020;
    static constexpr int WINDOW_HEIGHT = 750;
    static constexpr int TILE_SIZE = 58;
    static constexpr int TILE_GAP = 4;
    static constexpr int GRID_OFFSET_X = 36;
    static constexpr int GRID_OFFSET_Y = 130;
    static constexpr int PANEL_X = 670;

    void startTimedWave();
    void simulateRealtimeStep();
    void handleMouseClick(int aMouseX, int aMouseY);
    void render();
    void drawGrid();
    void drawBuildings();
    void drawBattle();
    void drawSidePanel();
    void drawDiagnostics();
    void drawText(const std::string& aText, float aX, float aY,
        sf::Color aColor, unsigned int aSize = 16);
    void drawButton(int aX, int aY, int aWidth, int aHeight, const std::string& aLabel,
        sf::Color aColor, bool aSelected = false, bool aEnabled = true);
    sf::Vector2f gridToPixel(double aGridX, double aGridY,
        int aEnemyIndex = 0, int aEnemyCount = 1) const;

public:
    explicit GameUI(Game& aGame);
    GameUI(const GameUI&) = delete;
    GameUI& operator=(const GameUI&) = delete;
    void run();
};
