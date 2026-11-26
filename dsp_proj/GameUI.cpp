#include "GameUI.h"
#include "Building.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace
{
    const sf::Color Background(22, 26, 34);
    const sf::Color Panel(32, 37, 48);

    void drawLine(sf::RenderWindow& window, sf::Vector2f from, sf::Vector2f to,
        sf::Color color, float thickness = 2.f)
    {
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length <= 0.f) return;

        sf::RectangleShape line({ length, thickness });
        line.setOrigin({ 0.f, thickness / 2.f });
        line.setPosition(from);
        line.setRotation(sf::degrees(std::atan2(dy, dx) * 180.f / 3.14159265f));
        line.setFillColor(color);
        window.draw(line);
    }

    sf::Color tileColor(char kind)
    {
        if (kind == 'C') return sf::Color(122, 73, 42);
        if (kind == 'K') return sf::Color(53, 56, 65);
        if (kind == '#') return sf::Color(40, 130, 195);
        if (kind == 'S') return sf::Color(68, 35, 44);
        return sf::Color(38, 44, 56);
    }
}

GameUI::GameUI(Game& aGame)
    : fGame(aGame),
    fWindow(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }),
        GameUIConfig::WindowTitle, sf::Style::Titlebar | sf::Style::Close),
    fTickAccumulator(0.f),
    fSelectedTool(ToolMode::BuildDrill),
    fStatusMessage("Build your base before the first wave arrives."),
    fEnemyMaxHP(0), fWaveCoreDamage(0), fWaveDefeatedCount(0),
    fWaveStep(0), fWaveHitCount(0), fWaveNextShot(0), fWaveAttackDamage(0),
    fWaveBeamTicks(0), fWaveVisualCoreHP(0), fWaveImpactTicks(0), fWaveWaveNumber(0),
    fElapsedMilliseconds(0), fProductionAccumulatorMilliseconds(0),
    fWaveCountdownMilliseconds(GameUIConfig::WaveSpawnIntervalMilliseconds),
    fWaveActive(false), fPaused(false), fShowDiagnostics(false)
{
    fWindow.setFramerateLimit(60);
    // Visual Studio launches from the output directory, which may differ from
    // the project working directory. Check both locations and the source asset.
    const std::filesystem::path fontCandidates[] = {
        "tuffy.ttf",
        "assets/tuffy.ttf",
        std::filesystem::path("..") / ".." / "dsp_proj" / "assets" / "tuffy.ttf"
    };
    bool fontLoaded = false;
    for (const auto& candidate : fontCandidates)
    {
        if (fFont.openFromFile(candidate.string()))
        {
            fontLoaded = true;
            break;
        }
    }
    if (!fontLoaded)
    {
        throw std::runtime_error("Could not load tuffy.ttf. Ensure assets/tuffy.ttf is copied beside the executable.");
    }
}

sf::Vector2f GameUI::gridToPixel(double aGridX, double aGridY,
    int aEnemyIndex, int aEnemyCount) const
{
    const int formationOffset = (2 * aEnemyIndex - (aEnemyCount - 1)) * 10;
    return {
        static_cast<float>(GRID_OFFSET_X + std::lround(aGridX * (TILE_SIZE + TILE_GAP)) +
            TILE_SIZE / 2 + formationOffset),
        static_cast<float>(GRID_OFFSET_Y + std::lround(aGridY * (TILE_SIZE + TILE_GAP)) +
            TILE_SIZE / 2)
    };
}

void GameUI::startTimedWave()
{
    if (fWaveActive || !fGame.getWaveManager().hasNextWave() || fGame.isGameOver()) return;

    fEnemyHealth = fGame.startWave(fWaveEnemyName, fWaveAttackDamage);
    if (fEnemyHealth.empty()) return;

    fWaveActive = true;
    fWaveStep = 0;
    fWaveHitCount = 0;
    fWaveNextShot = 0;
    fWaveBeamTicks = 0;
    fWaveBeamOrigins.clear();
    fWaveBeamTargets.clear();
    fEnemyMaxHP = fEnemyHealth.front();
    fWaveCoreDamage = 0;
    fWaveDefeatedCount = 0;
    fWaveTurretPositions.clear();
    fWaveTurretCooldowns.clear();
    fEnemyVisualHealth = fEnemyHealth;
    const Position spawn = fGame.getMap().getEnemySpawnPosition();
    fEnemyGridX.assign(fEnemyHealth.size(), spawn.x);
    fEnemyGridY.assign(fEnemyHealth.size(), spawn.y);
    fEnemySpawned.assign(fEnemyHealth.size(), false);
    fEnemyCoreHitCounted.assign(fEnemyHealth.size(), false);
    fEnemyAttackCooldowns.assign(fEnemyHealth.size(), 0);

    const List<Building*>& buildings = fGame.getMap().getBuildings();
    for (auto it = buildings.getIteratorFront(); it != it.end(); ++it)
    {
        Building* building = *it;
        if (building->getType() == "Turret" && !building->isDestroyed())
        {
            fWaveTurretPositions.push_back(building->getPos());
            fWaveTurretCooldowns.push_back(0);
        }
    }

    fWaveVisualCoreHP = fGame.getMap().getCoreHP();
    fWaveWaveNumber = fGame.getWaveManager().getCurrentWave();
    fStatusMessage = "Wave " + std::to_string(fWaveWaveNumber) + " incoming from an outer-edge spawn!";
}

