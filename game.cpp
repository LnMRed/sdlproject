#include <cstdarg>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "game.h"
#include "InputManager.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Game::Game()
    : window_(nullptr),
      sdl_renderer_(nullptr),
      renderer_(nullptr),
      inputManager_(this),
      debug_(false),
      lastStepTime_(0),
      currentStepFoot_(0),
      walkCycle_(0.0f),
      lastFrameTime_(0)
{
}

Game::~Game() {
    destroyEntity(&player_);
    destroyEntity(&grabbableBall_);
    if (sdl_renderer_) SDL_DestroyRenderer(sdl_renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool Game::init() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }
    window_ = SDL_CreateWindow("Game", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window_) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    sdl_renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!sdl_renderer_) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window_);
        SDL_Quit();
        return false;
    }
    if (!SDL_SetRenderVSync(sdl_renderer_, true)) {
        printf("SDL_SetRenderVSync Warning: %s\n", SDL_GetError());
    }
    renderer_ = Renderer(sdl_renderer_);

    initEntity(&player_, &renderer_, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 50, 50, Shape::TRIANGLE, {255, 0, 0, 255}, 50, false, true);
    player_.isCore = true;
    
    if (player_.texture) {
        renderer_.setTextureScaleMode(player_.texture, SDL_SCALEMODE_LINEAR);
    }

    float ballX = SCREEN_WIDTH - 60;
    float ballY = SCREEN_HEIGHT - 100; // Adjusted to start higher for visibility
    int ballRadius = 30;
    initEntity(&grabbableBall_, &renderer_, ballX, ballY, ballRadius * 2, ballRadius * 2, Shape::CIRCLE, {0, 200, 255, 255}, ballRadius * 2, false, false);
    grabbableBall_.isCore = false;
    grabbableBall_.Xvel = 0.0f;
    grabbableBall_.Yvel = 0.0f;

    grabbableEntities_.push_back(&grabbableBall_);

    logDebug("Player initialized at x=%.2f, y=%.2f, texture=%p\n", player_.Xpos, player_.Ypos, player_.texture);
    logDebug("Grabbable ball initialized at x=%.2f, y=%.2f, nodeCount=%d\n", grabbableBall_.Xpos, grabbableBall_.Ypos, grabbableBall_.nodeCount);
    return true;
}

void Game::updateHands(Entity* entity) {
    // Use InputManager’s tracked state instead of SDL_GetMouseState
    bool isLeftMouseDown = inputManager_.isLeftMouseHeld();

    for (auto& app : entity->appendages) {
        if (app->isHandOrFoot && app->shapetype == Shape::TRIANGLE && !inputManager_.getInventoryOpen()) {
            float nodeX = 0.0f, nodeY = 0.0f;
            if (findParentNodePosition(app.get(), nodeX, nodeY)) {
                float dx = inputManager_.getMouseX() - nodeX;
                float dy = inputManager_.getMouseY() - nodeY;
                float dist = std::sqrt(dx * dx + dy * dy);

                // Clamp arm length
                float maxArmLength = 120.0f;
                if (dist > maxArmLength) {
                    dx *= maxArmLength / dist;
                    dy *= maxArmLength / dist;
                }

                bool wasGrabbing = app->grabbing;
                app->grabbing = isLeftMouseDown;  // follow InputManager state

                if (!app->grabbing && app->grabbedObject) {
                    // Release immediately on mouse up
                    app->grabbedObject = nullptr;
                    logDebug("Released grabbed object\n");
                }
                else if (app->grabbing && !wasGrabbing) {
                    // Just started grabbing
                    app->offsetX = dx;
                    app->offsetY = dy;

                    float handX = nodeX + app->offsetX;
                    float handY = nodeY + app->offsetY;

                    app->grabbedObject = getGrabbableAt(handX, handY, 15.0f);
                    if (app->grabbedObject) {
                        logDebug("Hand at (%.2f, %.2f) grabbed object at (%.2f, %.2f)\n",
                                 handX, handY,
                                 app->grabbedObject->Xpos, app->grabbedObject->Ypos);
                    }
                }

                // Smooth movement interpolation
                float handLerp = std::clamp(dist / 60.0f, 0.15f, 0.7f);
                app->offsetX += (dx - app->offsetX) * handLerp;
                app->offsetY += (dy - app->offsetY) * handLerp;
                app->rotation = std::atan2(dy, dx);

                // While grabbing, drag object along with hand
                if (app->grabbing && app->grabbedObject) {
                    app->grabbedObject->Xpos = nodeX + app->offsetX;
                    app->grabbedObject->Ypos = nodeY + app->offsetY;
                    app->grabbedObject->Xvel = 0.0f;
                    app->grabbedObject->Yvel = 0.0f;
                    logDebug("Grabbed object moved to (%.2f, %.2f)\n",
                             app->grabbedObject->Xpos, app->grabbedObject->Ypos);
                }
            }
        }
        else if (app->grabbedObject && !app->grabbing) {
            // Safety release (in case branch above didn’t catch it)
            app->grabbedObject = nullptr;
            logDebug("Released grabbed object (safety)\n");
        }
        // Recurse into appendages
        updateHands(app.get());
    }
}



