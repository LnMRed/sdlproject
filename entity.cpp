#include "entity.h"
#include "Renderer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

NodeRel absoluteToRelative(Entity* entity, float abs_x, float abs_y) {
    NodeRel rel;
    float cx = entity->Xpos;
    float cy = entity->Ypos;
    float dx = abs_x - cx;
    float dy = abs_y - cy;
    float rot = -entity->rotation;
    float rx = dx * cos(rot) - dy * sin(rot);
    float ry = dx * sin(rot) + dy * cos(rot);
    rel.x_rel = rx / (entity->width / 2.0f);
    rel.y_rel = ry / (entity->height / 2.0f);
    
    //printf("Absolute to relative: abs_x=%.2f, abs_y=%.2f, entity=(%.2f, %.2f), rotation=%.2f, x_rel=%.2f, y_rel=%.2f\n",
    //       abs_x, abs_y, cx, cy, entity->rotation, rel.x_rel, rel.y_rel);
    
    return rel;
}

SDL_FPoint relativeToAbsolute(Entity* entity, NodeRel nodeRel) {
    SDL_FPoint abs;
    float rx = nodeRel.x_rel * (entity->width / 2.0f);
    float ry = nodeRel.y_rel * (entity->height / 2.0f);
    float rot = entity->rotation;
    abs.x = entity->Xpos + rx * cos(rot) - ry * sin(rot);
    abs.y = entity->Ypos + rx * sin(rot) + ry * cos(rot);
    
    //printf("Relative to absolute: x_rel=%.2f, y_rel=%.2f, entity=(%.2f, %.2f), rotation=%.2f, abs_x=%.2f, abs_y=%.2f\n",
    //      nodeRel.x_rel, nodeRel.y_rel, entity->Xpos, entity->Ypos, rot, abs.x, abs.y);
    
    return abs;
}

bool pointInRectangle(float px, float py, Entity* entity) {
    float cx = entity->Xpos;
    float cy = entity->Ypos;
    float rot = -entity->rotation;
    float dx = px - cx;
    float dy = py - cy;
    float rx = dx * cos(rot) - dy * sin(rot);
    float ry = dx * sin(rot) + dy * cos(rot);
    float left = -entity->width / 2.0f;
    float right = entity->width / 2.0f;
    float top = -entity->height / 2.0f;
    float bottom = entity->height / 2.0f;
    bool inside = (rx >= left && rx <= right && ry >= top && ry <= bottom);
    printf("Checking point in rectangle: px=%.2f, py=%.2f, center=(%.2f, %.2f), w=%d, h=%d, rotation=%.2f, rx=%.2f, ry=%.2f, inside=%d\n",
           px, py, cx, cy, entity->width, entity->height, entity->rotation, rx, ry, inside);
    return inside;
}

bool pointInCircle(float px, float py, Entity* entity) {
    float dx = px - entity->Xpos;
    float dy = py - entity->Ypos;
    float r = entity->width / 2.0f;
    bool inside = (dx * dx + dy * dy <= r * r);
    printf("Checking point in circle: px=%.2f, py=%.2f, center=(%.2f, %.2f), radius=%.2f, inside=%d\n",
           px, py, entity->Xpos, entity->Ypos, r, inside);
    return inside;
}

bool pointInTriangle(float px, float py, Entity* entity) {
    float s = entity->width / 2.0f;
    float cx = entity->Xpos;
    float cy = entity->Ypos;
    float rot = entity->rotation;
    SDL_FPoint p1 = {0.0f, -s};
    SDL_FPoint p2 = {-s, s};
    SDL_FPoint p3 = {s, s};
    SDL_FPoint rp1, rp2, rp3;
    rp1.x = cx + (p1.x * cos(rot) - p1.y * sin(rot));
    rp1.y = cy + (p1.x * sin(rot) + p1.y * cos(rot));
    rp2.x = cx + (p2.x * cos(rot) - p2.y * sin(rot));
    rp2.y = cy + (p2.x * sin(rot) + p2.y * cos(rot));
    rp3.x = cx + (p3.x * cos(rot) - p3.y * sin(rot));
    rp3.y = cy + (p3.x * sin(rot) + p3.y * cos(rot));
    float denom = (rp2.y - rp3.y) * (rp1.x - rp3.x) + (rp3.x - rp2.x) * (rp1.y - rp3.y);
    if (fabs(denom) < 0.0001f) { // Relaxed tolerance
        printf("Triangle point check failed: near-degenerate triangle, denom=%.6f\n", denom);
        return false;
    }
    float a = ((rp2.y - rp3.y) * (px - rp3.x) + (rp3.x - rp2.x) * (py - rp3.y)) / denom;
    float b = ((rp3.y - rp1.y) * (px - rp3.x) + (rp1.x - rp3.x) * (py - rp3.y)) / denom;
    float c = 1.0f - a - b;
    bool inside = (a >= -0.01f && b >= -0.01f && c >= -0.01f && (a + b + c) <= 1.01f); // Relaxed bounds
    printf("Checking point in triangle: px=%.2f, py=%.2f, center=(%.2f, %.2f), s=%.2f, rotation=%.2f, a=%.2f, b=%.2f, c=%.2f, inside=%d\n",
           px, py, cx, cy, s, rot, a, b, c, inside);
    return inside;
}

