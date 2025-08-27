/*
g++ -Wall -Wextra -g3 -Ic:/Users/melle/Desktop/sdlvoorjari/project/include -Lc:/Users/melle/Desktop/sdlvoorjari/project/lib -LC:/msys64/mingw64/lib c:/Users/melle/Desktop/sdlvoorjari/main.cpp c:/Users/melle/Desktop/sdlvoorjari/entity.cpp c:/Users/melle/Desktop/sdlvoorjari/game.cpp c:/Users/melle/Desktop/sdlvoorjari/InputManager.cpp c:/Users/melle/Desktop/sdlvoorjari/Renderer.cpp -o c:/Users/melle/Desktop/sdlvoorjari/output/main.exe -lmingw32 -lSDL3
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