#include <cstdarg>
#include <cmath>
#include <cstdio>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "game.h"

Game::Game()
    : window_(nullptr), renderer_(nullptr), draggedAppendage_(nullptr),
      dragStartX_(0.0f), dragStartY_(0.0f), initialOffsetX_(0.0f), initialOffsetY_(0.0f),
      initialRotation_(0.0f), isRotating_(false), inventoryOpen_(false),
      placingNode_(false), removingNode_(false), pressedTab_(false),
      mouseX_(0.0f), mouseY_(0.0f), debug_(false), currentMode_(EditMode::TORSO), // Enable debug
      currentShape_(TRIANGLE), shapeSelectedForAppendage_(false), lastFrameTime_(0),
      movingLeft_(false), movingRight_(false), lastStepTime_(0), currentStepFoot_(0) {
    shapeButtons_ = {
        {{10, 50, 40, 40}, RECTANGLE, {255, 0, 0, 255}},
        {{10, 100, 40, 40}, CIRCLE, {0, 255, 0, 255}},
        {{10, 150, 40, 40}, TRIANGLE, {0, 0, 255, 255}}
    };
    editModeButtons_ = {
        {{SCREEN_WIDTH - 50, 10, 40, 40}, EditMode::TORSO, {200, 200, 200, 255}},
        {{SCREEN_WIDTH - 50, 60, 40, 40}, EditMode::APPENDAGE, {150, 150, 150, 255}},
        {{SCREEN_WIDTH - 50, 110, 40, 40}, EditMode::HANDS_FEET, {100, 100, 100, 255}}
    };
    addNodeBtn_ = {{10, 200, 40, 40}, RECTANGLE, {255, 255, 0, 255}};
    removeNodeBtn_ = {{10, 250, 40, 40}, RECTANGLE, {255, 0, 255, 255}};
}Game::~Game() {
    destroyEntity(&player_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}bool Game::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }window_ = SDL_CreateWindow("Game", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
if (!window_) {
    printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
    SDL_Quit();
    return false;
}

// Create renderer with default driver (chooses hardware-accelerated renderer)
renderer_ = SDL_CreateRenderer(window_, nullptr);
if (!renderer_) {
    printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window_);
    SDL_Quit();
    return false;
}

// Enable V-Sync
if (!SDL_SetRenderVSync(renderer_, true)) {
    printf("SDL_SetRenderVSync Warning: %s\n", SDL_GetError());
    // V-Sync is optional, so continue even if it fails
}

// Verify renderer properties
SDL_PropertiesID props = SDL_GetRendererProperties(renderer_);
if (props) {
    const char* driver = SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "unknown");
    bool vsync_enabled = SDL_GetBooleanProperty(props, SDL_PROP_RENDERER_VSYNC_NUMBER, false);
    printf("Renderer: %s, V-Sync: %s\n", driver, vsync_enabled ? "Enabled" : "Disabled");
}

// Initialize player entity with texture
initEntity(&player_, renderer_, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 50, 50, currentShape_, {255, 0, 0, 255}, 50, false);
player_.isCore = true;

