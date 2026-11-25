#include "CommandManager.h"

CommandManager::CommandManager() : fUndoStack()
{
}

void CommandManager::recordBuild(Position aPos, const std::string& aType, int aCopperCost)
{
    fUndoStack.push(BuildCommand(aPos, aType, aCopperCost));
}

bool CommandManager::canUndo() const
{
    return !fUndoStack.isEmpty();
}

BuildCommand CommandManager::popLastCommand()
{
    return fUndoStack.pop();
}

const BuildCommand& CommandManager::getTopCommand() const
{
    return fUndoStack.top();
}

void CommandManager::clear()
{
    while (!fUndoStack.isEmpty())
    {
        fUndoStack.pop();
    }
}

int CommandManager::getHistorySize() const
{
    return fUndoStack.size();
}