void GameUI::simulateRealtimeStep()
{
    if (fPaused || fGame.isGameOver()) return;

    constexpr int tickMilliseconds = static_cast<int>(GameUIConfig::AnimationTickMilliseconds);
    fElapsedMilliseconds += tickMilliseconds;
    fProductionAccumulatorMilliseconds += tickMilliseconds;
    while (fProductionAccumulatorMilliseconds >=
        static_cast<int>(GameConfig::ProductionIntervalMilliseconds))
    {
        fGame.runProductionInterval();
        fProductionAccumulatorMilliseconds -=
            static_cast<int>(GameConfig::ProductionIntervalMilliseconds);
    }

    if (!fWaveActive)
    {
        if (fGame.getWaveManager().hasNextWave())
        {
            fWaveCountdownMilliseconds -= tickMilliseconds;
            if (fWaveCountdownMilliseconds <= 0) startTimedWave();
        }
        if (!fWaveActive) return;
    }

    if (fWaveImpactTicks > 0) --fWaveImpactTicks;
    if (fWaveBeamTicks > 0 && --fWaveBeamTicks == 0)
    {
        fWaveBeamOrigins.clear();
        fWaveBeamTargets.clear();
    }
    ++fWaveStep;

    Map& map = fGame.getMap();
    const Position core = map.getCorePosition();
    for (size_t i = 0; i < fEnemyHealth.size(); ++i)
    {
        if (fEnemyVisualHealth[i] <= 0) continue;
        if (!fEnemySpawned[i])
        {
            if (fWaveStep < static_cast<int>(i) * GameUIConfig::EnemySpawnSpacingTicks) continue;
            fEnemySpawned[i] = true;
        }
        if (fEnemyAttackCooldowns[i] > 0) --fEnemyAttackCooldowns[i];

        Position cell{ static_cast<int>(std::lround(fEnemyGridX[i])),
            static_cast<int>(std::lround(fEnemyGridY[i])) };
        if (cell.x == core.x && cell.y == core.y)
        {
            const double coreDx = core.x - fEnemyGridX[i];
            const double coreDy = core.y - fEnemyGridY[i];
            const double coreDistance = std::sqrt(coreDx * coreDx + coreDy * coreDy);
            const double movement = GameUIConfig::EnemySpeedTilesPerSecond *
                tickMilliseconds / 1000.0;
            if (coreDistance > 0.01)
            {
                if (coreDistance <= movement)
                {
                    fEnemyGridX[i] = core.x;
                    fEnemyGridY[i] = core.y;
                }
                else
                {
                    fEnemyGridX[i] += coreDx / coreDistance * movement;
                    fEnemyGridY[i] += coreDy / coreDistance * movement;
                }
                continue;
            }
            if (!fEnemyCoreHitCounted[i])
            {
                fEnemyCoreHitCounted[i] = true;
                ++fWaveHitCount;
            }
            if (fEnemyAttackCooldowns[i] == 0)
            {
                fGame.damageCore(fWaveAttackDamage);
                fWaveCoreDamage += fWaveAttackDamage;
                fEnemyAttackCooldowns[i] = GameUIConfig::EnemyAttackCooldownTicks;
                fWaveVisualCoreHP = fGame.getMap().getCoreHP();
                fWaveImpactTicks = 10;
                fStatusMessage = fWaveEnemyName + " is attacking the Core for " +
                    std::to_string(fWaveAttackDamage) + " damage!";
                if (fGame.isGameOver())
                {
                    fWaveActive = false;
                    fStatusMessage = "The Core was destroyed by the attacking wave!";
                    return;
                }
            }
            continue;
        }

        std::vector<Position> path = map.findPath(cell, core);
        if (path.empty())
        {
            // Follow the shortest route even when blocked, moving to the first
            // obstacle and attacking it until the route opens.
            path = map.findPath(cell, core, true);
            Building* blocker = nullptr;
            size_t blockerIndex = path.size();
            for (size_t pathIndex = 0; pathIndex < path.size(); ++pathIndex)
            {
                blocker = map.getBuildingAt(path[pathIndex]);
                if (blocker != nullptr)
                {
                    blockerIndex = pathIndex;
                    break;
                }
            }
            if (blocker != nullptr && blockerIndex > 1 && path.size() > 1)
            {
                const Position next = path[1];
                const double dx = next.x - fEnemyGridX[i];
                const double dy = next.y - fEnemyGridY[i];
                const double distance = std::sqrt(dx * dx + dy * dy);
                const double move = GameUIConfig::EnemySpeedTilesPerSecond * tickMilliseconds / 1000.0;
                if (distance <= move)
                {
                    fEnemyGridX[i] = next.x;
                    fEnemyGridY[i] = next.y;
                }
                else if (distance > 0.0)
                {
                    fEnemyGridX[i] += dx / distance * move;
                    fEnemyGridY[i] += dy / distance * move;
                }
            }
            else if (blocker != nullptr && fEnemyAttackCooldowns[i] == 0)
            {
                blocker->damaged(fWaveAttackDamage);
                fEnemyAttackCooldowns[i] = GameUIConfig::EnemyAttackCooldownTicks;
                const Position blockerPosition = blocker->getPos();
                const std::string blockerName = blocker->getName();
                const int remainingHP = blocker->getHP();
                if (remainingHP <= 0)
                {
                    map.removeBuildingAt(blockerPosition);
                    fStatusMessage = fWaveEnemyName + " destroyed " + blockerName + " blocking its route!";
                }
                else
                {
                    fStatusMessage = fWaveEnemyName + " is attacking " + blockerName +
                        " (" + std::to_string(remainingHP) + " HP left).";
                }
            }
            continue;
        }

        if (path.size() < 2) continue;
        const Position next = path[1];
        const double dx = next.x - fEnemyGridX[i];
        const double dy = next.y - fEnemyGridY[i];
        const double distance = std::sqrt(dx * dx + dy * dy);
        const double movement = GameUIConfig::EnemySpeedTilesPerSecond * tickMilliseconds / 1000.0;
        if (distance <= movement)
        {
            fEnemyGridX[i] = next.x;
            fEnemyGridY[i] = next.y;
        }
        else if (distance > 0.0)
        {
            fEnemyGridX[i] += dx / distance * movement;
            fEnemyGridY[i] += dy / distance * movement;
        }
    }

    // Refresh turrets each tick so buildings created during a wave join combat.
    std::vector<Position> activeTurretPositions;
    std::vector<int> activeTurretCooldowns;
    const List<Building*>& liveBuildings = map.getBuildings();
    for (auto it = liveBuildings.getIteratorFront(); it != it.end(); ++it)
    {
        Building* building = *it;
        if (building->getType() != "Turret" || building->isDestroyed()) continue;
        const Position position = building->getPos();
        int cooldown = 0;
        for (size_t oldIndex = 0; oldIndex < fWaveTurretPositions.size(); ++oldIndex)
        {
            if (fWaveTurretPositions[oldIndex].x == position.x &&
                fWaveTurretPositions[oldIndex].y == position.y)
            {
                cooldown = fWaveTurretCooldowns[oldIndex];
                break;
            }
        }
        activeTurretPositions.push_back(position);
        activeTurretCooldowns.push_back(cooldown);
    }
    fWaveTurretPositions.swap(activeTurretPositions);
    fWaveTurretCooldowns.swap(activeTurretCooldowns);

    for (size_t turretIndex = 0; turretIndex < fWaveTurretPositions.size(); ++turretIndex)
    {
        if (fWaveTurretCooldowns[turretIndex] > 0) --fWaveTurretCooldowns[turretIndex];
        if (fWaveTurretCooldowns[turretIndex] > 0) continue;

        const Position turretPosition = fWaveTurretPositions[turretIndex];
        Building* turretBuilding = map.getBuildingAt(turretPosition);
        if (turretBuilding == nullptr || turretBuilding->isDestroyed()) continue;
        Turret* turret = static_cast<Turret*>(turretBuilding);
        const double range = turret->getRangeTiles();
        int target = -1;
        double nearestDistance = range;
        for (int enemy = 0; enemy < static_cast<int>(fEnemyVisualHealth.size()); ++enemy)
        {
            if (!fEnemySpawned[enemy] || fEnemyVisualHealth[enemy] <= 0) continue;
            const double dx = turretPosition.x - fEnemyGridX[enemy];
            const double dy = turretPosition.y - fEnemyGridY[enemy];
            const double distance = std::sqrt(dx * dx + dy * dy);
            if (distance <= nearestDistance)
            {
                target = enemy;
                nearestDistance = distance;
            }
        }

        if (target < 0) continue;
        int damage = 0;
        if (!fGame.fireTurret(turretPosition, damage)) continue;
        fWaveTurretCooldowns[turretIndex] = BuildingConfig::TurretFireCooldownTicks;
        ++fWaveNextShot;
        fEnemyVisualHealth[target] = std::max(0, fEnemyVisualHealth[target] - damage);
        fEnemyHealth[target] = fEnemyVisualHealth[target];
        fWaveBeamOrigins.push_back(turretPosition);
        fWaveBeamTargets.push_back(target);
        fWaveBeamTicks = 4;
        fStatusMessage = fEnemyVisualHealth[target] == 0
            ? fWaveEnemyName + " destroyed by turret fire!"
            : fWaveEnemyName + " hit for " + std::to_string(damage) + " damage (" +
                std::to_string(fEnemyVisualHealth[target]) + " HP left).";
        if (fEnemyVisualHealth[target] == 0) ++fWaveDefeatedCount;
    }

    bool waveResolved = true;
    for (size_t i = 0; i < fEnemyVisualHealth.size(); ++i)
    {
        if (!fEnemySpawned[i] || fEnemyVisualHealth[i] > 0)
        {
            waveResolved = false;
            break;
        }
    }
    if (waveResolved && fWaveBeamTicks == 0)
    {
        fGame.finishWave();
        fWaveActive = false;
        fEnemyHealth.clear();
        fEnemyVisualHealth.clear();
        fEnemyGridX.clear();
        fEnemyGridY.clear();
        fEnemySpawned.clear();
        fEnemyCoreHitCounted.clear();
        fEnemyAttackCooldowns.clear();
        fWaveTurretPositions.clear();
        fWaveTurretCooldowns.clear();
        fWaveCountdownMilliseconds = GameUIConfig::WaveSpawnIntervalMilliseconds;
        fStatusMessage = fGame.isVictory()
            ? "All waves defeated. Sector secured!"
            : "Wave " + std::to_string(fWaveWaveNumber) + " cleared: " +
                std::to_string(fWaveDefeatedCount) + " enemies destroyed; Core lost " +
                std::to_string(fWaveCoreDamage) + " HP.";
    }
}