Entity* Game::getGrabbableAt(float x, float y, float tolerance) {
    if (std::abs(y - 700.0f) < tolerance) {
        return nullptr;
    }
    for (Entity* obj : grabbableEntities_) {
        float dx = x - obj->Xpos;
        float dy = y - obj->Ypos;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < obj->width / 2.0f + tolerance) {
            return obj;
        }
    }
    return nullptr;
}

void Game::update() {
    if (!inputManager_.getInventoryOpen()) {
        // Update player
        player_.Yvel += GRAVITY;
        player_.Ypos += player_.Yvel;

        float lowestY = getLowestEntityY(&player_);
        if (lowestY >= SCREEN_HEIGHT) {
            player_.Ypos -= (lowestY - SCREEN_HEIGHT);
            player_.Yvel = 0.0f;
            player_.onGround = true;
            logDebug("Ground collision: adjusted player Ypos=%.2f, Yvel=0\n", player_.Ypos);
        } else {
            player_.onGround = false;
        }

        player_.Xpos += player_.Xvel;

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        getEntityMinMaxX(&player_, minX, maxX);

        if (minX < 0.0f) {
            player_.Xpos -= minX;
        }
        if (maxX > SCREEN_WIDTH) {
            player_.Xpos -= (maxX - SCREEN_WIDTH);
        }

        // Update grabbable ball (if not grabbed)
        bool isBallGrabbed = false;
        for (auto& app : player_.appendages) {
            if (app->grabbedObject == &grabbableBall_) {
                isBallGrabbed = true;
                break;
            }
        }
        if (!isBallGrabbed) {
            grabbableBall_.Yvel += GRAVITY;
            grabbableBall_.Ypos += grabbableBall_.Yvel;
            if (grabbableBall_.Ypos + grabbableBall_.height / 2.0f >= SCREEN_HEIGHT) {
                grabbableBall_.Ypos = SCREEN_HEIGHT - grabbableBall_.height / 2.0f;
                grabbableBall_.Yvel = 0.0f;
                grabbableBall_.onGround = true;
                logDebug("Ball ground collision: Ypos=%.2f, Yvel=0\n", grabbableBall_.Ypos);
            } else {
                grabbableBall_.onGround = false;
            }
        }
    }

    if (inputManager_.getMovingLeft() && !inputManager_.getInventoryOpen()) {
        player_.Xvel = -MOVE_SPEED;
    } else if (inputManager_.getMovingRight() && !inputManager_.getInventoryOpen()) {
        player_.Xvel = MOVE_SPEED;
    } else {
        player_.Xvel = 0.0f;
    }

    if (inputManager_.getJumpRequested() && player_.onGround && !inputManager_.getInventoryOpen()) {
        player_.Yvel = -10.0f;
        player_.onGround = false;
        inputManager_.clearJumpRequested();
        logDebug("Jump initiated: Yvel=%.2f\n", player_.Yvel);
    }

    if (std::abs(player_.Xvel) > 0.0f && player_.onGround) {
        updateWalkingAnimation(&player_);
    } else {
        walkCycle_ = 0.0f;
    }

    updateNodePositions(&player_);
    updateAppendagePositions(&player_);
    updateHands(&player_);
}

void Game::updateWalkingAnimation(Entity* entity) {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastStepTime_ >= STEP_INTERVAL) {
        std::vector<Entity*> feet = getFeet(entity);
        if (!feet.empty()) {
            Entity* foot = feet[currentStepFoot_ % feet.size()];
            foot->rotation = sin(walkCycle_) * 0.2f;
            walkCycle_ += 0.1f;
            currentStepFoot_ = (currentStepFoot_ + 1) % feet.size();
            lastStepTime_ = currentTime;
        }
    }
}

