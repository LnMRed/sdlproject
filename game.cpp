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
      renderer_(nullptr),
      inputManager_(this),
      debug_(true),
      lastStepTime_(0),
      currentStepFoot_(0),
      walkCycle_(0.0f),
      lastFrameTime_(0)
{

}


// Create renderer with default driver (chooses hardware-accelerated renderer)
Game::~Game() {
    destroyEntity(&player_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

// Enable V-Sync
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
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window_);
        SDL_Quit();
        return false;
    }
    if (!SDL_SetRenderVSync(renderer_, true)) {
        printf("SDL_SetRenderVSync Warning: %s\n", SDL_GetError());
    }

        // Initialize player_ with default values
    initEntity(&player_, renderer_, SCREEN_HEIGHT/2, SCREEN_WIDTH/2, 50, 50, Shape::TRIANGLE, {255, 0, 0, 255}, 50, false);
    player_.isCore = true;
    // Update player_ texture with renderer
    if (player_.texture) {
        SDL_DestroyTexture(player_.texture);
    }
    

    if (player_.texture) {
        SDL_SetTextureScaleMode(player_.texture, SDL_SCALEMODE_LINEAR);
    }
    logDebug("Player initialized at x=%.2f, y=%.2f, texture=%p\n", player_.Xpos, player_.Ypos, player_.texture);
    return true;
}

void Game::update() {
    if (!inputManager_.getInventoryOpen()) {
        // Apply gravity to core entity (fixed: removed double GRAVITY)
        player_.Yvel += GRAVITY;
        player_.Ypos += player_.Yvel;

        // Handle ground collision
        float lowestFootY = getLowestFootY(&player_);
        if (lowestFootY >= SCREEN_HEIGHT) {
            player_.Ypos -= (lowestFootY - SCREEN_HEIGHT);
            player_.Yvel = 0.0f;
            logDebug("Ground collision: adjusted player Ypos=%.2f, Yvel=0\n", player_.Ypos);
        }
    }

    // Handle movement
    auto feet = getFeet(&player_);
    bool canMove = false;
    for (auto* foot : feet) {
        if (foot->onGround) {
            canMove = true;
            break;
        }
    }
    if (canMove && !inputManager_.getInventoryOpen()) {
        if (inputManager_.getMovingLeft()) {
            player_.Xvel = -MOVE_SPEED;
        } else if (inputManager_.getMovingRight()) {
            player_.Xvel = MOVE_SPEED;
        } else {
            player_.Xvel = 0.0f;
        }
    } else {
        player_.Xvel = 0.0f;
    }
    player_.Xpos += player_.Xvel;
    player_.Xpos = std::max(0.0f, std::min(player_.Xpos, static_cast<float>(SCREEN_WIDTH)));

    // Update walking animation for feet (if moving)
    updateWalkingAnimation(&player_);

    // Update all appendage positions to follow the core entity
    updateAppendagePositions(&player_);

    logDebug("Updated player: x=%.2f, y=%.2f, Xvel=%.2f, Yvel=%.2f, canMove=%d\n",
             player_.Xpos, player_.Ypos, player_.Xvel, player_.Yvel, canMove);
}