void GameUI::handleMouseClick(int aMouseX, int aMouseY)
{
    for (int y = 0; y < Map::HEIGHT; ++y)
    {
        for (int x = 0; x < Map::WIDTH; ++x)
        {
            const int left = GRID_OFFSET_X + x * (TILE_SIZE + TILE_GAP);
            const int top = GRID_OFFSET_Y + y * (TILE_SIZE + TILE_GAP);
            if (aMouseX < left || aMouseX >= left + TILE_SIZE ||
                aMouseY < top || aMouseY >= top + TILE_SIZE) continue;

            if (fGame.isGameOver())
            {
                fStatusMessage = "Game Over! Use New Game to restart.";
                return;
            }
            const Position pos{ x, y };
            std::string message;
            switch (fSelectedTool)
            {
            case ToolMode::BuildDrill: fGame.placeBuilding(pos, "Drill", message); break;
            case ToolMode::BuildGenerator: fGame.placeBuilding(pos, "Generator", message); break;
            case ToolMode::BuildTurret: fGame.placeBuilding(pos, "Turret", message); break;
            case ToolMode::Upgrade: fGame.upgradeBuilding(pos, message); break;
            default: message = "Select a tool first."; break;
            }
            fStatusMessage = message;
            return;
        }
    }

    if (aMouseX < PANEL_X || aMouseX >= 980) return;
    std::string message;
    if (aMouseY >= 180 && aMouseY < 222)
    {
        fSelectedTool = ToolMode::BuildDrill;
        fStatusMessage = "Build Drill (" + std::to_string(BuildingConfig::DrillCost) +
            " Copper). Click a Copper or Coal deposit.";
    }
    else if (aMouseY >= 232 && aMouseY < 274)
    {
        if (fGame.getMap().isTechUnlocked("Generator"))
        {
            fSelectedTool = ToolMode::BuildGenerator;
            fStatusMessage = "Build Generator (" + std::to_string(BuildingConfig::GeneratorCost) +
                " Copper). Click an empty tile.";
        }
        else
        {
            fGame.unlockTech("Generator", message);
            fStatusMessage = message;
        }
    }
    else if (aMouseY >= 284 && aMouseY < 326)
    {
        if (fGame.getMap().isTechUnlocked("Turret"))
        {
            fSelectedTool = ToolMode::BuildTurret;
            fStatusMessage = "Build Turret (" + std::to_string(BuildingConfig::TurretCost) +
                " Copper). Click an empty tile.";
        }
        else
        {
            fGame.unlockTech("Turret", message);
            fStatusMessage = message;
        }
    }
    else if (aMouseY >= 336 && aMouseY < 378)
    {
        fSelectedTool = ToolMode::Upgrade;
        fStatusMessage = "Upgrade Building. Click an existing structure.";
    }
    else if (aMouseY >= 404 && aMouseY < 446)
    {
        fGame.undoLastBuild(message);
        fStatusMessage = message;
    }
    else if (aMouseY >= 456 && aMouseY < 504)
    {
        if (fGame.isGameOver())
        {
            fGame.startNewGame();
            fSelectedTool = ToolMode::BuildDrill;
            fWaveActive = false;
            fPaused = false;
            fTickAccumulator = 0.f;
            fElapsedMilliseconds = 0;
            fProductionAccumulatorMilliseconds = 0;
            fWaveCountdownMilliseconds = GameUIConfig::WaveSpawnIntervalMilliseconds;
            fEnemyHealth.clear();
            fEnemyVisualHealth.clear();
            fEnemyGridX.clear();
            fEnemyGridY.clear();
            fEnemySpawned.clear();
            fEnemyCoreHitCounted.clear();
            fEnemyAttackCooldowns.clear();
            fWaveTurretPositions.clear();
            fWaveTurretCooldowns.clear();
            fWaveBeamOrigins.clear();
            fWaveBeamTargets.clear();
            fStatusMessage = "New map generated. Build before the first wave.";
        }
        else
        {
            fPaused = !fPaused;
            fTickAccumulator = 0.f;
            fStatusMessage = fPaused ? "Simulation paused." : "Simulation resumed.";
        }
    }
    else if (aMouseY >= 518 && aMouseY < 558)
    {
        if (aMouseX < 820)
        {
            fGame.save(message);
        }
        else if (fGame.load(message))
        {
            fWaveActive = false;
            fPaused = false;
            fElapsedMilliseconds = 0;
            fProductionAccumulatorMilliseconds = 0;
            fWaveCountdownMilliseconds = GameUIConfig::WaveSpawnIntervalMilliseconds;
            fEnemyHealth.clear();
            fEnemyVisualHealth.clear();
            fEnemyGridX.clear();
            fEnemyGridY.clear();
            fEnemySpawned.clear();
            fEnemyCoreHitCounted.clear();
            fEnemyAttackCooldowns.clear();
            fWaveTurretPositions.clear();
            fWaveTurretCooldowns.clear();
            fWaveBeamOrigins.clear();
            fWaveBeamTargets.clear();
            fSelectedTool = ToolMode::BuildDrill;
        }
        fStatusMessage = message;
    }
}

