#pragma once
#include <string>
#include "Building.h"

// Forward declarations for Building hierarchy
class Drill;
class Generator;
class Turret;
class ResourceStore;

class BuildingVisitor
{
public:
    virtual ~BuildingVisitor() = default;

    // Overloaded visit methods resolved via double dispatch
    virtual void visit(Drill& aDrill) = 0;
    virtual void visit(Generator& aGenerator) = 0;
    virtual void visit(Turret& aTurret) = 0;
};
 
// Concrete Visitor 1: BuildingReportVisitor
// Collects a human-readable status summary of all visited buildings  
class BuildingReportVisitor : public BuildingVisitor
{
private:
    std::string fReport;
    int fCount;

public:
    BuildingReportVisitor();

    void clear();
    const std::string& getReport() const;

    void visit(Drill& aDrill) override;
    void visit(Generator& aGenerator) override;
    void visit(Turret& aTurret) override;
};

//concrete visitor 2
// Applies one timed resource-production interval to drills and generators
// Turrets are simulated continuously by GameUI while enemies are present 
class ProductionSummaryVisitor : public BuildingVisitor
{
private:
    ResourceStore& fResources;
    int fCopperMined;
    int fCoalMined;
    int fPowerGenerated;
    int fInactiveGenerators;

public:
    explicit ProductionSummaryVisitor(ResourceStore& aResources);

    void resetTotals();

    int getCopperMined() const;
    int getCoalMined() const;
    int getPowerGenerated() const;
    int getInactiveGenerators() const;

    void visit(Drill& aDrill) override;
    void visit(Generator& aGenerator) override;
    void visit(Turret& aTurret) override;
};
