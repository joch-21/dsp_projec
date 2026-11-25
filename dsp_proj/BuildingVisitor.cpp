#include "BuildingVisitor.h"
#include "Building.h"
#include "Game.h"
#include "ResourceStore.h"
#include <sstream>

// ============================================================================
// BuildingReportVisitor Implementation
// ============================================================================
BuildingReportVisitor::BuildingReportVisitor() : fReport(""), fCount(0)
{
}

void BuildingReportVisitor::clear()
{
    fReport = "";
    fCount = 0;
}

const std::string& BuildingReportVisitor::getReport() const
{
    return fReport;
}

void BuildingReportVisitor::visit(Drill& aDrill)
{
    ++fCount;
    std::ostringstream oss;
    oss << fCount << ". [" << aDrill.getType() << "] " << aDrill.getName()
        << " (Lv." << aDrill.getLevel() << ") at (" << aDrill.getPos().x << "," << aDrill.getPos().y << ")"
        << " | HP: " << aDrill.getHP() << "/" << aDrill.getMaxHP()
        << " | Mines +" << aDrill.getMineRate() << " " << aDrill.getTargetOre() << "/"
        << GameConfig::ProductionIntervalMilliseconds / 1000 << "s\n";
    fReport += oss.str();
}

void BuildingReportVisitor::visit(Generator& aGenerator)
{
    ++fCount;
    std::ostringstream oss;
    oss << fCount << ". [" << aGenerator.getType() << "] " << aGenerator.getName()
        << " (Lv." << aGenerator.getLevel() << ") at (" << aGenerator.getPos().x << "," << aGenerator.getPos().y << ")"
        << " | HP: " << aGenerator.getHP() << "/" << aGenerator.getMaxHP()
        << " | Burns " << aGenerator.getCoalConsume() << " Coal -> +" << aGenerator.getPowerOut()
        << " Power/" << GameConfig::ProductionIntervalMilliseconds / 1000 << "s\n";
    fReport += oss.str();
}

void BuildingReportVisitor::visit(Turret& aTurret)
{
    ++fCount;
    std::ostringstream oss;
    oss << fCount << ". [" << aTurret.getType() << "] " << aTurret.getName()
        << " (Lv." << aTurret.getLevel() << ") at (" << aTurret.getPos().x << "," << aTurret.getPos().y << ")"
        << " | HP: " << aTurret.getHP() << "/" << aTurret.getMaxHP()
        << " | DMG: " << aTurret.getAttackDmg()
        << " (Needs " << aTurret.getPowerReq() << " Power + " << aTurret.getCopperAmmoReq() << " Copper)\n";
    fReport += oss.str();
}

// ============================================================================
// ProductionSummaryVisitor Implementation
// ============================================================================
ProductionSummaryVisitor::ProductionSummaryVisitor(ResourceStore& aResources)
    : fResources(aResources),
    fCopperMined(0),
    fCoalMined(0),
    fPowerGenerated(0),
    fInactiveGenerators(0)
{
}

void ProductionSummaryVisitor::resetTotals()
{
    fCopperMined = 0;
    fCoalMined = 0;
    fPowerGenerated = 0;
    fInactiveGenerators = 0;
}

int ProductionSummaryVisitor::getCopperMined() const { return fCopperMined; }
int ProductionSummaryVisitor::getCoalMined() const { return fCoalMined; }
int ProductionSummaryVisitor::getPowerGenerated() const { return fPowerGenerated; }
int ProductionSummaryVisitor::getInactiveGenerators() const { return fInactiveGenerators; }

void ProductionSummaryVisitor::visit(Drill& aDrill)
{
    if (aDrill.isDestroyed())
    {
        return;
    }

    int mined = aDrill.getMineRate();
    fResources.add(aDrill.getTargetOre(), mined);

    if (aDrill.getTargetOre() == "Copper")
    {
        fCopperMined += mined;
    }
    else if (aDrill.getTargetOre() == "Coal")
    {
        fCoalMined += mined;
    }
}

void ProductionSummaryVisitor::visit(Generator& aGenerator)
{
    if (aGenerator.isDestroyed())
    {
        return;
    }

    // Combination Mechanic Step 1: Burn Coal to produce Power
    if (fResources.spend("Coal", aGenerator.getCoalConsume()))
    {
        int power = aGenerator.getPowerOut();
        fResources.add("Power", power);
        fPowerGenerated += power;
    }
    else
    {
        ++fInactiveGenerators;
    }
}

void ProductionSummaryVisitor::visit(Turret& aTurret)
{
    // Combat is real-time and handled by GameUI's live targeting loop.
    (void)aTurret;
}