bool pointInEntityShape(float px, float py, Entity* entity) {
    bool result = false;
    switch (entity->shapetype) {
        case RECTANGLE: result = pointInRectangle(px, py, entity); break;
        case CIRCLE: result = pointInCircle(px, py, entity); break;
        case TRIANGLE: result = pointInTriangle(px, py, entity); break;
        default: printf("Unknown shape type: %d\n", entity->shapetype); break;
    }
    return result;
}

void GenerateNodes(Entity* entity) {
    if (!entity) {
        printf("Error: generateNodes called with null entity\n");
        return;
    }
    entity->nodeCount = 0;
    switch (entity->shapetype) {
        case RECTANGLE:
        case CIRCLE: {
            entity->nodesRel[0] = {0.0f, -1.0f}; // top
            entity->nodesRel[1] = {0.0f, 1.0f};  // bottom
            entity->nodesRel[2] = {-1.0f, 0.0f}; // left
            entity->nodesRel[3] = {1.0f, 0.0f};  // right
            entity->nodeCount = 4;
            break;
        }
        case TRIANGLE: {
            entity->nodesRel[0] = {0.0f, -1.0f}; // top
            entity->nodesRel[1] = {-1.0f, 1.0f}; // bottom left
            entity->nodesRel[2] = {1.0f, 1.0f};  // bottom right
            entity->nodeCount = 3;
            break;
        }
    }
    updateNodePositions(entity);
}

void updateNodePositions(Entity* entity) {
    for (int i = 0; i < entity->nodeCount; ++i) {
        SDL_FPoint abs = relativeToAbsolute(entity, entity->nodesRel[i]);
        entity->nodes[i].x = abs.x;
        entity->nodes[i].y = abs.y;
    }
}

SDL_FPoint clampNodeToShape(SDL_FPoint pt, Entity* entity) {
    switch (entity->shapetype) {
        case RECTANGLE: {
            float hw = entity->width / 2.0f;
            float hh = entity->height / 2.0f;
            float cx = entity->Xpos;
            float cy = entity->Ypos;
            float rot = -entity->rotation;
            float dx = pt.x - cx;
            float dy = pt.y - cy;
            float rx = dx * cos(rot) - dy * sin(rot);
            float ry = dx * sin(rot) + dy * cos(rot);
            rx = std::clamp(rx, -hw, hw);
            ry = std::clamp(ry, -hh, hh);
            pt.x = cx + rx * cos(-rot) - ry * sin(-rot);
            pt.y = cy + rx * sin(-rot) + ry * cos(-rot);
            break;
        }
        case CIRCLE: {
            float r = entity->width / 2.0f;
            float cx = entity->Xpos;
            float cy = entity->Ypos;
            float dx = pt.x - cx;
            float dy = pt.y - cy;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist > r && dist > 0.0001f) {
                float scale = r / dist;
                pt.x = cx + dx * scale;
                pt.y = cy + dy * scale;
            }
            break;
        }
        case TRIANGLE: {
            float s = entity->width / 2.0f;
            float cx = entity->Xpos;
            float cy = entity->Ypos;
            float rot = -entity->rotation;
            float dx = pt.x - cx;
            float dy = pt.y - cy;
            float rx = dx * cos(rot) - dy * sin(rot);
            float ry = dx * sin(rot) + dy * cos(rot);
            // Triangle bounds: top vertex at (0, -s), bottom left (-s, s), bottom right (s, s)
            // Use barycentric coordinates to clamp
            SDL_FPoint p1 = {0.0f, -s};
            SDL_FPoint p2 = {-s, s};
            SDL_FPoint p3 = {s, s};
            float denom = (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y);
            if (fabs(denom) < 0.0001f) {
                return pt; // Degenerate triangle
            }
            float a = ((p2.y - p3.y) * (rx - p3.x) + (p3.x - p2.x) * (ry - p3.y)) / denom;
            float b = ((p3.y - p1.y) * (rx - p3.x) + (p1.x - p3.x) * (ry - p3.y)) / denom;
            float c = 1.0f - a - b;
            a = std::clamp(a, 0.0f, 1.0f);
            b = std::clamp(b, 0.0f, 1.0f);
            c = std::clamp(c, 0.0f, 1.0f);
            float sum = a + b + c;
            if (sum > 0.0001f) {
                a /= sum;
                b /= sum;
                c /= sum;
            }
            rx = a * p1.x + b * p2.x + c * p3.x;
            ry = a * p1.y + b * p2.y + c * p3.y;
            pt.x = cx + rx * cos(-rot) - ry * sin(-rot);
            pt.y = cy + rx * sin(-rot) + ry * cos(-rot);
            break;
        }
    }
    return pt;
}

