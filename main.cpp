/*
g++ -Wall -Wextra -g3 -Ic:/Users/melle/Desktop/sdlvoorjari/project/include -Lc:/Users/melle/Desktop/sdlvoorjari/project/lib c:/Users/melle/Desktop/sdlvoorjari/main.cpp c:/Users/melle/Desktop/sdlvoorjari/entity.cpp c:/Users/melle/Desktop/sdlvoorjari/game.cpp -o c:/Users/melle/Desktop/sdlvoorjari/output/main.exe -lSDL3
*/

#include "game.h"

int main() {
    Game game;
    if (!game.init()) {
        return 1;
    }
    game.run();
    return 0;
}