void GameUI::drawText(const std::string& aText, float aX, float aY,
    sf::Color aColor, unsigned int aSize)
{
    sf::Text text(fFont, aText, aSize);
    text.setPosition({ aX, aY });
    text.setFillColor(aColor);
    fWindow.draw(text);
}

void GameUI::drawButton(int aX, int aY, int aWidth, int aHeight, const std::string& aLabel,
    sf::Color aColor, bool aSelected, bool aEnabled)
{
    sf::RectangleShape button({ static_cast<float>(aWidth), static_cast<float>(aHeight) });
    button.setPosition({ static_cast<float>(aX), static_cast<float>(aY) });
    button.setFillColor(aEnabled ? aColor : sf::Color(45, 48, 55));
    button.setOutlineColor(aSelected ? sf::Color(255, 210, 100) : sf::Color(95, 110, 135));
    button.setOutlineThickness(aSelected ? 3.f : 1.f);
    fWindow.draw(button);
    drawText(aLabel, static_cast<float>(aX + 12), static_cast<float>(aY + 10),
        aEnabled ? sf::Color(245, 247, 250) : sf::Color(130, 135, 145), 15);
}

void GameUI::render()
{
    fWindow.clear(Background);
    drawText("COPPERLINE", 36.f, 20.f, sf::Color(240, 165, 90), 23);

    const int elapsedSeconds = fElapsedMilliseconds / 1000;
    std::string status = "Time: " + std::to_string(elapsedSeconds / 60) + ":" +
        (elapsedSeconds % 60 < 10 ? "0" : "") + std::to_string(elapsedSeconds % 60) +
        "   |   Core HP: " + std::to_string(fGame.getMap().getCoreHP()) + "/" +
        std::to_string(fGame.getMap().getMaxCoreHP());
    if (fGame.isGameOver())
        status += fGame.isVictory() ? "   |   VICTORY" : "   |   DEFEAT";
    else if (fWaveActive)
        status += "   |   Wave " + std::to_string(fWaveWaveNumber) + " active";
    else if (fGame.getWaveManager().hasNextWave())
        status += "   |   Next wave: " +
            std::to_string((fWaveCountdownMilliseconds + 999) / 1000) + "s";
    if (fPaused) status += "   |   PAUSED";
    drawText(status, 36.f, 52.f, sf::Color(180, 210, 240), 15);

    drawGrid();
    drawBuildings();
    drawBattle();
    drawSidePanel();
    if (fShowDiagnostics) drawDiagnostics();

    sf::RectangleShape messagePanel({ 944.f, 42.f });
    messagePanel.setPosition({ 36.f, 680.f });
    messagePanel.setFillColor(sf::Color(30, 35, 46));
    messagePanel.setOutlineColor(sf::Color(65, 75, 95));
    messagePanel.setOutlineThickness(1.f);
    fWindow.draw(messagePanel);
    drawText(fStatusMessage, 48.f, 692.f, sf::Color(225, 230, 240), 13);
    fWindow.display();
}

