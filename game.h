#ifndef GAME_H
#define GAME_H
#include <SDL3/SDL.h>
#include <vector>
#include "entity.h"
#include "InputManager.h"
#include "Renderer.h"

class Game {
public:
    Game();
    ~Game();
    bool init();
    void run();
    void logDebug(const char* format, ...) const;

    Entity* getPlayer() { return &player_; }
    InputManager& getInputManager() { return inputManager_; }

    bool findParentNodePosition(Entity* appendage, float& nodeX, float& nodeY);
    float angleToPoint(float x1, float y1, float x2, float y2) const;
    bool addAppendageToEntity(Entity* entity, float mouseX, float mouseY, Shape shape, int& nodeIndex, Entity*& parentEntity, bool isHandOrFoot = false);

    static constexpr int SCREEN_WIDTH = 700;
    static constexpr int SCREEN_HEIGHT = 700;
    static constexpr int MAX_APPENDAGES = 20;
    static constexpr Uint32 FRAME_DELAY = 1000 / 60;
    static constexpr float MOVE_SPEED = 5.0f;
    static constexpr float GRAVITY = 0.3f;
    static constexpr float STEP_INTERVAL = 500.0f;

private:
    SDL_Window* window_;
    SDL_Renderer* sdl_renderer_;
    Renderer renderer_;
    InputManager inputManager_;
    Entity player_;
    bool debug_;
    Uint32 lastStepTime_;
    int currentStepFoot_;
    float walkCycle_;
    Uint32 lastFrameTime_;
    
    std::vector<Entity*> grabbableEntities_;
    Entity grabbableBall_; 

    Entity* getGrabbableAt(float x, float y, float tolerance);

    float distanceSquared(float x1, float y1, float x2, float y2) const;
    void update();
    void render();
    void renderUI();
    void updateWalkingAnimation(Entity* entity);
    std::vector<Entity*> getFeet(Entity* entity);
    float getLowestEntityY(Entity* entity);
    void getEntityMinMaxX(Entity* entity, float& minX, float& maxX);
    void updateHands(Entity* entity); 
};

#endif // GAME_H