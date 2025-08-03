#include "InputManager.h"
#include "game.h"
#include <cstdio>
#include <cmath>

InputManager::InputManager(Game* game)
    : game_(game),
      player_(game->getPlayer()), // Initialize player_ from game
      pressedTab_(false),
      pressedSpace_(false),
      inventoryOpen_(false),
      shapeSelectedForAppendage_(false),
      movingLeft_(false),
      movingRight_(false),
      jumpRequested_(false),
      placingNode_(false),
      removingNode_(false),
      isRotating_(false),
      mouseX_(0.0f),
      mouseY_(0.0f),
      dragStartX_(0.0f),
      dragStartY_(0.0f),
      initialOffsetX_(0.0f),
      initialOffsetY_(0.0f),
      initialRotation_(0.0f),
      draggedAppendage_(nullptr),
      currentMode_(EditMode::TORSO),
      currentShape_(Shape::TRIANGLE)
{
    shapeButtons_ = {
        {{10, 50, 40, 40}, Shape::RECTANGLE, {255, 0, 0, 255}},
        {{10, 100, 40, 40}, Shape::CIRCLE, {0, 255, 0, 255}},
        {{10, 150, 40, 40}, Shape::TRIANGLE, {0, 0, 255, 255}}
    };
    editModeButtons_ = {
        {{Game::SCREEN_WIDTH - 50, 10, 40, 40}, EditMode::TORSO, {200, 200, 200, 255}},
        {{Game::SCREEN_WIDTH - 50, 60, 40, 40}, EditMode::APPENDAGE, {150, 150, 150, 255}},
        {{Game::SCREEN_WIDTH - 50, 110, 40, 40}, EditMode::HANDS_FEET, {100, 100, 100, 255}}
    };
    addNodeBtn_ = {{10, 200, 40, 40}, Shape::RECTANGLE, {255, 255, 0, 255}};
    removeNodeBtn_ = {{10, 250, 40, 40}, Shape::RECTANGLE, {255, 0, 255, 255}};
    game_->logDebug("InputManager initialized, player_=%p\n", player_);
}

void InputManager::handleQuitEvent()
{
    SDL_Quit();
    exit(0);
}