void Game::logDebug(const char* format, ...) const {
    if (!debug_) return;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

float Game::distanceSquared(float x1, float y1, float x2, float y2) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

float Game::angleToPoint(float x1, float y1, float x2, float y2) const {
    return atan2(y2 - y1, x2 - x1);
}

bool Game::findParentNodePosition(Entity* appendage, float& nodeX, float& nodeY) {
    if (!appendage) return false;
    auto search = [&](Entity* entity, auto& self) -> bool {
        if (!entity) return false;
        for (auto& app : entity->appendages) {
            if (app.get() == appendage) {
                if (app->coreNodeIndex >= 0 && app->coreNodeIndex < entity->nodeCount) {
                    nodeX = entity->nodes[app->coreNodeIndex].x;
                    nodeY = entity->nodes[app->coreNodeIndex].y;
                    return true;
                }
                return false;
            }
            if (self(app.get(), self)) return true;
        }
        return false;
    };
    bool found = search(&player_, search);
    logDebug("findParentNodePosition: appendage=%p, found=%d, nodeX=%.2f, nodeY=%.2f\n", appendage, found, nodeX, nodeY);
    return found;
}

std::vector<Entity*> Game::getFeet(Entity* entity) {
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
}

float Game::getLowestFootY(Entity* entity) {
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
}

void Game::updateWalkingAnimation(Entity* entity) {
    auto feet = getFeet(entity);
    if (feet.empty() || (!inputManager_.getMovingLeft() && !inputManager_.getMovingRight())) {
        // Reset animation when not moving
        walkCycle_ = 0.0f;
        for (auto* foot : feet) {
            foot->rotation = 0.0f;
            foot->offsetX = 0.0f;
            foot->offsetY = foot->height / 2.0f; // Default offset below parent node
        }
        return;
    }

    // Update walk cycle based on delta time
    Uint32 currentTime = SDL_GetTicks();
    float dt = (currentTime - lastFrameTime_) / 1000.0f; // Delta time in seconds
    const float cycleDuration = 0.5f; // Time for one full step cycle
    walkCycle_ += dt / cycleDuration;
    if (walkCycle_ > 1.0f) walkCycle_ -= 1.0f; // Loop the cycle

    float direction = inputManager_.getMovingRight() ? 1.0f : -1.0f;
    for (size_t i = 0; i < feet.size(); ++i) {
        // Alternate phases for each foot
        float phase = (i == 0) ? walkCycle_ : (walkCycle_ + 0.5f);
        if (phase > 1.0f) phase -= 1.0f;

        auto* foot = feet[i];
        // Apply sinusoidal motion for stepping
        foot->rotation = sin(phase * 2.0f * M_PI) * 0.3f; // ~17 degrees max rotation
        foot->offsetX = cos(phase * 2.0f * M_PI) * 20.0f * direction; // Horizontal step
        foot->offsetY = foot->height / 2.0f - 10.0f * sin(phase * 2.0f * M_PI); // Vertical lift

        logDebug("Animating foot %zu: offsetX=%.2f, offsetY=%.2f, rotation=%.2f, onGround=%d\n",
                 i, foot->offsetX, foot->offsetY, foot->rotation, foot->onGround);
    }
}


void Game::renderUI() {
    if (!inputManager_.getInventoryOpen() || !renderer_) {
        logDebug("renderUI skipped: inventoryOpen=%d, renderer=%p\n", inputManager_.getInventoryOpen(), renderer_);
        return;
    }
    logDebug("Rendering inventory UI\n");

    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    int baseIndex = 0;

    // Inventory background
    SDL_Color bgColor = {100, 0, 100, 255};
    SDL_FRect invBox = {0, 0, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
    vertices.push_back({{invBox.x, invBox.y}, {bgColor.r / 255.0f, bgColor.g / 255.0f, bgColor.b / 255.0f, bgColor.a / 255.0f}, {0.0f, 0.0f}});
    vertices.push_back({{invBox.x + invBox.w, invBox.y}, {bgColor.r / 255.0f, bgColor.g / 255.0f, bgColor.b / 255.0f, bgColor.a / 255.0f}, {0.0f, 0.0f}});
    vertices.push_back({{invBox.x + invBox.w, invBox.y + invBox.h}, {bgColor.r / 255.0f, bgColor.g / 255.0f, bgColor.b / 255.0f, bgColor.a / 255.0f}, {0.0f, 0.0f}});
    vertices.push_back({{invBox.x, invBox.y + invBox.h}, {bgColor.r / 255.0f, bgColor.g / 255.0f, bgColor.b / 255.0f, bgColor.a / 255.0f}, {0.0f, 0.0f}});
    indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
    baseIndex += 4;

    // Shape buttons
    for (const auto& btn : inputManager_.getShapeButtons()) {
        SDL_Color color = btn.color;
        vertices.push_back({{btn.rect.x, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{btn.rect.x, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
        baseIndex += 4;
    }

    // Node buttons
    if (inputManager_.getCurrentMode() != InputManager::EditMode::HANDS_FEET) {
        for (const auto& btn : {inputManager_.getAddNodeButton(), inputManager_.getRemoveNodeButton()}) {
            SDL_Color color = btn.color;
            vertices.push_back({{btn.rect.x, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
            vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
            vertices.push_back({{btn.rect.x + btn.rect.w, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
            vertices.push_back({{btn.rect.x, btn.rect.y + btn.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
            indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
            baseIndex += 4;
        }
    }

    // Edit mode tabs
    for (const auto& tab : inputManager_.getEditModeButtons()) {
        SDL_Color color = tab.color;
        vertices.push_back({{tab.rect.x, tab.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{tab.rect.x + tab.rect.w, tab.rect.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{tab.rect.x + tab.rect.w, tab.rect.y + tab.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        vertices.push_back({{tab.rect.x, tab.rect.y + tab.rect.h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
        baseIndex += 4;
    }

    if (SDL_RenderGeometry(renderer_, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size()) != 0) {
        logDebug("SDL_RenderGeometry failed: %s\n", SDL_GetError());
    }

    // Draw outlines for selected buttons
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    for (const auto& btn : inputManager_.getShapeButtons()) {
        if ((inputManager_.getCurrentMode() == InputManager::EditMode::TORSO && btn.shapeType == player_.shapetype) ||
            ((inputManager_.getCurrentMode() == InputManager::EditMode::APPENDAGE || inputManager_.getCurrentMode() == InputManager::EditMode::HANDS_FEET) && btn.shapeType == inputManager_.getCurrentShape() && inputManager_.getShapeSelectedForAppendage())) {
            SDL_RenderRect(renderer_, &btn.rect);
        }
    }
    if (inputManager_.getCurrentMode() != InputManager::EditMode::HANDS_FEET) {
        if (inputManager_.getPlacingNode()) SDL_RenderRect(renderer_, &inputManager_.getAddNodeButton().rect);
        if (inputManager_.getRemovingNode()) SDL_RenderRect(renderer_, &inputManager_.getRemoveNodeButton().rect);
    }
    for (const auto& tab : inputManager_.getEditModeButtons()) {
        if (tab.mode == inputManager_.getCurrentMode()) SDL_RenderRect(renderer_, &tab.rect);
    }
}

void Game::render() {
    if (!renderer_) return;
    SDL_SetRenderDrawColor(renderer_, 100, 100, 100, 255);
    SDL_RenderClear(renderer_);
    renderUI();
    SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);
    drawEntityWithNodesAndLines(renderer_, &player_);
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderLine(renderer_, 0, SCREEN_HEIGHT - 1, SCREEN_WIDTH, SCREEN_HEIGHT - 1);
    SDL_RenderPresent(renderer_);
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