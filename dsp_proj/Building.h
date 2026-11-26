#pragma once
#include <string>

// Building costs, names, and base stats are configured here. Gameplay, UI,
// and save/load code reference the same values through BuildingConfig.
namespace BuildingConfig
{
    inline constexpr int DrillCost = 15;
    inline constexpr int GeneratorCost = 15;
    inline constexpr int TurretCost = 30;
    inline constexpr int UpgradeCostPerLevel = 20;
    inline constexpr int BaseUpgradeHitPoints = 5;
    inline constexpr int DrillHitPoints = 25;
    inline constexpr int DrillMiningPerInterval = 10;
    inline constexpr int DrillMiningUpgrade = 5;
    inline constexpr int GeneratorHitPoints = 25;
    inline constexpr int GeneratorCoalPerInterval = 5;
    inline constexpr int GeneratorPowerPerInterval = 15;
    inline constexpr int GeneratorPowerUpgrade = 10;
    inline constexpr int TurretHitPoints = 25;
    inline constexpr int TurretDamagePerShot = 25;
    inline constexpr int TurretDamageUpgrade = 15;
    inline constexpr int TurretPowerPerShot = 5;
    inline constexpr int TurretCopperPerShot = 5;
    inline constexpr int TurretFireCooldownTicks = 8;
    inline constexpr int TurretRangeTilesPerLevel = 1;
    inline constexpr const char* GeneratorName = "Combustion Generator";
    inline constexpr const char* TurretName = "Duo Turret";
}

// Utility struct for map coordinates (follows Lab 9 Position pattern)
struct Position
{
    int x;
    int y;
};

// Forward declaration to avoid circular #include with BuildingVisitor.h
class BuildingVisitor;

class Building
{
protected:
    std::string fName;
    int fCost;
    int fHP;
    int fMaxHP;
    int fLevel;
    Position fPos;

public:
    Building(const std::string& aName, int aCost, int aMaxHP, Position aPos);
    virtual ~Building() = default;

    // Common accessors
    const std::string& getName() const;
    int getCost() const;
    int getHP() const;
    int getMaxHP() const;
    int getLevel() const;
    Position getPos() const;

    // Common combat/damage state helpers
    void damaged(int aAmount);
    void repair(int aAmount);
    bool isDestroyed() const;

    // Polymorphic operations overridden by derived classes
    virtual void upgrade();
    virtual char getSymbol() const = 0;
    virtual std::string getType() const = 0;

    // Concept 4b: Double-dispatch entry point for the Visitor Pattern
    virtual void accept(BuildingVisitor& aVisitor) = 0;
};

// Derived Class 1: Drill (Extracts Copper or Coal ore each turn) 
class Drill : public Building
{
private:
    int fMineRate;
    std::string fTargetOre; // "Copper" or "Coal"

public:
    Drill(Position aPos, const std::string& aTargetOre = "Copper");

    int getMineRate() const;
    const std::string& getTargetOre() const;

    void upgrade() override;
    char getSymbol() const override;
    std::string getType() const override;
    void accept(BuildingVisitor& aVisitor) override;
};
 
// Derived Class - Generator (Burns Coal to generate Power each turn) 
class Generator : public Building
{
private:
    int fCoalConsume;
    int fPowerOut;

public:
    Generator(Position aPos);

    int getCoalConsume() const;
    int getPowerOut() const;

    void upgrade() override;
    char getSymbol() const override;
    std::string getType() const override;
    void accept(BuildingVisitor& aVisitor) override;
};
 
// Derived Class 3: Turret (Combines Power + Copper ammo to defend against waves) 
class Turret : public Building
{
private:
    int fAttackDmg;
    int fPowerReq;
    int fCopperAmmoReq;

public:
    Turret(Position aPos);

    int getAttackDmg() const;
    int getRangeTiles() const;
    int getPowerReq() const;
    int getCopperAmmoReq() const;

    void upgrade() override;
    char getSymbol() const override;
    std::string getType() const override;
    void accept(BuildingVisitor& aVisitor) override;
};
