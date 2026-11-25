#include "Game.h"
#include "GameUI.h"

int main()
{
    Game game("savegame.txt");
    GameUI ui(game);
    ui.run();
    return 0;
}