void GameUI::drawDiagnostics()
{
    sf::RectangleShape overlay({ 930.f, 600.f });
    overlay.setPosition({ 45.f, 75.f });
    overlay.setFillColor(sf::Color(16, 20, 28, 245));
    overlay.setOutlineColor(sf::Color(120, 160, 205));
    overlay.setOutlineThickness(2.f);
    fWindow.draw(overlay);

    drawText("DATA STRUCTURE INSPECTOR  |  Press F1 to close", 65.f, 90.f,
        sf::Color(255, 205, 115), 20);
    drawText("Singly linked hash chains (ResourceStore):", 65.f, 130.f,
        sf::Color(150, 210, 255), 14);

    const std::string buckets = fGame.getResources().getDebugLayout();
    std::istringstream bucketStream(buckets);
    std::string line;
    float y = 154.f;
    while (std::getline(bucketStream, line))
    {
        drawText(line, 70.f, y, sf::Color(220, 225, 235), 12);
        y += 18.f;
    }

    ResourceStore collisionDemo;
    collisionDemo.set("A", 1);
    collisionDemo.set("H", 2); // A and H hash to the same bucket; the chain uses SLL nodes.
    drawText("SLL collision example (A and H share one bucket):", 65.f, y + 4.f,
        sf::Color(150, 210, 255), 12);
    drawText(collisionDemo.getDebugLayout(), 70.f, y + 22.f,
        sf::Color(220, 225, 235), 11);

    drawText("Map grid:", 65.f, 300.f, sf::Color(150, 210, 255), 14);
    std::istringstream gridStream(fGame.getMap().getGridDisplay());
    y = 324.f;
    while (std::getline(gridStream, line))
    {
        drawText(line, 70.f, y, sf::Color(220, 225, 235), 12);
        y += 17.f;
    }

    drawText("Tech tree (BTree):", 430.f, 130.f, sf::Color(150, 210, 255), 14);
    std::istringstream techStream(fGame.getMap().getTechTreeDisplay());
    y = 154.f;
    while (std::getline(techStream, line))
    {
        drawText(line, 435.f, y, sf::Color(220, 225, 235), 12);
        y += 18.f;
    }

    drawText("Buildings (polymorphism + Visitor):", 430.f, 280.f,
        sf::Color(150, 210, 255), 14);
    std::istringstream buildingStream(fGame.getBuildingReport());
    y = 304.f;
    while (std::getline(buildingStream, line) && y < 565.f)
    {
        drawText(line, 435.f, y, sf::Color(220, 225, 235), 11);
        y += 16.f;
    }

    drawText("Queue: pending waves = " +
        std::to_string(fGame.getWaveManager().getRemainingWaveCount()) +
        "   |   Stack: undo entries = " +
        std::to_string(fGame.getCommandManager().getHistorySize()),
        430.f, 580.f, sf::Color(255, 205, 115), 13);
}