NodeRel clampRelativeNodeToShape(NodeRel rel, Entity* entity) {
    switch (entity->shapetype) {
        case RECTANGLE:
        case CIRCLE: {
            rel.x_rel = std::clamp(rel.x_rel, -1.0f, 1.0f);
            rel.y_rel = std::clamp(rel.y_rel, -1.0f, 1.0f);
            if (entity->shapetype == CIRCLE) {
                float dist = sqrt(rel.x_rel * rel.x_rel + rel.y_rel * rel.y_rel);
                if (dist > 1.0f && dist > 0.0001f) {
                    float scale = 1.0f / dist;
                    rel.x_rel *= scale;
                    rel.y_rel *= scale;
                }
            }
            break;
        }
        case TRIANGLE: {
            float a = -rel.y_rel;
            float b = (rel.x_rel + rel.y_rel) / 2.0f;
            float c = 1.0f - a - b;
            a = std::clamp(a, 0.0f, 1.0f);
            b = std::clamp(b, 0.0f, 1.0f);
            c = std::clamp(c, 0.0f, 1.0f);
            float sum = a + b + c;
            if (sum > 0.0001f) {
                a /= sum;
                b /= sum;
                c /= sum;
            }
            rel.x_rel = -b + c;
            rel.y_rel = -a + b + c;
            break;
        }
    }
    return rel;
}

void switchShape(Entity* entity, Shape newShape) {
    entity->shapetype = newShape;
    GenerateNodes(entity);
}

void updateAppendagePositions(Entity* entity) {
    for (auto& app : entity->appendages) {
        if (app->coreNodeIndex >= 0 && app->coreNodeIndex < entity->nodeCount) {
            SDL_FPoint nodePos = {entity->nodes[app->coreNodeIndex].x, entity->nodes[app->coreNodeIndex].y};
            float rot = entity->rotation;
            app->Xpos = nodePos.x + app->offsetX * cos(rot) - app->offsetY * sin(rot);
            app->Ypos = nodePos.y + app->offsetX * sin(rot) + app->offsetY * cos(rot);
            app->rotation = rot;
            updateNodePositions(app.get());
            updateAppendagePositions(app.get());
        }
    }
}

bool addNodeToEntity(Entity* entity, float mouseX, float mouseY) {
    if (entity->nodeCount >= MAX_NODES) {
        printf("Node limit reached (%d) for entity at (%.2f, %.2f)\n", MAX_NODES, entity->Xpos, entity->Ypos);
        return false;
    }
    NodeRel rel = absoluteToRelative(entity, mouseX, mouseY);
    rel = clampRelativeNodeToShape(rel, entity);
    entity->nodesRel[entity->nodeCount] = rel;
    SDL_FPoint abs = relativeToAbsolute(entity, rel);
    entity->nodes[entity->nodeCount] = {abs.x, abs.y};
    entity->nodeCount++;
    printf("Added node %d at x=%.2f, y=%.2f (rel: %.2f, %.2f) to entity at (%.2f, %.2f)\n",
           entity->nodeCount - 1, abs.x, abs.y, rel.x_rel, rel.y_rel, entity->Xpos, entity->Ypos);
    return true;
}

bool shouldRemoveAppendage(const std::unique_ptr<Entity>& entity) {
    for (const auto& app : entity->appendages) {
        if (shouldRemoveAppendage(app)) return true;
    }
    return entity->appendages.empty();
}

