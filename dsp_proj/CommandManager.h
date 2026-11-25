#pragma once
#include <string>
#include "Stack.h"
#include "Building.h"

struct BuildCommand
{
    Position pos;
    std::string buildingType;
    int copperRefund;

    BuildCommand() : pos{ 0, 0 }, buildingType(""), copperRefund(0) {}
    BuildCommand(Position aPos, const std::string& aType, int aRefund)
        : pos(aPos), buildingType(aType), copperRefund(aRefund)
    {
    }
};

class CommandManager
{
private:
    Stack<BuildCommand> fUndoStack;

public:
    CommandManager();

    // Push a newly placed building command onto the LIFO stack
    void recordBuild(Position aPos, const std::string& aType, int aCopperCost);

    // Check if an undo action is available
    bool canUndo() const;

    // Pop the most recent build command from the top of the stack
    BuildCommand popLastCommand();
    const BuildCommand& getTopCommand() const;

    // Clear command history (e.g., on New Game or Load Game)
    void clear();

    int getHistorySize() const;
};
