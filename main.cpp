/*
compile command kon het niet fixen met een makefile: 
    g++ -Wall -Wextra -g3 -Ic:/Users/melle/Desktop/sdlvoorjari/project/include -Lc:/Users/melle/Desktop/sdlvoorjari/project/lib c:/Users/melle/Desktop/sdlvoorjari/main.cpp -o c:/Users/melle/Desktop/sdlvoorjari/output/main.exe c:/Users/melle/Desktop/sdlvoorjari/project/lib/libSDL3.dll.a
*/
#include <SDL3/SDL.h>
#include <stdio.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL3 Test", SCREEN_WIDTH, SCREEN_HEIGHT, 1);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