void InputManager::handleKeyDownEvent(const SDL_KeyboardEvent& key)
{
    game_->logDebug("Key down: key=%d\n", key.key);
    if (!player_) {
        game_->logDebug("Error: player_ is null in handleKeyDownEvent\n");
        return;
    }
    if (key.key == SDLK_TAB && !pressedTab_) {
        inventoryOpen_ = !inventoryOpen_;
        pressedTab_ = true;
        game_->logDebug("Inventory toggled: %s\n", inventoryOpen_ ? "open" : "closed");
    } else if (key.key == SDLK_1 && inventoryOpen_ && currentMode_ != EditMode::HANDS_FEET) {
        game_->logDebug("Attempting to remove node at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
        removeNodeFromEntity(player_, mouseX_, mouseY_);
    } else if (key.key == SDLK_A && !inventoryOpen_) {
        movingLeft_ = true;
        movingRight_ = false;
        game_->logDebug("Moving left\n");
    } else if (key.key == SDLK_D && !inventoryOpen_) {
        movingRight_ = true;
        movingLeft_ = false;
        game_->logDebug("Moving right\n");
    }
        if (key.key == SDLK_SPACE && !inventoryOpen_ && !pressedSpace_) {
        jumpRequested_ = true;
        pressedSpace_ = true;
        game_->logDebug("Jump requested\n");
    }
}

void InputManager::handleKeyUpEvent(const SDL_KeyboardEvent& key)
{
    if(key.key == SDLK_SPACE) {
        pressedSpace_ = false;
        jumpRequested_ = false;
        game_->logDebug("Jump cancelled\n");
    } else if (key.key == SDLK_1 && inventoryOpen_) {
        game_->logDebug("Node removal cancelled\n");
    } else
    if (key.key == SDLK_TAB) {
        pressedTab_ = false;
        game_->logDebug("Tab key released\n");
    } else if (key.key == SDLK_A) {
        movingLeft_ = false;
        game_->logDebug("Stopped moving left\n");
    } else if (key.key == SDLK_D) {
        movingRight_ = false;
        game_->logDebug("Stopped moving right\n");
    }
}

void InputManager::setHandsGrabbing(Entity* entity, bool grabbing) {
    if (!entity) return;
    for (auto& app : entity->appendages) {
        if (app->isHandOrFoot && app->shapetype == TRIANGLE) {
            app->grabbing = grabbing;
        }
        setHandsGrabbing(app.get(), grabbing);
    }
}

void InputManager::handleMouseButtonDown(const SDL_MouseButtonEvent& button)
{
    mouseX_ = button.x;
    mouseY_ = button.y;
    game_->logDebug("Mouse down at x=%.2f, y=%.2f, button=%d, inventoryOpen=%d\n", mouseX_, mouseY_, button.button, inventoryOpen_);
    if (button.button == SDL_BUTTON_LEFT && !inventoryOpen_) {
    setHandsGrabbing(player_, true);
    }
    if (button.button == SDL_BUTTON_LEFT && inventoryOpen_) {
        if (!handleButtonClick(mouseX_, mouseY_)) {
            if (currentMode_ != EditMode::HANDS_FEET && placingNode_) {
                game_->logDebug("Attempting to add node at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                addNodeToEntity(player_, mouseX_, mouseY_);
                placingNode_ = false;
                game_->logDebug("Node placement attempted, placingNode reset\n");
            } else if (currentMode_ != EditMode::HANDS_FEET && removingNode_) {
                game_->logDebug("Attempting to remove node at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                removeNodeFromEntity(player_, mouseX_, mouseY_);
                removingNode_ = false;
                game_->logDebug("Node removal attempted, removingNode reset\n");
            } else if ((currentMode_ == EditMode::APPENDAGE || currentMode_ == EditMode::HANDS_FEET) && shapeSelectedForAppendage_) {
                int nodeIndex;
                Entity* parentEntity;
                bool isHandOrFoot = (currentMode_ == EditMode::HANDS_FEET);
                if (game_->addAppendageToEntity(player_, mouseX_, mouseY_, currentShape_, nodeIndex, parentEntity, isHandOrFoot)) {
                    shapeSelectedForAppendage_ = false;
                    updateAppendagePositions(player_);
                    game_->logDebug("Added %s appendage at node %d\n", isHandOrFoot ? "hand/foot" : "regular", nodeIndex);
                } else {
                    game_->logDebug("No node clicked for appendage at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                }
            } else if (currentMode_ != EditMode::HANDS_FEET) {
                draggedAppendage_ = findAppendageAtPoint(player_, mouseX_, mouseY_);
                if (draggedAppendage_) {
                    dragStartX_ = mouseX_;
                    dragStartY_ = mouseY_;
                    initialOffsetX_ = draggedAppendage_->offsetX;
                    initialOffsetY_ = draggedAppendage_->offsetY;
                    game_->logDebug("Started dragging appendage at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                } else {
                    game_->logDebug("No appendage found for dragging at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                }
            }
        }
    } else if (button.button == SDL_BUTTON_RIGHT && inventoryOpen_ && !shapeSelectedForAppendage_ && !placingNode_ && !removingNode_) {
        draggedAppendage_ = findAppendageAtPoint(player_, mouseX_, mouseY_);
        if (draggedAppendage_) {
            isRotating_ = true;
            dragStartX_ = mouseX_;
            dragStartY_ = mouseY_;
            initialRotation_ = draggedAppendage_->rotation;
            game_->logDebug("Started rotating appendage at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
        } else {
            game_->logDebug("No appendage found for rotating at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
        }
    }
}

void InputManager::handleMouseButtonUp(const SDL_MouseButtonEvent& button)
{
    if (button.button == SDL_BUTTON_LEFT && !inventoryOpen_) {
    setHandsGrabbing(player_, false);
    }
    if (button.button == SDL_BUTTON_LEFT && draggedAppendage_) {
        draggedAppendage_ = nullptr;
        game_->logDebug("Stopped dragging appendage\n");
    } else if (button.button == SDL_BUTTON_RIGHT && isRotating_) {
        isRotating_ = false;
        draggedAppendage_ = nullptr;
        game_->logDebug("Stopped rotating appendage\n");
    }
}

void InputManager::handleMouseMotion(const SDL_MouseMotionEvent& motion) {
    mouseX_ = motion.x;
    mouseY_ = motion.y;
    if (draggedAppendage_ && inventoryOpen_) {
        float nodeX = 0.0f, nodeY = 0.0f;
        if (!game_->findParentNodePosition(draggedAppendage_, nodeX, nodeY)) {
            game_->logDebug("Failed to find parent node for dragged appendage\n");
            return;
        }
        if (motion.state & SDL_BUTTON_LMASK) {
            float dx = mouseX_ - nodeX;
            float dy = mouseY_ - nodeY;
            draggedAppendage_->offsetX = dx * cos(-draggedAppendage_->rotation) - dy * sin(-draggedAppendage_->rotation);
            draggedAppendage_->offsetY = dx * sin(-draggedAppendage_->rotation) + dy * cos(-draggedAppendage_->rotation);
            updateAppendagePositions(player_);
            game_->logDebug("Dragging appendage: offsetX=%.2f, offsetY=%.2f\n", draggedAppendage_->offsetX, draggedAppendage_->offsetY);
        } else if (motion.state & SDL_BUTTON_RMASK && isRotating_) {
            float initialAngle = game_->angleToPoint(nodeX, nodeY, dragStartX_, dragStartY_);
            float newAngle = game_->angleToPoint(nodeX, nodeY, mouseX_, mouseY_);
            draggedAppendage_->rotation = initialRotation_ + (newAngle - initialAngle);
            updateAppendagePositions(player_);
            game_->logDebug("Rotating appendage: initialAngle=%.2f, newAngle=%.2f, rotation=%.2f\n",
                            initialAngle, newAngle, draggedAppendage_->rotation);
        }
    }
}

bool InputManager::handleButtonClick(float x, float y)
{
    
    game_->logDebug("Checking button click at x=%.2f, y=%.2f, mode=%d, inventoryOpen=%d\n",
                    x, y, static_cast<int>(currentMode_), inventoryOpen_);
    if (!inventoryOpen_) {
        game_->logDebug("Button click ignored: inventory not open\n");
        return false;
    }
    if (!player_) {
        game_->logDebug("Error: player_ is null in handleButtonClick\n");
        return false;
    }

    // Check shape buttons
    for (const auto& btn : shapeButtons_) {
        if (x >= btn.rect.x && x <= btn.rect.x + btn.rect.w &&
            y >= btn.rect.y && y <= btn.rect.y + btn.rect.h) {
            // In InputManager::handleButtonClick
            game_->logDebug("Shape button clicked: shape=%d, currentMode=%d, shapeSelectedForAppendage=%d\n",
                btn.shapeType, static_cast<int>(currentMode_), shapeSelectedForAppendage_);
            if (currentMode_ == EditMode::TORSO) {
                switchShape(player_, btn.shapeType);
                currentShape_ = btn.shapeType; // Restore currentShape_ update
                updateAppendagePositions(player_);
                game_->logDebug("Switched player shape to %d\n", btn.shapeType);
            } else if (currentMode_ == EditMode::APPENDAGE || currentMode_ == EditMode::HANDS_FEET) {
                currentShape_ = btn.shapeType;
                shapeSelectedForAppendage_ = true;
                game_->logDebug("Selected shape %d for appendage, shapeSelected=%d\n",
                                btn.shapeType, shapeSelectedForAppendage_);
            }
            return true;
        }
    }

    // Check node buttons
    if (currentMode_ != EditMode::HANDS_FEET) {
        if (x >= addNodeBtn_.rect.x && x <= addNodeBtn_.rect.x + addNodeBtn_.rect.w &&
            y >= addNodeBtn_.rect.y && y <= addNodeBtn_.rect.y + addNodeBtn_.rect.h) {
            placingNode_ = true;
            removingNode_ = false;
            shapeSelectedForAppendage_ = false;
            game_->logDebug("Add node button clicked, placingNode=%d\n", placingNode_);
            return true;
        }
        if (x >= removeNodeBtn_.rect.x && x <= removeNodeBtn_.rect.x + removeNodeBtn_.rect.w &&
            y >= removeNodeBtn_.rect.y && y <= removeNodeBtn_.rect.y + removeNodeBtn_.rect.h) {
            removingNode_ = true;
            placingNode_ = false;
            shapeSelectedForAppendage_ = false;
            game_->logDebug("Remove node button clicked, removingNode=%d\n", removingNode_);
            return true;
        }
    }

    // Check edit mode buttons
    for (const auto& btn : editModeButtons_) {
        if (x >= btn.rect.x && x <= btn.rect.x + btn.rect.w &&
            y >= btn.rect.y && y <= btn.rect.y + btn.rect.h) {
            currentMode_ = btn.mode;
            shapeSelectedForAppendage_ = false;
            placingNode_ = false;
            removingNode_ = false;
            game_->logDebug("Switched to edit mode %d\n", static_cast<int>(btn.mode));
            return true;
        }
    }
    return false;
}

void InputManager::handleEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                handleQuitEvent();
                break;
            case SDL_EVENT_KEY_DOWN:
                handleKeyDownEvent(event.key);
                break;
            case SDL_EVENT_KEY_UP:
                handleKeyUpEvent(event.key);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                handleMouseButtonDown(event.button);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                handleMouseButtonUp(event.button);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                handleMouseMotion(event.motion);
                break;
        }
    }
}