// Set texture scale mode for better quality
if (player_.texture) {
    SDL_SetTextureScaleMode(player_.texture, SDL_SCALEMODE_LINEAR);
}
return true;}void Game::run()
{
    lastFrameTime_ = SDL_GetTicks();
    bool running = true;
    while (running) {
        handleEvents();
        update();
        render();
        Uint32 currentTime = SDL_GetTicks();
        Uint32 frameTime = currentTime - lastFrameTime_;
        if (frameTime < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }
        lastFrameTime_ = currentTime;
    }
}void Game::logDebug(const char* format, ...) const {
    if (!debug_) return;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}float Game::distanceSquared(float x1, float y1, float x2, float y2) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}float Game::angleToPoint(float x1, float y1, float x2, float y2) const {
    return atan2(y2 - y1, x2 - x1);
}bool Game::findParentNodePosition(Entity* appendage, float& nodeX, float& nodeY) {
    if (!appendage) return false;
    Entity* parent = &player_;
    for (auto& app : parent->appendages) {
        if (app.get() == appendage) {
            nodeX = parent->nodes[appendage->coreNodeIndex].x;
            nodeY = parent->nodes[appendage->coreNodeIndex].y;
            return true;
        }
        for (auto& subApp : app->appendages) {
            if (subApp.get() == appendage) {
                nodeX = app->nodes[appendage->coreNodeIndex].x;
                nodeY = app->nodes[appendage->coreNodeIndex].y;
                return true;
            }
        }
    }
    return false;
}std::vector<Entity*> Game::getFeet(Entity* entity) {
    std::vector<Entity*> feet;
    for (auto& app : entity->appendages) {
        if (app->isHandOrFoot && app->shapetype == RECTANGLE) {
            feet.push_back(app.get());
        }
        auto subFeet = getFeet(app.get());
        feet.insert(feet.end(), subFeet.begin(), subFeet.end());
    }
    // Sort by bottom Y-position
    std::sort(feet.begin(), feet.end(), [](Entity* a, Entity* b) {
        float aBottom = a->Ypos + a->height / 2.0f * cos(a->rotation);
        float bBottom = b->Ypos + b->height / 2.0f * cos(b->rotation);
        return aBottom > bBottom;
    });
    if (feet.size() > 2) feet.resize(2);
    for (size_t i = 0; i < feet.size(); ++i) {
        logDebug("Foot %zu: x=%.2f, y=%.2f, onGround=%d\n", i, feet[i]->Xpos, feet[i]->Ypos, feet[i]->onGround);
    }
    return feet;
}float Game::getLowestFootY(Entity* entity) {
    auto feet = getFeet(entity);
    float lowestY = SCREEN_HEIGHT;
    for (auto* foot : feet) {
        float hw = foot->width / 2.0f;
        float hh = foot->height / 2.0f;
        SDL_FPoint corners[4] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
        for (int i = 0; i < 4; ++i) {
            float rx = corners[i].x * cos(foot->rotation) - corners[i].y * sin(foot->rotation);
            float ry = corners[i].x * sin(foot->rotation) + corners[i].y * cos(foot->rotation);
            float absY = foot->Ypos + ry;
            lowestY = std::max(lowestY, absY);
        }
    }
    logDebug("Lowest foot Y: %.2f\n", lowestY);
    return lowestY;
}void Game::updateWalkingAnimation(Entity* entity) {
    auto feet = getFeet(entity);
    if (feet.empty() || (!movingLeft_ && !movingRight_)) {
        // Reset animation when not moving
        walkCycle_ = 0.0f;
        for (auto* foot : feet) {
            foot->rotation = 0.0f;
            foot->offsetX = 0.0f;
            foot->offsetY = 50.0f; // Default offset to keep foot below core node
        }
        updateAppendagePositions(entity);
        return;
    }// Update walk cycle based on delta time
Uint32 currentTime = SDL_GetTicks();
float dt = (currentTime - lastFrameTime_) / 1000.0f; // Delta time in seconds
const float cycleDuration = 0.5f; // Time for one full step cycle
walkCycle_ += dt / cycleDuration;
if (walkCycle_ > 1.0f) walkCycle_ -= 1.0f; // Loop the cycle

float direction = movingRight_ ? 1.0f : -1.0f;
for (size_t i = 0; i < feet.size(); ++i) {
    // Alternate phases for each foot
    float phase = (i == 0) ? walkCycle_ : (walkCycle_ + 0.5f);
    if (phase > 1.0f) phase -= 1.0f;

    auto* foot = feet[i];
    // Apply sinusoidal motion for stepping
    foot->rotation = sin(phase * 2.0f * M_PI) * 0.3f; // ~17 degrees max rotation
    foot->offsetX = cos(phase * 2.0f * M_PI) * 20.0f * direction; // Horizontal step
    foot->offsetY = 50.0f - 10.0f * sin(phase * 2.0f * M_PI); // Vertical lift

    // Snap to ground if on ground
    if (foot->onGround) {
        float groundY = static_cast<float>(SCREEN_HEIGHT);
        foot->Ypos = groundY - foot->height / 2.0f;
    }

    logDebug("Animating foot %zu: x=%.2f, y=%.2f, rotation=%.2f, offsetX=%.2f, offsetY=%.2f, onGround=%d\n",
             i, foot->Xpos, foot->Ypos, foot->rotation, foot->offsetX, foot->offsetY, foot->onGround);
}

updateAppendagePositions(entity);}bool Game::handleButtonClick(float x, float y) {
    for (const auto& btn : shapeButtons_) {
        if (x >= btn.rect.x && x <= btn.rect.x + btn.rect.w &&
            y >= btn.rect.y && y <= btn.rect.y + btn.rect.h) {
            if (currentMode_ == EditMode::TORSO) {
                switchShape(&player_, btn.shapeType);
                currentShape_ = btn.shapeType;
                logDebug("Clicked shape button: shapeType=%d (TORSO mode), player shape changed\n", btn.shapeType);
                return true;
            } else if (currentMode_ == EditMode::APPENDAGE || currentMode_ == EditMode::HANDS_FEET) {
                currentShape_ = btn.shapeType;
                shapeSelectedForAppendage_ = true;
                logDebug("Clicked shape button: shapeType=%d (%s mode), shapeSelectedForAppendage=%d\n",
                         btn.shapeType, currentMode_ == EditMode::APPENDAGE ? "APPENDAGE" : "HANDS_FEET", shapeSelectedForAppendage_);
                return true;
            }
        }
    }
    if (currentMode_ != EditMode::HANDS_FEET) {
        if (x >= addNodeBtn_.rect.x && x <= addNodeBtn_.rect.x + addNodeBtn_.rect.w &&
            y >= addNodeBtn_.rect.y && y <= addNodeBtn_.rect.y + addNodeBtn_.rect.h) {
            placingNode_ = true;
            logDebug("Clicked add node button, placingNode=%d\n", placingNode_);
            return true;
        }
        if (x >= removeNodeBtn_.rect.x && x <= removeNodeBtn_.rect.x + removeNodeBtn_.rect.w &&
            y >= removeNodeBtn_.rect.y && y <= removeNodeBtn_.rect.y + removeNodeBtn_.rect.h) {
            removingNode_ = true;
            logDebug("Clicked remove node button, removingNode=%d\n", removingNode_);
            return true;
        }
    }
    for (const auto& tab : editModeButtons_) {
        if (x >= tab.rect.x && x <= tab.rect.x + tab.rect.w &&
            y >= tab.rect.y && y <= tab.rect.y + tab.rect.h) {
            currentMode_ = tab.mode;
            shapeSelectedForAppendage_ = false;
            placingNode_ = false;
            removingNode_ = false;
            logDebug("Clicked edit mode button: mode=%d, shapeSelectedForAppendage reset\n", static_cast<int>(tab.mode));
            return true;
        }
    }
    return false;
}void Game::handleQuitEvent() {
    SDL_Quit();
    exit(0);
}void Game::handleKeyDownEvent(const SDL_KeyboardEvent& key) {
    if (key.key == SDLK_TAB && !pressedTab_) {
        inventoryOpen_ = !inventoryOpen_;
        pressedTab_ = true;
        logDebug("Inventory toggled: %s\n", inventoryOpen_ ? "open" : "closed");
    } else if (key.key == SDLK_1 && inventoryOpen_ && currentMode_ != EditMode::HANDS_FEET) {
        logDebug("Attempting to remove node at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
        removeNodeFromEntity(&player_, mouseX_, mouseY_);
    } else if (key.key == SDLK_A && !inventoryOpen_) {
        movingLeft_ = true;
        movingRight_ = false;
        logDebug("Moving left\n");
    } else if (key.key == SDLK_D && !inventoryOpen_) {
        movingRight_ = true;
        movingLeft_ = false;
        logDebug("Moving right\n");
    }
}void Game::handleKeyUpEvent(const SDL_KeyboardEvent& key) {
    if (key.key == SDLK_TAB) {
        pressedTab_ = false;
        logDebug("Tab key released\n");
    } else if (key.key == SDLK_A) {
        movingLeft_ = false;
        logDebug("Stopped moving left\n");
    } else if (key.key == SDLK_D) {
        movingRight_ = false;
        logDebug("Stopped moving right\n");
    }
}void Game::handleMouseButtonDown(const SDL_MouseButtonEvent& button) {
    mouseX_ = button.x;
    mouseY_ = button.y;
    logDebug("Mouse down at x=%.2f, y=%.2f, button=%d, inventoryOpen=%d\n", mouseX_, mouseY_, button.button, inventoryOpen_);
    if (button.button == SDL_BUTTON_LEFT && inventoryOpen_) {
        if (!handleButtonClick(mouseX_, mouseY_)) {
            if (currentMode_ != EditMode::HANDS_FEET && placingNode_) {
                logDebug("Attempting to add node at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                if (!addNodeToEntity(&player_, mouseX_, mouseY_)) {
                    logDebug("Failed to add node to any entity at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                }
                placingNode_ = false;
                logDebug("Node placement attempted, placingNode reset\n");
            } else if (currentMode_ != EditMode::HANDS_FEET && removingNode_) {
                logDebug("Attempting to remove node at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                removeNodeFromEntity(&player_, mouseX_, mouseY_);
                removingNode_ = false;
                logDebug("Node removal attempted, removingNode reset\n");
            } else if ((currentMode_ == EditMode::APPENDAGE || currentMode_ == EditMode::HANDS_FEET) && shapeSelectedForAppendage_) {
                int nodeIndex;
                Entity* parentEntity;
                bool isHandOrFoot = (currentMode_ == EditMode::HANDS_FEET);
                if (addAppendageToEntity(&player_, mouseX_, mouseY_, currentShape_, nodeIndex, parentEntity, isHandOrFoot)) {
                    shapeSelectedForAppendage_ = false;
                    updateAppendagePositions(&player_);
                    logDebug("Added %s appendage at node %d\n", isHandOrFoot ? "hand/foot" : "regular", nodeIndex);
                } else {
                    logDebug("No node clicked for appendage at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                }
            } else if (currentMode_ != EditMode::HANDS_FEET) {
                draggedAppendage_ = findAppendageAtPoint(&player_, mouseX_, mouseY_);
                if (draggedAppendage_) {
                    dragStartX_ = mouseX_;
                    dragStartY_ = mouseY_;
                    initialOffsetX_ = draggedAppendage_->offsetX;
                    initialOffsetY_ = draggedAppendage_->offsetY;
                    logDebug("Started dragging appendage at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                } else {
                    logDebug("No appendage found for dragging at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
                }
            }
        }
    } else if (button.button == SDL_BUTTON_RIGHT && inventoryOpen_ && !shapeSelectedForAppendage_ && !placingNode_ && !removingNode_ && currentMode_ != EditMode::HANDS_FEET) {
        draggedAppendage_ = findAppendageAtPoint(&player_, mouseX_, mouseY_);
        if (draggedAppendage_) {
            isRotating_ = true;
            dragStartX_ = mouseX_;
            dragStartY_ = mouseY_;
            initialRotation_ = draggedAppendage_->rotation;
            logDebug("Started rotating appendage at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
        } else {
            logDebug("No appendage found for rotating at x=%.2f, y=%.2f\n", mouseX_, mouseY_);
        }
    }
}void Game::handleMouseButtonUp(const SDL_MouseButtonEvent& button) {
    if (button.button == SDL_BUTTON_LEFT && draggedAppendage_) {
        draggedAppendage_ = nullptr;
        logDebug("Stopped dragging appendage\n");
    } else if (button.button == SDL_BUTTON_RIGHT && isRotating_) {
        isRotating_ = false;
        draggedAppendage_ = nullptr;
        logDebug("Stopped rotating appendage\n");
    }
}void Game::handleMouseMotion(const SDL_MouseMotionEvent& motion) {
    mouseX_ = motion.x;
    mouseY_ = motion.y;
    if (draggedAppendage_ && inventoryOpen_ && currentMode_ != EditMode::HANDS_FEET) {
        float nodeX = 0.0f, nodeY = 0.0f;
        if (!findParentNodePosition(draggedAppendage_, nodeX, nodeY)) {
            logDebug("Failed to find parent node for dragged appendage\n");
            return;
        }
        if (motion.state & SDL_BUTTON_LMASK) {
            float dx = mouseX_ - nodeX;
            float dy = mouseY_ - nodeY;
            draggedAppendage_->offsetX = dx * cos(-draggedAppendage_->rotation) - dy * sin(-draggedAppendage_->rotation);
            draggedAppendage_->offsetY = dx * sin(-draggedAppendage_->rotation) + dy * cos(-draggedAppendage_->rotation);
            updateAppendagePositions(&player_);
            logDebug("Dragging appendage: offsetX=%.2f, offsetY=%.2f\n", draggedAppendage_->offsetX, draggedAppendage_->offsetY);
        } else if (motion.state & SDL_BUTTON_RMASK && isRotating_) {
            float newAngle = angleToPoint(nodeX, nodeY, mouseX_, mouseY_);
            draggedAppendage_->rotation = newAngle;
            updateAppendagePositions(&player_);
            logDebug("Rotating appendage: rotation=%.2f radians\n", draggedAppendage_->rotation);
        }
    }
}void Game::handleEvents() {
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
}void Game::update() {
    // Apply gravity to core entity
    if (!inventoryOpen_) {
        player_.Yvel += GRAVITY + GRAVITY;
        player_.Ypos += player_.Yvel;
        // Check if feet hit the ground
        float lowestFootY = getLowestFootY(&player_);
        if (lowestFootY >= SCREEN_HEIGHT) {
            player_.Ypos -= (lowestFootY - SCREEN_HEIGHT);
            player_.Yvel = 0.0f;
            logDebug("Ground collision: adjusted player Ypos=%.2f, Yvel=0\n", player_.Ypos);
        }
    }// Update horizontal movement
auto feet = getFeet(&player_);
bool canMove = false;
for (auto* foot : feet) {
    if (foot->onGround) {
        canMove = true;
        break;
    }
}
if (canMove && !inventoryOpen_) {
    if (movingLeft_) {
        player_.Xvel = -MOVE_SPEED;
    } else if (movingRight_) {
        player_.Xvel = MOVE_SPEED;
    } else {
        player_.Xvel = 0.0f;
    }
} else {
    player_.Xvel = 0.0f;
}
player_.Xpos += player_.Xvel;
player_.Xpos = std::max(0.0f, std::min(player_.Xpos, static_cast<float>(SCREEN_WIDTH)));
logDebug("Updated player: x=%.2f, y=%.2f, Xvel=%.2f, Yvel=%.2f, canMove=%d\n",
         player_.Xpos, player_.Ypos, player_.Xvel, player_.Yvel, canMove);

// Animate feet before updating positions
updateWalkingAnimation(&player_);

// Update all appendage positions
updateAppendagePositions(&player_);}void Game::renderUI() {
    if (!inventoryOpen_ || !renderer_) return;std::vector<SDL_Vertex> vertices;
std::vector<int> indices;
int baseIndex = 0;

// Inventory background
SDL_Color bgColor = {100, 0, 100, 255};
SDL_FRect invBox = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
vertices.push_back({{invBox.x, invBox.y}, {bgColor.r / 255.0f, bgColor.g / 255.0f, bgColor.b / 255.0f, bgColor.a / 255.0f}});
vertices.push_back({{invBox.x + invBox.w, invBox.y}, {bgColor.r / 255.0f, bgColor.g / 255.0f, bgColor.b / 255.0f, bgColor.a / 255.0f}});
vertices.push_back({{invBox.x + invBox.w, invBox.y + invBox.h}, {bgColor.r / 255.0f, bgColor.g / 255.0f, bgColor.b / 255.0f, bgColor.a / 255.0f}});
vertices.push_back({{invBox.x, invBox.y + invBox.h}, {bgColor.r / 255.0f, bgColor.g / 255.0f, bgColor.b / 255.0f, bgColor.a / 255.0f}});
indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
baseIndex += 4;

// Shape buttons
for (const auto& btn : shapeButtons_) {
    SDL_Color color = btn.color;
    vertices.push_back({{btn.rect.x, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
    vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
    vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
    vertices.push_back({{btn.rect.x, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
    indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
    baseIndex += 4;
}

// Node buttons
if (currentMode_ != EditMode::HANDS_FEET) {
    for (const auto& btn : {addNodeBtn_, removeNodeBtn_}) {
        SDL_Color color = btn.color;
        vertices.push_back({{btn.rect.x, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
        vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
        vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
        vertices.push_back({{btn.rect.x, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
        indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
        baseIndex += 4;
    }
}

// Edit mode tabs
for (const auto& tab : editModeButtons_) {
    SDL_Color color = tab.color;
    vertices.push_back({{tab.rect.x, tab.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
    vertices.push_back({{tab.rect.x + tab.rect.w, tab.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
    vertices.push_back({{tab.rect.x + tab.rect.w, tab.rect.y + tab.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
    vertices.push_back({{tab.rect.x, tab.rect.y + tab.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}});
    indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
    baseIndex += 4;
}

SDL_RenderGeometry(renderer_, NULL, vertices.data(), vertices.size(), indices.data(), indices.size());

// Draw outlines for selected buttons
SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
for (const auto& btn : shapeButtons_) {
    if ((currentMode_ == EditMode::TORSO && btn.shapeType == player_.shapetype) ||
        ((currentMode_ == EditMode::APPENDAGE || currentMode_ == EditMode::HANDS_FEET) && btn.shapeType == currentShape_ && shapeSelectedForAppendage_)) {
        SDL_RenderRect(renderer_, &btn.rect);
    }
}
if (currentMode_ != EditMode::HANDS_FEET) {
    if (placingNode_) SDL_RenderRect(renderer_, &addNodeBtn_.rect);
    if (removingNode_) SDL_RenderRect(renderer_, &removeNodeBtn_.rect);
}
for (const auto& tab : editModeButtons_) {
    if (tab.mode == currentMode_) SDL_RenderRect(renderer_, &tab.rect);
}}void Game::render() {
    if (!renderer_) return;
    SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
    SDL_RenderClear(renderer_);
    renderUI();
    drawEntityWithNodesAndLines(renderer_, &player_);
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderLine(renderer_, 0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, SCREEN_HEIGHT - 1);
    SDL_RenderPresent(renderer_);
}bool Game::addAppendageToEntity(Entity* entity, float mouseX, float mouseY, Shape shape, int& nodeIndex, Entity*& parentEntity, bool isHandOrFoot) {
    if (!entity) return false;
    for (int i = 0; i < entity->nodeCount; i++) {
        float distance = distanceSquared(mouseX, mouseY, entity->nodes[i].x, entity->nodes[i].y);
        logDebug("Checking node %d of entity at (%.2f, %.2f): x=%.2f, y=%.2f, distance=%.2f\n",
                 i, entity->Xpos, entity->Ypos, entity->nodes[i].x, entity->nodes[i].y, sqrt(distance));
        if (distance <= 100.0f) {
            if (entity->appendages.size() >= MAX_APPENDAGES) {
                logDebug("Appendage limit reached (%d) for entity at (%.2f, %.2f)\n", MAX_APPENDAGES, entity->Xpos, entity->Ypos);
                return false;
            }
            Entity appendage;
            int width = (shape == TRIANGLE) ? 50 : 50;
            int height = (shape == TRIANGLE) ? 50 : 50;
            SDL_FPoint nodePos = {entity->nodes[i].x, entity->nodes[i].y};
            float offset = (shape == TRIANGLE) ? 50 : 50;
            initEntity(&appendage, renderer_, nodePos.x, nodePos.y + offset, width, height, shape, {0, 255, 0, 255}, 50, isHandOrFoot);
            appendage.isCore = false;
            appendage.coreNodeIndex = i;
            appendage.offsetX = 0.0f;
            appendage.offsetY = offset;
            appendage.isLeg = isHandOrFoot; // Set isLeg for hands/feet (feet are legs)
            entity->appendages.push_back(std::make_unique<Entity>(std::move(appendage)));
            nodeIndex = i;
            parentEntity = entity;
            logDebug("Added %s appendage to node %d at x=%.2f, y=%.2f on entity at (%.2f, %.2f), isLeg=%d\n",
                     isHandOrFoot ? "hand/foot" : "regular", i, nodePos.x, nodePos.y, entity->Xpos, entity->Ypos, appendage.isLeg);
            return true;
        }
    }
    for (auto& app : entity->appendages) {
        if (addAppendageToEntity(app.get(), mouseX, mouseY, shape, nodeIndex, parentEntity, isHandOrFoot)) {
            return true;
        }
    }
    return false;
}