void GameUI::drawGrid()
{
    const Map& map = fGame.getMap();
    for (int y = 0; y < Map::HEIGHT; ++y)
    {
        for (int x = 0; x < Map::WIDTH; ++x)
        {
            const int tx = GRID_OFFSET_X + x * (TILE_SIZE + TILE_GAP);
            const int ty = GRID_OFFSET_Y + y * (TILE_SIZE + TILE_GAP);
            const char kind = map.getTile({ x, y });

            sf::RectangleShape tile({ static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE) });
            tile.setPosition({ static_cast<float>(tx), static_cast<float>(ty) });
            tile.setFillColor(tileColor(kind));
            tile.setOutlineColor(sf::Color(55, 62, 78));
            tile.setOutlineThickness(1.f);
            fWindow.draw(tile);

            if (kind == 'C')
            {
                sf::CircleShape ore(7.f);
                ore.setPosition({ static_cast<float>(tx + 11), static_cast<float>(ty + 11) });
                ore.setFillColor(sf::Color(220, 120, 55));
                fWindow.draw(ore);
                drawText("Cu", static_cast<float>(tx + 25), static_cast<float>(ty + 25), sf::Color(245, 175, 110), 10);
            }
            else if (kind == 'K')
            {
                sf::CircleShape ore(8.f);
                ore.setPosition({ static_cast<float>(tx + 10), static_cast<float>(ty + 10) });
                ore.setFillColor(sf::Color(23, 25, 30));
                ore.setOutlineColor(sf::Color(125, 130, 145));
                ore.setOutlineThickness(1.f);
                fWindow.draw(ore);
                drawText("Coal", static_cast<float>(tx + 22), static_cast<float>(ty + 30), sf::Color(190, 195, 205), 9);
            }
            else if (kind == '#')
            {
                sf::RectangleShape core({ static_cast<float>(TILE_SIZE - 8), static_cast<float>(TILE_SIZE - 8) });
                core.setPosition({ static_cast<float>(tx + 4), static_cast<float>(ty + 4) });
                core.setFillColor(sf::Color(40, 130, 195));
                core.setOutlineColor(fWaveImpactTicks > 0 ? sf::Color(255, 65, 55) : sf::Color(130, 230, 255));
                core.setOutlineThickness(3.f);
                fWindow.draw(core);
                drawText("CORE", static_cast<float>(tx + 7), static_cast<float>(ty + 19), sf::Color::White, 13);
                if (fWaveImpactTicks > 0)
                {
                    sf::CircleShape impact(static_cast<float>(TILE_SIZE / 2 - 2));
                    impact.setPosition({ static_cast<float>(tx + 2), static_cast<float>(ty + 2) });
                    impact.setFillColor(sf::Color::Transparent);
                    impact.setOutlineColor(sf::Color(255, 80, 55));
                    impact.setOutlineThickness(3.f);
                    fWindow.draw(impact);
                }
            }
            else if (kind == 'S')
            {
                sf::RectangleShape spawn({ static_cast<float>(TILE_SIZE - 8), static_cast<float>(TILE_SIZE - 8) });
                spawn.setPosition({ static_cast<float>(tx + 4), static_cast<float>(ty + 4) });
                spawn.setFillColor(sf::Color(68, 35, 44));
                spawn.setOutlineColor(sf::Color(235, 105, 115));
                spawn.setOutlineThickness(2.f);
                fWindow.draw(spawn);
                drawText("SPAWN", static_cast<float>(tx + 5), static_cast<float>(ty + 22), sf::Color(255, 155, 165), 11);
            }
        }
    }
}

void GameUI::drawBuildings()
{
    const List<Building*>& buildings = fGame.getMap().getBuildings();
    for (auto it = buildings.getIteratorFront(); it != it.end(); ++it)
    {
        Building* building = *it;
        const Position pos = building->getPos();
        const int tx = GRID_OFFSET_X + pos.x * (TILE_SIZE + TILE_GAP);
        const int ty = GRID_OFFSET_Y + pos.y * (TILE_SIZE + TILE_GAP);
        sf::Color fill(70, 115, 165);
        sf::Color outline(180, 215, 250);
        std::string stat;
        if (building->getType() == "Drill")
        {
            const Drill* drill = static_cast<const Drill*>(building);
            fill = drill->getTargetOre() == "Copper" ? sf::Color(205, 112, 52) : sf::Color(110, 115, 125);
            outline = sf::Color(245, 200, 150);
            stat = "+" + std::to_string(drill->getMineRate()) +
                (drill->getTargetOre() == "Copper" ? " Cu/" : " Co/") +
                std::to_string(GameConfig::ProductionIntervalMilliseconds / 1000) + "s";
        }
        else if (building->getType() == "Generator")
        {
            const Generator* generator = static_cast<const Generator*>(building);
            fill = sf::Color(190, 145, 35);
            outline = sf::Color(255, 230, 120);
            stat = "+" + std::to_string(generator->getPowerOut()) + " P/" +
                std::to_string(GameConfig::ProductionIntervalMilliseconds / 1000) + "s";
        }
        else if (building->getType() == "Turret")
        {
            const Turret* turret = static_cast<const Turret*>(building);
            stat = std::to_string(turret->getAttackDmg()) + "D R" + std::to_string(turret->getRangeTiles());
        }

        sf::RectangleShape block({ static_cast<float>(TILE_SIZE - 6), static_cast<float>(TILE_SIZE - 6) });
        block.setPosition({ static_cast<float>(tx + 3), static_cast<float>(ty + 3) });
        block.setFillColor(fill);
        block.setOutlineColor(outline);
        block.setOutlineThickness(1.f);
        fWindow.draw(block);
        drawText(std::string(1, building->getSymbol()), static_cast<float>(tx + 18), static_cast<float>(ty + 6),
            sf::Color::White, 23);
        drawText("Lv" + std::to_string(building->getLevel()), static_cast<float>(tx + 4), static_cast<float>(ty + 3),
            sf::Color::White, 9);

        sf::RectangleShape hpBack({ static_cast<float>(TILE_SIZE - 14), 4.f });
        hpBack.setPosition({ static_cast<float>(tx + 7), static_cast<float>(ty + 36) });
        hpBack.setFillColor(sf::Color(55, 35, 38));
        fWindow.draw(hpBack);
        sf::RectangleShape hpBar({ static_cast<float>(TILE_SIZE - 14) * building->getHP() /
            std::max(1, building->getMaxHP()), 4.f });
        hpBar.setPosition(hpBack.getPosition());
        hpBar.setFillColor(sf::Color(220, 100, 95));
        fWindow.draw(hpBar);
        drawText(stat, static_cast<float>(tx + 4), static_cast<float>(ty + 40), sf::Color(255, 245, 205), 8);
    }
}