std::vector<Entity*> Game::getFeet(Entity* entity) {
    std::vector<Entity*> feet;
    for (auto& app : entity->appendages) {
        if (app->isHandOrFoot && app->isLeg) {
            feet.push_back(app.get());
        }
        auto subFeet = getFeet(app.get());
        feet.insert(feet.end(), subFeet.begin(), subFeet.end());
    }
    return feet;
}

float Game::getLowestEntityY(Entity* entity) {
    float lowestY = entity->Ypos + entity->height / 2.0f;
    for (auto& app : entity->appendages) {
        lowestY = std::max(lowestY, getLowestEntityY(app.get()));
    }
    return lowestY;
}

void Game::getEntityMinMaxX(Entity* entity, float& minX, float& maxX) {
    float hw = entity->width / 2.0f;
    float cx = entity->Xpos;
    float cy = entity->Ypos;
    float rot = entity->rotation;
    SDL_FPoint points[4] = {
        {-hw, -hw}, {hw, -hw}, {hw, hw}, {-hw, hw}
    };
    for (int i = 0; i < 4; ++i) {
        float rx = points[i].x * cos(rot) - points[i].y * sin(rot);
        float x = cx + rx;
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
    }
    for (auto& app : entity->appendages) {
        getEntityMinMaxX(app.get(), minX, maxX);
    }
}

