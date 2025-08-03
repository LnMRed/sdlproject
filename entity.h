#ifndef ENTITY_H
#define ENTITY_H
#include <memory>
#include <SDL3/SDL.h>
#include <vector>
#define MAX_NODES 50
struct Entity;
typedef enum {
    RECTANGLE,
    CIRCLE,
    TRIANGLE
} Shape;typedef struct {
    float x;
    float y;
} Node;struct NodeRel {
    float x_rel; // relative x, from -1 to 1
    float y_rel;
};struct RenderBatch {
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    Shape shapeType;
};struct Entity {
    Shape shapetype;
    float Xpos, Ypos, Xvel, Yvel;
    int width, height, size;
    bool onGround;
    SDL_Color color;
    SDL_Texture* texture;Node nodes[MAX_NODES];
NodeRel nodesRel[MAX_NODES];
int nodeCount;
bool isCore; // True for core shape (torso), false for appendages
bool isHandOrFoot; // True for hands or feet appendages
bool isLeg; // New flag for legs
bool grabbing = false; // True if this entity is grabbing something
int coreNodeIndex; // Index of the core shape's node this appendage is attached to (-1 for core)
float offsetX, offsetY;
float rotation;
Entity* grabbedObject = nullptr;
std::vector<std::unique_ptr<Entity>> appendages; // Sub-entities (limbs)

// Constructor for initializing with coreNodeIndex
Entity(int coreIndex = -1)
    : shapetype(RECTANGLE), Xpos(0.0f), Ypos(0.0f), Xvel(0.0f), Yvel(0.0f),
      width(0), height(0), size(0), onGround(false), color{255, 255, 255, 255},
      nodeCount(0), isCore(coreIndex == -1), isHandOrFoot(false),
      coreNodeIndex(coreIndex), offsetX(0.0f), offsetY(0.0f), rotation(0.0f), grabbedObject(nullptr) {
    // Initialize nodes and nodesRel arrays to zero
    for (int i = 0; i < MAX_NODES; ++i) {
        nodes[i] = {0.0f, 0.0f};
        nodesRel[i] = {0.0f, 0.0f};
    }
}

// Delete copy constructor and assignment to prevent copying
Entity(const Entity&) = delete;
Entity& operator=(const Entity&) = delete;

// Move constructor
Entity(Entity&& other) noexcept
    : shapetype(other.shapetype), Xpos(other.Xpos), Ypos(other.Ypos),
      Xvel(other.Xvel), Yvel(other.Yvel), width(other.width), height(other.height),
      size(other.size), onGround(other.onGround), color(other.color),
      nodeCount(other.nodeCount), isCore(other.isCore), isHandOrFoot(other.isHandOrFoot), isLeg(other.isLeg), grabbing(other.grabbing),
      coreNodeIndex(other.coreNodeIndex), offsetX(other.offsetX), offsetY(other.offsetY),
      rotation(other.rotation), grabbedObject(other.grabbedObject), appendages(std::move(other.appendages)) {
    for (int i = 0; i < MAX_NODES; ++i) {
        nodes[i] = other.nodes[i];
        nodesRel[i] = other.nodesRel[i];
    }
}

// Move assignment operator
Entity& operator=(Entity&& other) noexcept {
    if (this != &other) {
        shapetype = other.shapetype;
        Xpos = other.Xpos;
        Ypos = other.Ypos;
        Xvel = other.Xvel;
        Yvel = other.Yvel;
        width = other.width;
        height = other.height;
        size = other.size;
        onGround = other.onGround;
        color = other.color;
        nodeCount = other.nodeCount;
        isCore = other.isCore;
        isHandOrFoot = other.isHandOrFoot;
        isLeg = other.isLeg;
        grabbing = other.grabbing;
        coreNodeIndex = other.coreNodeIndex;
        offsetX = other.offsetX;
        offsetY = other.offsetY;
        rotation = other.rotation;
        grabbedObject = other.grabbedObject;
        appendages = std::move(other.appendages);
        for (int i = 0; i < MAX_NODES; ++i) {
            nodes[i] = other.nodes[i];
            nodesRel[i] = other.nodesRel[i];
        }
    }
    return *this;
}};void initEntity(Entity* entity,SDL_Renderer* renderer, float Xpos, float Ypos, int width, int height, Shape shape, SDL_Color color, int size, bool isHandOrFoot);
void drawFilledCircle(SDL_Renderer* renderer, int Xpos, int Ypos, int radius, SDL_Color color, float rotation);
void drawFilledTriangle(SDL_Renderer* renderer, SDL_Point p1, SDL_Point p2, SDL_Point p3, SDL_Color color, float rotation);
void drawEntity(SDL_Renderer* renderer, Entity* entity);
void drawEntityWithNodesAndLines(SDL_Renderer* renderer, Entity* entity);
bool pointInRectangle(float px, float py, Entity* entity);
bool pointInCircle(float px, float py, Entity* entity);
bool pointInTriangle(float px, float py, Entity* entity);
bool pointInEntityShape(float px, float py, Entity* entity);
NodeRel absoluteToRelative(Entity* entity, float abs_x, float abs_y);
void switchShape(Entity* entity, Shape newShape);
void updateNodePositions(Entity* entity);
SDL_FPoint clampNodeToShape(SDL_FPoint pt, Entity* entity);
NodeRel clampRelativeNodeToShape(NodeRel rel, Entity* entity);
SDL_FPoint relativeToAbsolute(Entity* entity, NodeRel nodeRel);
void generateNodes(Entity* entity);
void updateAppendagePositions(Entity* entity);
bool addNodeToEntity(Entity* entity, float mouseX, float mouseY);
bool shouldRemoveAppendage(const std::unique_ptr<Entity>& entity);
void removeNodeFromEntity(Entity* entity, float mouseX, float mouseY);
Entity* findAppendageAtPoint(Entity* entity, float px, float py);
bool isEntityOnGround(Entity* entity, float groundY);
void destroyEntity(Entity* entity);
#endif // ENTITY_H