void deleteAppendagesAtNode(Entity* entity, int nodeIndex)
{
    if (!entity) return;

    auto it = entity->appendages.begin();
    while (it != entity->appendages.end()) {
        if ((*it)->coreNodeIndex == nodeIndex) {
            for(auto& app : (*it)->appendages) {
                destroyEntity(app.get());
            }
            (*it)->appendages.clear();
            destroyEntity(it->get());
            it = entity->appendages.erase(it);
        } else {
            ++it;
        }
    }
    for (auto& app : entity->appendages) {
        deleteAppendagesAtNode(app.get(), nodeIndex);
    }

}

void removeNodeFromEntity(Entity* entity, float mouseX, float mouseY) {
    float minDist = 100.0f; // Threshold for node proximity
    int closestNode = -1;
    for (int i = 0; i < entity->nodeCount; i++) {
        float dx = mouseX - entity->nodes[i].x;
        float dy = mouseY - entity->nodes[i].y;
        float dist = dx * dx + dy * dy;
        if (dist < minDist) {
            bool nodeInUse = false;
            for (const auto& app : entity->appendages) {
                if (app->coreNodeIndex == i) {
                    nodeInUse = true;
                    break;
                }
            }
            if (!nodeInUse) {
                minDist = dist;
                closestNode = i;
            } else 
            {
                deleteAppendagesAtNode(entity, i);
                minDist = dist;
                closestNode = i;
                printf("Deleted appendages at node %d before removal\n", i);
            }
        }
    }
    if (closestNode >= 0) {
        for (int i = closestNode; i < entity->nodeCount - 1; i++) {
            entity->nodes[i] = entity->nodes[i + 1];
            entity->nodesRel[i] = entity->nodesRel[i + 1];
            for (auto& app : entity->appendages) {
                if (app->coreNodeIndex > closestNode) {
                    app->coreNodeIndex--;
                }
            }
        }
        entity->nodeCount--;
        printf("Removed node %d from entity at (%.2f, %.2f), new nodeCount=%d\n",
               closestNode, entity->Xpos, entity->Ypos, entity->nodeCount);
    }

    for (auto& app : entity->appendages) {
        removeNodeFromEntity(app.get(), mouseX, mouseY);
    }
}

Entity* findAppendageAtPoint(Entity* entity, float px, float py) {
    if (!entity->isCore && pointInEntityShape(px, py, entity)) {
        printf("Found appendage at x=%.2f, y=%.2f, isHandOrFoot=%d\n", px, py, entity->isHandOrFoot);
        return entity;
    }
    for (auto& app : entity->appendages) {
        Entity* result = findAppendageAtPoint(app.get(), px, py);
        if (result) return result;
    }
    return nullptr;
}

bool isEntityOnGround(Entity* entity, float groundY) {
    float lowestY = entity->Ypos + entity->height / 2.0f;
    for (auto& app : entity->appendages) {
        float appLowestY = isEntityOnGround(app.get(), groundY);
        lowestY = std::max(lowestY, appLowestY);
    }
    return lowestY >= groundY;
}

void destroyEntity(Entity* entity) {
    if (entity->texture) {
        SDL_DestroyTexture(entity->texture);
        entity->texture = nullptr;
    }
    for (auto& app : entity->appendages) {
        destroyEntity(app.get());
    }
    entity->appendages.clear();
}

void initEntity(Entity* entity, Renderer* renderer, float Xpos, float Ypos, int width, int height, Shape shape, SDL_Color color, int size, bool isHandOrFoot, bool generateNodes) {
    if (!entity) return;
    entity->shapetype = shape;
    entity->Xpos = Xpos;
    entity->Ypos = Ypos;
    entity->Xvel = 0.0f;
    entity->Yvel = 0.0f;
    entity->width = width;
    entity->height = height;
    entity->size = size;
    entity->onGround = false;
    entity->color = color;
    entity->texture = nullptr;
    entity->nodeCount = 0;
    entity->isHandOrFoot = isHandOrFoot;
    entity->isLeg = isHandOrFoot && shape == Shape::RECTANGLE;
    entity->grabbing = false;
    entity->coreNodeIndex = -1;
    entity->offsetX = 0.0f;
    entity->offsetY = 0.0f;
    entity->rotation = 0.0f;
    entity->grabbedObject = nullptr;

    if (generateNodes) {
        GenerateNodes(entity);
    }
}