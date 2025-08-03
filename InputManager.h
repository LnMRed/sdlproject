#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <SDL3/SDL.h>
#include <vector>
#include "entity.h"

class Game;

class InputManager {
public:
    InputManager(Game* game);
    void handleEvents();
    Entity* getPlayer() { return player_; }
    
    enum class EditMode { TORSO, APPENDAGE, HANDS_FEET };

    EditMode currentMode_;
    Shape currentShape_;
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

    // Getters for input state
    bool getInventoryOpen() const { return inventoryOpen_; }
    bool getPlacingNode() const { return placingNode_; }
    bool getRemovingNode() const { return removingNode_; }
    bool getShapeSelectedForAppendage() const { return shapeSelectedForAppendage_; }
    bool getMovingLeft() const { return movingLeft_; }
    bool getMovingRight() const { return movingRight_; }
    bool getJumpRequested() const { return jumpRequested_; };
    void clearJumpRequested() { jumpRequested_ = false; }
    bool getIsRotating() const { return isRotating_; }
    float getMouseX() const { return mouseX_; }
    float getMouseY() const { return mouseY_; }
    Entity* getDraggedAppendage() const { return draggedAppendage_; }

    EditMode getCurrentMode() const { return currentMode_; }
    Shape getCurrentShape() const { return currentShape_; }
    const std::vector<ShapeButton>& getShapeButtons() const { return shapeButtons_; }
    const std::vector<EditModeButton>& getEditModeButtons() const { return editModeButtons_; }
    const ShapeButton& getAddNodeButton() const { return addNodeBtn_; }
    const ShapeButton& getRemoveNodeButton() const { return removeNodeBtn_; }

private:
    Game* game_;
    Entity* player_; // Removed duplicate 'player'
    bool pressedTab_;
    bool pressedSpace_;
    bool inventoryOpen_;
    bool shapeSelectedForAppendage_;
    bool movingLeft_;
    bool movingRight_;
    bool jumpRequested_;

    bool placingNode_;
    bool removingNode_;
    bool isRotating_;
    float mouseX_, mouseY_;
    float dragStartX_, dragStartY_;
    float initialOffsetX_, initialOffsetY_;
    float initialRotation_;
    Entity* draggedAppendage_;
    
    std::vector<ShapeButton> shapeButtons_;
    std::vector<EditModeButton> editModeButtons_;
    ShapeButton addNodeBtn_;
    ShapeButton removeNodeBtn_;

    void handleQuitEvent();
    void handleKeyDownEvent(const SDL_KeyboardEvent& key);
    void handleKeyUpEvent(const SDL_KeyboardEvent& key);
    void handleMouseButtonDown(const SDL_MouseButtonEvent& button);
    void handleMouseButtonUp(const SDL_MouseButtonEvent& button);
    void handleMouseMotion(const SDL_MouseMotionEvent& motion);
    bool handleButtonClick(float x, float y);
    void setHandsGrabbing(Entity* entity, bool grabbing);
};

#endif // INPUT_MANAGER_H