void Game::logDebug(const char* format, ...) const {
    if (!debug_) return;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

bool Game::findParentNodePosition(Entity* appendage, float& nodeX, float& nodeY) {
    if (!appendage || appendage->isCore || appendage->coreNodeIndex < 0) {
        return false;
    }
    Entity* parent = &player_;
    for (auto& app : parent->appendages) {
        if (app.get() == appendage) {
            if (appendage->coreNodeIndex < parent->nodeCount) {
                nodeX = parent->nodes[appendage->coreNodeIndex].x;
                nodeY = parent->nodes[appendage->coreNodeIndex].y;
                return true;
            }
            return false;
        }
        for (auto& subApp : app->appendages) {
            if (subApp.get() == appendage) {
                if (appendage->coreNodeIndex < app->nodeCount) {
                    nodeX = app->nodes[appendage->coreNodeIndex].x;
                    nodeY = app->nodes[appendage->coreNodeIndex].y;
                    return true;
                }
                return false;
            }
        }
    }
    return false;
}

float Game::angleToPoint(float x1, float y1, float x2, float y2) const {
    return atan2(y2 - y1, x2 - x1);
}

float Game::distanceSquared(float x1, float y1, float x2, float y2) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

void Game::renderUI() {
    if (!inputManager_.getInventoryOpen()) return;

    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    int baseIndex = 0;

    // Draw shape buttons
    for (const auto& btn : inputManager_.getShapeButtons()) {
        SDL_Color color = btn.color;
        vertices.push_back({{btn.rect.x, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{btn.rect.x, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
        baseIndex += 4;
    }

    // Draw node buttons if not in HANDS_FEET mode
    if (inputManager_.getCurrentMode() != InputManager::EditMode::HANDS_FEET) {
        auto addBtn = inputManager_.getAddNodeButton();
        SDL_Color addColor = addBtn.color;
        vertices.push_back({{addBtn.rect.x, addBtn.rect.y}, {addColor.r / 255.0f, addColor.g / 255.0f, addColor.b / 255.0f, addColor.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{addBtn.rect.x + addBtn.rect.w, addBtn.rect.y}, {addColor.r / 255.0f, addColor.g / 255.0f, addColor.b / 255.0f, addColor.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{addBtn.rect.x + addBtn.rect.w, addBtn.rect.y + addBtn.rect.h}, {addColor.r / 255.0f, addColor.g / 255.0f, addColor.b / 255.0f, addColor.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{addBtn.rect.x, addBtn.rect.y + addBtn.rect.h}, {addColor.r / 255.0f, addColor.g / 255.0f, addColor.b / 255.0f, addColor.a / 255.0f}, {0.0f, 0.0f}});
        indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
        baseIndex += 4;

        auto removeBtn = inputManager_.getRemoveNodeButton();
        SDL_Color removeColor = removeBtn.color;
        vertices.push_back({{removeBtn.rect.x, removeBtn.rect.y}, {removeColor.r / 255.0f, removeColor.g / 255.0f, removeColor.b / 255.0f, removeColor.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{removeBtn.rect.x + removeBtn.rect.w, removeBtn.rect.y}, {removeColor.r / 255.0f, removeColor.g / 255.0f, removeColor.b / 255.0f, removeColor.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{removeBtn.rect.x + removeBtn.rect.w, removeBtn.rect.y + removeBtn.rect.h}, {removeColor.r / 255.0f, removeColor.g / 255.0f, removeColor.b / 255.0f, removeColor.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{removeBtn.rect.x, removeBtn.rect.y + removeBtn.rect.h}, {removeColor.r / 255.0f, removeColor.g / 255.0f, removeColor.b / 255.0f, removeColor.a / 255.0f}, {0.0f, 0.0f}});
        indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
        baseIndex += 4;
    }

    // Draw edit mode buttons
    for (const auto& tab : inputManager_.getEditModeButtons()) {
        SDL_Color color = tab.color;
        vertices.push_back({{tab.rect.x, tab.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{tab.rect.x + tab.rect.w, tab.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{tab.rect.x + tab.rect.w, tab.rect.y + tab.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{tab.rect.x, tab.rect.y + tab.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
        baseIndex += 4;
    }

    renderer_.renderGeometry(vertices.data(), vertices.size(), indices.data(), indices.size());

    // Draw borders for active buttons
    renderer_.setDrawColor({255, 255, 255, 255});
    for (const auto& btn : inputManager_.getShapeButtons()) {
        if ((inputManager_.getCurrentMode() == InputManager::EditMode::TORSO && btn.shapeType == player_.shapetype) ||
            ((inputManager_.getCurrentMode() == InputManager::EditMode::APPENDAGE || inputManager_.getCurrentMode() == InputManager::EditMode::HANDS_FEET) && btn.shapeType == inputManager_.getCurrentShape() && inputManager_.getShapeSelectedForAppendage())) {
            renderer_.drawRect(&btn.rect);
        }
    }
    if (inputManager_.getCurrentMode() != InputManager::EditMode::HANDS_FEET) {
        if (inputManager_.getPlacingNode()) renderer_.drawRect(&inputManager_.getAddNodeButton().rect);
        if (inputManager_.getRemovingNode()) renderer_.drawRect(&inputManager_.getRemoveNodeButton().rect);
    }
    for (const auto& tab : inputManager_.getEditModeButtons()) {
        if (tab.mode == inputManager_.getCurrentMode()) {
            renderer_.drawRect(&tab.rect);
        }
    }
}

bool Game::addAppendageToEntity(Entity* entity, float mouseX, float mouseY, Shape shape, int& nodeIndex, Entity*& parentEntity, bool isHandOrFoot) {
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
            Entity appendage(-1); // Initialize with coreNodeIndex = -1, will be set below
            int width = 50;
            int height = 50;
            SDL_FPoint nodePos = {entity->nodes[i].x, entity->nodes[i].y};
            float offset = 50;
            initEntity(&appendage, &renderer_, nodePos.x, nodePos.y + offset, width, height, shape, {0, 255, 0, 255}, 50, isHandOrFoot);
            appendage.isCore = false;
            appendage.coreNodeIndex = i;
            appendage.offsetX = 0.0f;
            appendage.offsetY = offset;
            appendage.isHandOrFoot = isHandOrFoot;
            appendage.isLeg = isHandOrFoot && shape == Shape::RECTANGLE;
            entity->appendages.push_back(std::make_unique<Entity>(std::move(appendage)));
            nodeIndex = i;
            parentEntity = entity;
            logDebug("Added %s appendage (shape=%d, isHandOrFoot=%d, isLeg=%d) to node %d at x=%.2f, y=%.2f on entity at (%.2f, %.2f)\n",
                     isHandOrFoot ? "hand/foot" : "regular", shape, isHandOrFoot, appendage.isLeg, i, nodePos.x, nodePos.y, entity->Xpos, entity->Ypos);
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

void Game::run() {
    lastFrameTime_ = SDL_GetTicks();
    bool running = true;
    while (running) {
        inputManager_.handleEvents();
        update();
        render();
        Uint32 currentTime = SDL_GetTicks();
        Uint32 frameTime = currentTime - lastFrameTime_;
        if (frameTime < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }
        lastFrameTime_ = currentTime;
    }
}

void Game::render() {
    renderer_.clear({100, 100, 100, 255});
    renderUI();
    renderer_.setDrawColor({255, 0, 0, 255});
    renderer_.drawEntityWithNodesAndLines(&player_);
    renderer_.setDrawColor({0, 200, 255, 255});
    renderer_.drawEntityWithNodesAndLines(&grabbableBall_);
    renderer_.setDrawColor({255, 255, 255, 255});
    renderer_.drawLine(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, SCREEN_HEIGHT - 1);
    renderer_.present();
}