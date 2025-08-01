#ifndef GAME_H
#define GAME_H
#include <SDL3/SDL.h>
#include <vector>
#include "entity.h"
class Game {
public:
    Game();
    ~Game();
    bool init();
    void run();private:
    // Constants
    static constexpr int SCREEN_WIDTH = 700;
    static constexpr int SCREEN_HEIGHT = 700;
    static constexpr int MAX_APPENDAGES = 20;
    static constexpr Uint32 FRAME_DELAY = 1000 / 60; // 60 FPS
    static constexpr float MOVE_SPEED = 5.0f; // Pixels per frame
    static constexpr float GRAVITY = 0.5f; // Pixels per frame^2
    static constexpr float STEP_INTERVAL = 500.0f; // Milliseconds per step// SDL Resources
SDL_Window* window_;
SDL_Renderer* renderer_;

// Game State
Entity player_;
Entity* draggedAppendage_;
float dragStartX_, dragStartY_;
float initialOffsetX_, initialOffsetY_;
float initialRotation_;
bool isRotating_;
bool inventoryOpen_;
bool placingNode_;
bool removingNode_;
bool pressedTab_;
float mouseX_, mouseY_;
bool debug_;
// Walking state
bool movingLeft_;
bool movingRight_;
Uint32 lastStepTime_;
int currentStepFoot_;
float walkCycle_;

// UI Elements
enum class EditMode { TORSO, APPENDAGE, HANDS_FEET };
EditMode currentMode_;
Shape currentShape_;
bool shapeSelectedForAppendage_;
struct ShapeButton {
    SDL_FRect rect;
    Shape shapeType;
    SDL_Color color;
};
struct EditModeButton {
    SDL_FRect rect;
    EditMode mode;
    SDL_Color color;
};
std::vector<ShapeButton> shapeButtons_;
std::vector<EditModeButton> editModeButtons_;
ShapeButton addNodeBtn_;
ShapeButton removeNodeBtn_;

// Frame Timing
Uint32 lastFrameTime_;

// Private Methods
void handleEvents();
void handleQuitEvent();
void handleKeyDownEvent(const SDL_KeyboardEvent& key);
void handleKeyUpEvent(const SDL_KeyboardEvent& key);
void handleMouseButtonDown(const SDL_MouseButtonEvent& button);
void handleMouseButtonUp(const SDL_MouseButtonEvent& button);
void handleMouseMotion(const SDL_MouseMotionEvent& motion);
bool handleButtonClick(float x, float y);
bool findParentNodePosition(Entity* appendage, float& nodeX, float& nodeY);
void logDebug(const char* format, ...) const;
float distanceSquared(float x1, float y1, float x2, float y2) const;
float angleToPoint(float x1, float y1, float x2, float y2) const;
void update();
void render();
void renderUI();
bool addAppendageToEntity(Entity* entity, float mouseX, float mouseY, Shape shape, int& nodeIndex, Entity*& parentEntity, bool isHandOrFoot = false);
void updateWalkingAnimation(Entity* entity);
std::vector<Entity*> getFeet(Entity* entity);
float getLowestFootY(Entity* entity);};

#endif // GAME_H