void GameUI::drawBattle()
{
    if (fGame.isGameOver()) return;

    SpawnEvent preview;
    if (fWaveActive)
    {
        preview.waveNumber = fWaveWaveNumber;
        preview.enemyName = fWaveEnemyName;
        preview.enemyCount = static_cast<int>(fEnemyHealth.size());
        preview.enemyHP = fEnemyMaxHP;
    }
    else if (fGame.getWaveManager().hasNextWave())
    {
        preview = fGame.getWaveManager().peekNextWave();
    }
    else return;

    const Map& map = fGame.getMap();
    if (fWaveActive)
    {
        const Position corePosition = map.getCorePosition();
        const float bx = static_cast<float>(GRID_OFFSET_X + corePosition.x * (TILE_SIZE + TILE_GAP));
        const float by = static_cast<float>(GRID_OFFSET_Y + corePosition.y * (TILE_SIZE + TILE_GAP) - 7);
        sf::RectangleShape back({ static_cast<float>(TILE_SIZE), 6.f });
        back.setPosition({ bx, by });
        back.setFillColor(sf::Color(70, 35, 40));
        fWindow.draw(back);
        sf::RectangleShape bar({ static_cast<float>(TILE_SIZE) * fWaveVisualCoreHP /
            std::max(1, map.getMaxCoreHP()), 6.f });
        bar.setPosition({ bx, by });
        bar.setFillColor(sf::Color(95, 220, 125));
        fWindow.draw(bar);
    }

    Position routeStart = map.getEnemySpawnPosition();
    if (fWaveActive)
    {
        for (size_t i = 0; i < fEnemyHealth.size(); ++i)
        {
            if (fEnemySpawned[i] && fEnemyVisualHealth[i] > 0)
            {
                routeStart = { static_cast<int>(std::lround(fEnemyGridX[i])),
                    static_cast<int>(std::lround(fEnemyGridY[i])) };
                break;
            }
        }
    }
    std::vector<Position> route = map.findPath(routeStart, map.getCorePosition());
    if (route.empty()) route = map.findPath(routeStart, map.getCorePosition(), true);
    for (size_t i = 1; i < route.size(); ++i)
    {
        const sf::Vector2f from = gridToPixel(route[i - 1].x, route[i - 1].y);
        const sf::Vector2f to = gridToPixel(route[i].x, route[i].y);
        drawLine(fWindow, from, to, sf::Color(115, 90, 92, 170), 3.f);
    }

    const int enemyCount = fWaveActive ? static_cast<int>(fEnemyHealth.size()) : preview.enemyCount;
    const Position spawn = map.getEnemySpawnPosition();
    for (int i = 0; i < enemyCount; ++i)
    {
        if (fWaveActive && (!fEnemySpawned[i] || fEnemyVisualHealth[i] <= 0)) continue;
        const int hp = fWaveActive ? fEnemyVisualHealth[i] : preview.enemyHP;
        const sf::Vector2f center = gridToPixel(fWaveActive ? fEnemyGridX[i] : spawn.x,
            fWaveActive ? fEnemyGridY[i] : spawn.y, i, enemyCount);
        sf::CircleShape enemy(14.f);
        enemy.setOrigin({ 14.f, 14.f });
        enemy.setPosition(center);
        enemy.setFillColor(hp * 3 < preview.enemyHP ? sf::Color(150, 65, 60) : sf::Color(190, 75, 70));
        enemy.setOutlineColor(sf::Color(255, 170, 135));
        enemy.setOutlineThickness(2.f);
        fWindow.draw(enemy);

        sf::RectangleShape hpBack({ 30.f, 4.f });
        hpBack.setPosition({ center.x - 15.f, center.y - 22.f });
        hpBack.setFillColor(sf::Color(65, 35, 35));
        fWindow.draw(hpBack);
        sf::RectangleShape hpBar({ std::max(1.f, 30.f * hp / std::max(1, preview.enemyHP)), 4.f });
        hpBar.setPosition(hpBack.getPosition());
        hpBar.setFillColor(sf::Color(100, 225, 105));
        fWindow.draw(hpBar);
    }

    if (fWaveActive && fWaveBeamTicks > 0)
    {
        for (size_t i = 0; i < fWaveBeamTargets.size(); ++i)
        {
            const int target = fWaveBeamTargets[i];
            if (target < 0 || target >= enemyCount) continue;
            const sf::Vector2f from = gridToPixel(fWaveBeamOrigins[i].x, fWaveBeamOrigins[i].y);
            const sf::Vector2f to = gridToPixel(fEnemyGridX[target], fEnemyGridY[target], target, enemyCount);
            drawLine(fWindow, from, to, sf::Color(40, 125, 180, 150), 8.f);
            drawLine(fWindow, from, to, sf::Color(90, 220, 255), 3.f);
        }
    }

    if (fWaveActive)
    {
        drawText("TURRET SHOTS: " + std::to_string(fWaveNextShot) + "  |  DESTROYED: " +
            std::to_string(fWaveDefeatedCount), 320.f, 78.f, sf::Color(255, 150, 125), 13);
    }
    else
    {
        drawText("NEXT WAVE " + std::to_string(preview.waveNumber) + "  " +
            std::to_string(preview.enemyCount) + " x " + preview.enemyName,
            326.f, 78.f, sf::Color(255, 130, 120), 13);
    }
}

