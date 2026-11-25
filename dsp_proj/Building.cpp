#include "Building.h"
#include "BuildingVisitor.h"

// ============================================================================
// Building Base Class Implementation
// ============================================================================
Building::Building(const std::string& aName, int aCost, int aMaxHP, Position aPos)
    : fName(aName), fCost(aCost), fHP(aMaxHP), fMaxHP(aMaxHP), fLevel(1), fPos(aPos)
{
}

const std::string& Building::getName() const { return fName; }
int Building::getCost() const { return fCost; }
int Building::getHP() const { return fHP; }
int Building::getMaxHP() const { return fMaxHP; }
int Building::getLevel() const { return fLevel; }
Position Building::getPos() const { return fPos; }

void Building::damaged(int aAmount)
{
    if (aAmount > 0)
    {
        fHP -= aAmount;
        if (fHP < 0)
        {
            fHP = 0;
        }
    }
}

void Building::repair(int aAmount)
{
    if (aAmount > 0)
    {
        fHP += aAmount;
        if (fHP > fMaxHP)
        {
            fHP = fMaxHP;
        }
    }
}

bool Building::isDestroyed() const
{
    return fHP <= 0;
}

void Building::upgrade()
{
    fLevel += 1;
    fMaxHP += BuildingConfig::BaseUpgradeHitPoints;
    fHP = fMaxHP;
}

// ============================================================================
// Drill Implementation
// ============================================================================
Drill::Drill(Position aPos, const std::string& aTargetOre)
    : Building(aTargetOre + " Drill", BuildingConfig::DrillCost,
        BuildingConfig::DrillHitPoints, aPos),
    fMineRate(BuildingConfig::DrillMiningPerInterval), fTargetOre(aTargetOre)
{
}

int Drill::getMineRate() const { return fMineRate; }
const std::string& Drill::getTargetOre() const { return fTargetOre; }

void Drill::upgrade()
{
    Building::upgrade();
    fMineRate += BuildingConfig::DrillMiningUpgrade;
}

char Drill::getSymbol() const { return 'D'; }
std::string Drill::getType() const { return "Drill"; }

void Drill::accept(BuildingVisitor& aVisitor)
{
    aVisitor.visit(*this);
}

// ============================================================================
// Generator Implementation
// ============================================================================
Generator::Generator(Position aPos)
    : Building(BuildingConfig::GeneratorName, BuildingConfig::GeneratorCost,
        BuildingConfig::GeneratorHitPoints, aPos),
    fCoalConsume(BuildingConfig::GeneratorCoalPerInterval),
    fPowerOut(BuildingConfig::GeneratorPowerPerInterval)
{
}

int Generator::getCoalConsume() const { return fCoalConsume; }
int Generator::getPowerOut() const { return fPowerOut; }

void Generator::upgrade()
{
    Building::upgrade();
    fPowerOut += BuildingConfig::GeneratorPowerUpgrade;
}

char Generator::getSymbol() const { return 'G'; }
std::string Generator::getType() const { return "Generator"; }

void Generator::accept(BuildingVisitor& aVisitor)
{
    aVisitor.visit(*this);
}

// ============================================================================
// Turret Implementation
// ============================================================================
Turret::Turret(Position aPos)
    : Building(BuildingConfig::TurretName, BuildingConfig::TurretCost,
        BuildingConfig::TurretHitPoints, aPos),
    fAttackDmg(BuildingConfig::TurretDamagePerShot),
    fPowerReq(BuildingConfig::TurretPowerPerShot),
    fCopperAmmoReq(BuildingConfig::TurretCopperPerShot)
{
}

int Turret::getAttackDmg() const { return fAttackDmg; }
int Turret::getRangeTiles() const { return fLevel * BuildingConfig::TurretRangeTilesPerLevel; }
int Turret::getPowerReq() const { return fPowerReq; }
int Turret::getCopperAmmoReq() const { return fCopperAmmoReq; }

void Turret::upgrade()
{
    Building::upgrade();
    fAttackDmg += BuildingConfig::TurretDamageUpgrade;
}

char Turret::getSymbol() const { return 'T'; }
std::string Turret::getType() const { return "Turret"; }

void Turret::accept(BuildingVisitor& aVisitor)
{
    aVisitor.visit(*this);
}