void GameUI::drawSidePanel()
{
    const ResourceStore& resources = fGame.getResources();
    const WaveManager& waves = fGame.getWaveManager();
    sf::RectangleShape panel({ 310.f, 130.f });
    panel.setPosition({ static_cast<float>(PANEL_X), 36.f });
    panel.setFillColor(Panel);
    panel.setOutlineColor(sf::Color(70, 80, 100));
    panel.setOutlineThickness(1.f);
    fWindow.draw(panel);
    drawText(GameUIConfig::ResourcePanelLabel, PANEL_X + 14.f, 44.f, sf::Color(160, 175, 200), 14);
    drawText(std::string(GameUIConfig::CopperLabel) + ": " + std::to_string(resources.get(GameUIConfig::CopperLabel)), PANEL_X + 14.f, 70.f,
        sf::Color(240, 145, 75), 18);
    drawText(std::string(GameUIConfig::CoalLabel) + ":   " + std::to_string(resources.get(GameUIConfig::CoalLabel)), PANEL_X + 14.f, 96.f,
        sf::Color(175, 180, 195), 18);
    drawText(std::string(GameUIConfig::PowerLabel) + ":  " + std::to_string(resources.get(GameUIConfig::PowerLabel)), PANEL_X + 14.f, 122.f,
        sf::Color(255, 225, 85), 18);
    if (waves.hasNextWave())
    {
        const SpawnEvent& event = waves.peekNextWave();
        drawText("Next: " + std::to_string(event.enemyCount) + " x " + event.enemyName,
            PANEL_X + 14.f, 146.f, sf::Color(255, 120, 120), 13);
    }

    const bool generatorUnlocked = fGame.getMap().isTechUnlocked("Generator");
    const bool turretUnlocked = fGame.getMap().isTechUnlocked("Turret");
    drawButton(PANEL_X, 180, 310, 42,
        std::string(GameUIConfig::DrillButtonLabel) + " (" + std::to_string(BuildingConfig::DrillCost) + " " + GameUIConfig::CopperLabel + ")",
        sf::Color(65, 85, 110), fSelectedTool == ToolMode::BuildDrill);
    drawButton(PANEL_X, 232, 310, 42,
        generatorUnlocked ? std::string(GameUIConfig::GeneratorButtonLabel) + " (" + std::to_string(BuildingConfig::GeneratorCost) + " " + GameUIConfig::CopperLabel + ")" :
            std::string(GameUIConfig::GeneratorUnlockLabel) + " (" + std::to_string(MapConfig::GeneratorUnlockCost) + " " + GameUIConfig::CopperLabel + ")",
        sf::Color(95, 75, 45), fSelectedTool == ToolMode::BuildGenerator);
    drawButton(PANEL_X, 284, 310, 42,
        turretUnlocked ? std::string(GameUIConfig::TurretButtonLabel) + " (" + std::to_string(BuildingConfig::TurretCost) + " " + GameUIConfig::CopperLabel + ")" :
            std::string(GameUIConfig::TurretUnlockLabel) + " (" + std::to_string(MapConfig::TurretUnlockCost) + " " + GameUIConfig::CopperLabel + ")",
        sf::Color(95, 75, 45), fSelectedTool == ToolMode::BuildTurret);
    drawButton(PANEL_X, 336, 310, 42,
        std::string(GameUIConfig::UpgradeButtonLabel) + " (" + std::to_string(BuildingConfig::UpgradeCostPerLevel) + " x Lv)",
        sf::Color(65, 85, 110), fSelectedTool == ToolMode::Upgrade);
    drawButton(PANEL_X, 404, 310, 42,
        "Undo Build [Stack: " + std::to_string(fGame.getCommandManager().getHistorySize()) + "]",
        sf::Color(85, 65, 95), false, fGame.getCommandManager().canUndo());
    drawButton(PANEL_X, 456, 310, 48,
        fGame.isGameOver() ? "New Game" : (fPaused ? "RESUME SIMULATION" : "PAUSE SIMULATION"),
        fGame.isGameOver() ? sf::Color(55, 130, 90) : sf::Color(45, 130, 85));
    drawButton(PANEL_X, 518, 150, 40, "Save Game", sf::Color(55, 70, 95));
    drawButton(PANEL_X + 160, 518, 150, 40, "Load Game", sf::Color(55, 70, 95));

    drawText("Every " + std::to_string(GameConfig::ProductionIntervalMilliseconds / 1000) +
        "s: " + GameUIConfig::ProductionLabel + " " + std::to_string(BuildingConfig::GeneratorCoalPerInterval) +
        " " + GameUIConfig::CoalLabel + " -> " + std::to_string(BuildingConfig::GeneratorPowerPerInterval) + " " + GameUIConfig::PowerLabel + ".",
        PANEL_X, 570.f, sf::Color(200, 205, 220), 10);
    drawText(std::string(GameUIConfig::TurretStatsLabel) + " " + std::to_string(BuildingConfig::TurretDamagePerShot) + " dmg; " +
        std::to_string(BuildingConfig::TurretPowerPerShot) + "P + " +
        std::to_string(BuildingConfig::TurretCopperPerShot) + "C/shot.",
        PANEL_X, 588.f, sf::Color(200, 205, 220), 11);
    drawText(std::string(GameUIConfig::TurretRangeLabel) + " " + std::to_string(BuildingConfig::TurretRangeTilesPerLevel) +
        " block per turret level.", PANEL_X, 606.f, sf::Color(200, 205, 220), 11);
    drawText("Upgrades improve mining, generation and turret damage/range.",
        PANEL_X, 624.f, sf::Color(200, 205, 220), 10);
}

void GameUI::run()
{
    const float tickSeconds = GameUIConfig::AnimationTickMilliseconds / 1000.f;
    while (fWindow.isOpen())
    {
        while (const std::optional event = fWindow.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                fWindow.close();
            }
            else if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (click->button == sf::Mouse::Button::Left)
                    handleMouseClick(click->position.x, click->position.y);
            }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::F1)
                    fShowDiagnostics = !fShowDiagnostics;
            }
        }

        const float elapsed = fFrameClock.restart().asSeconds();
        if (!fPaused)
        {
            fTickAccumulator += elapsed;
            while (fTickAccumulator >= tickSeconds)
            {
                simulateRealtimeStep();
                fTickAccumulator -= tickSeconds;
            }
        }
        else
        {
            fTickAccumulator = 0.f;
        }

        render();
    }
}
