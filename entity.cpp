#include "entity.h"
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
    /*
    printf("Absolute to relative: abs_x=%.2f, abs_y=%.2f, entity=(%.2f, %.2f), rotation=%.2f, x_rel=%.2f, y_rel=%.2f\n",
           abs_x, abs_y, cx, cy, entity->rotation, rel.x_rel, rel.y_rel);
    */
    return rel;
}
SDL_FPoint relativeToAbsolute(Entity* entity, NodeRel nodeRel) {
    SDL_FPoint abs;
    float rx = nodeRel.x_rel * (entity->width / 2.0f);
    float ry = nodeRel.y_rel * (entity->height / 2.0f);
    float rot = entity->rotation;
    abs.x = entity->Xpos + rx * cos(rot) - ry * sin(rot);
    abs.y = entity->Ypos + rx * sin(rot) + ry * cos(rot);
    /*
    printf("Relative to absolute: x_rel=%.2f, y_rel=%.2f, entity=(%.2f, %.2f), rotation=%.2f, abs_x=%.2f, abs_y=%.2f\n",
           nodeRel.x_rel, nodeRel.y_rel, entity->Xpos, entity->Ypos, rot, abs.x, abs.y);
    */
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

void generateNodes(Entity* entity) {
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
            entity->nodesRel[0] = {0.0f, -1.0f};  // top
            entity->nodesRel[1] = {-1.0f, 1.0f};  // bottom-left
            entity->nodesRel[2] = {1.0f, 1.0f};   // bottom-right
            entity->nodeCount = 3;
            break;
        }
        default:
            printf("Error: Invalid shapetype %d in generateNodes\n", entity->shapetype);
            return;
    }
    for (int i = 0; i < entity->nodeCount; ++i) {
        SDL_FPoint abs = relativeToAbsolute(entity, entity->nodesRel[i]);
        entity->nodes[i].x = abs.x;
        entity->nodes[i].y = abs.y;
        printf("Generated node %d: x=%.2f, y=%.2f for entity at x=%.2f, y=%.2f, isHandOrFoot=%d\n",
               i, abs.x, abs.y, entity->Xpos, entity->Ypos, entity->isHandOrFoot);
    }
}


void initEntity(Entity* entity, SDL_Renderer* renderer, float Xpos, float Ypos, int width, int height, Shape shape, SDL_Color color, int size, bool isHandOrFoot) {
    if (!entity || !renderer) {
        printf("Error: initEntity called with null entity or renderer\n");
        return;
    }
    entity->Xpos = Xpos;
    entity->Ypos = Ypos;
    entity->Xvel = 0;
    entity->Yvel = 0;
    entity->shapetype = shape;
    entity->color = isHandOrFoot ? SDL_Color{255, 255, 0, 255} : color;
    entity->size = size;
    entity->width = width;
    entity->height = height;
    entity->nodeCount = 0;
    entity->isCore = !isHandOrFoot; // Fix: Only core if not hand/foot
    entity->isHandOrFoot = isHandOrFoot;
    entity->isLeg = isHandOrFoot;
    entity->coreNodeIndex = -1;
    entity->offsetX = 0.0f;
    entity->offsetY = (shape == TRIANGLE) ? width / 2.0f : height / 2.0f;
    entity->rotation = 0.0f;
    entity->appendages.clear();
    entity->texture = nullptr;
    for (int i = 0; i < MAX_NODES; ++i) {
        entity->nodesRel[i] = {0.0f, 0.0f};
        entity->nodes[i] = {0.0f, 0.0f};
    }

    SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        printf("SDL_CreateSurface Error: %s\n", SDL_GetError());
        return;
    }
    const SDL_PixelFormatDetails* formatDetails = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
    if (!formatDetails) {
        printf("SDL_GetPixelFormatDetails Error: %s\n", SDL_GetError());
        SDL_DestroySurface(surface);
        return;
    }
    Uint32 transparent = SDL_MapRGBA(formatDetails, nullptr, 0, 0, 0, 0);
    SDL_FillSurfaceRect(surface, nullptr, transparent);
    Uint32 colorValue = SDL_MapRGBA(formatDetails, nullptr, entity->color.r, entity->color.g, entity->color.b, entity->color.a);
    if (shape == RECTANGLE) {
        SDL_FillSurfaceRect(surface, nullptr, colorValue);
    } else if (shape == CIRCLE) {
        int radius = width / 2;
        int cx = width / 2;
        int cy = height / 2;
        Uint32* pixels = (Uint32*)surface->pixels;
        int pitch = surface->pitch / sizeof(Uint32);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float dx = (x - cx) / (float)radius;
                float dy = (y - cy) / (float)radius;
                if (dx * dx + dy * dy <= 1.0f) {
                    pixels[y * pitch + x] = colorValue;
                }
            }
        }
    } else if (shape == TRIANGLE) {
        SDL_Point points[3] = {{width / 2, 0}, {0, height}, {width, height}};
        Uint32* pixels = (Uint32*)surface->pixels;
        int pitch = surface->pitch / sizeof(Uint32);
        for (int y = 0; y < height; y++) {
            float t = y / (float)height;
            int x1 = (1 - t) * points[0].x + t * points[1].x;
            int x2 = (1 - t) * points[0].x + t * points[2].x;
            if (x1 > x2) std::swap(x1, x2);
            for (int x = x1; x <= x2; x++) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    pixels[y * pitch + x] = colorValue;
                }
            }
        }
    }
    entity->texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!entity->texture) {
        printf("SDL_CreateTextureFromSurface Error: %s\n", SDL_GetError());
    }
    SDL_DestroySurface(surface);

    generateNodes(entity);
    printf("Initialized entity: shape=%d, x=%.2f, y=%.2f, width=%d, height=%d, isHandOrFoot=%d, nodes=%d, texture=%p\n",
           shape, Xpos, Ypos, width, height, isHandOrFoot, entity->nodeCount, entity->texture);
}

void drawFilledCircle(SDL_Renderer* renderer, int Xpos, int Ypos, int radius, SDL_Color color, float rotation) {
    const int segments = 32;
    SDL_Vertex vertices[segments];
    float angleStep = 2.0f * M_PI / segments;for (int i = 0; i < segments; ++i) {
    float angle = i * angleStep;
    vertices[i].position.x = Xpos + radius * cos(angle);
    vertices[i].position.y = Ypos + radius * sin(angle);
    vertices[i].color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
}

int indices[segments * 3];
for (int i = 0; i < segments; ++i) {
    indices[i * 3] = 0; // Center (we'll use the first vertex as center)
    indices[i * 3 + 1] = i;
    indices[i * 3 + 2] = (i + 1) % segments;
}

SDL_RenderGeometry(renderer, NULL, vertices, segments, indices, segments * 3);
/*
printf("Drawing circle at x=%d, y=%d, radius=%d with %d vertices\n", Xpos, Ypos, radius, segments);
*/
}

void drawFilledTriangle(SDL_Renderer* renderer, SDL_Point p1, SDL_Point p2, SDL_Point p3, SDL_Color color, float rotation) {
    float cx = (p1.x + p2.x + p3.x) / 3.0f;
    float cy = (p1.y + p2.y + p3.y) / 3.0f;
    SDL_Vertex vertices[3];SDL_FPoint points[3] = {{(float)p1.x, (float)p1.y}, {(float)p2.x, (float)p2.y}, {(float)p3.x, (float)p3.y}};
for (int i = 0; i < 3; ++i) {
    float dx = points[i].x - cx;
    float dy = points[i].y - cy;
    vertices[i].position.x = cx + dx * cos(rotation) - dy * sin(rotation);
    vertices[i].position.y = cy + dx * sin(rotation) + dy * cos(rotation);
    vertices[i].color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
}

int indices[3] = {0, 1, 2};
SDL_RenderGeometry(renderer, NULL, vertices, 3, indices, 3);
}

SDL_FPoint projectToSegment(SDL_FPoint p, SDL_FPoint v1, SDL_FPoint v2) {
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float lengthSquared = dx * dx + dy * dy;
    if (lengthSquared == 0.0f) return v1;
    float t = ((p.x - v1.x) * dx + (p.y - v1.y) * dy) / lengthSquared;
    t = std::clamp(t, 0.0f, 1.0f);
    SDL_FPoint closest;
    closest.x = v1.x + t * dx;
    closest.y = v1.y + t * dy;
    return closest;
}

float distanceSquared(SDL_FPoint p1, SDL_FPoint p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    return dx * dx + dy * dy;
}

NodeRel clampRelativeNodeToShape(NodeRel rel, Entity* entity) {
    NodeRel clamped = rel;
    if (entity->shapetype == RECTANGLE) {
        clamped.x_rel = std::clamp(rel.x_rel, -1.0f, 1.0f);
        clamped.y_rel = std::clamp(rel.y_rel, -1.0f, 1.0f);
        /*
        printf("Clamping rectangle node: input x_rel=%.2f, y_rel=%.2f, clamped x_rel=%.2f, y_rel=%.2f\n",
               rel.x_rel, rel.y_rel, clamped.x_rel, clamped.y_rel);*/
    } else if (entity->shapetype == CIRCLE) {
        float dist = std::sqrt(rel.x_rel * rel.x_rel + rel.y_rel * rel.y_rel);
        if (dist > 1.0f && dist > 0.0001f) {
            clamped.x_rel = rel.x_rel / dist;
            clamped.y_rel = rel.y_rel / dist;
        }/*
        printf("Clamping circle node: input x_rel=%.2f, y_rel=%.2f, dist=%.2f, clamped x_rel=%.2f, y_rel=%.2f\n",
               rel.x_rel, rel.y_rel, dist, clamped.x_rel, clamped.y_rel);*/
    } else if (entity->shapetype == TRIANGLE) {
        SDL_FPoint p = relativeToAbsolute(entity, rel);
        SDL_FPoint v1 = {entity->Xpos, entity->Ypos - entity->width / 2.0f};
        SDL_FPoint v2 = {entity->Xpos - entity->width / 2.0f, entity->Ypos + entity->width / 2.0f};
        SDL_FPoint v3 = {entity->Xpos + entity->width / 2.0f, entity->Ypos + entity->width / 2.0f};
        float denom = (v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y);
        if (denom == 0.0f) {
            printf("Triangle clamp failed: degenerate triangle\n");
            return absoluteToRelative(entity, v1.x, v1.y);
        }
        float a = ((v2.y - v3.y) * (p.x - v3.x) + (v3.x - v2.x) * (p.y - v3.y)) / denom;
        float b = ((v3.y - v1.y) * (p.x - v3.x) + (v1.x - v3.x) * (p.y - v3.y)) / denom;
        float c = 1.0f - a - b;
        if (a >= 0.0f && b >= 0.0f && c >= 0.0f) {/*
            printf("Triangle node inside: x_rel=%.2f, y_rel=%.2f\n", rel.x_rel, rel.y_rel);*/
            return rel;
        }
        SDL_FPoint projections[3];
        projections[0] = projectToSegment(p, v1, v2);
        projections[1] = projectToSegment(p, v2, v3);
        projections[2] = projectToSegment(p, v3, v1);
        int closestIdx = 0;
        float minDist = distanceSquared(p, projections[0]);
        for (int i = 1; i < 3; ++i) {
            float dist = distanceSquared(p, projections[i]);
            if (dist < minDist) {
                minDist = dist;
                closestIdx = i;
            }
        }
        clamped = absoluteToRelative(entity, projections[closestIdx].x, projections[closestIdx].y);
        printf("Clamping triangle node: input x_rel=%.2f, y_rel=%.2f, clamped x_rel=%.2f, y_rel=%.2f\n",
               rel.x_rel, rel.y_rel, clamped.x_rel, clamped.y_rel);
    }
    return clamped;
}

SDL_FPoint clampNodeToShape(SDL_FPoint pt, Entity* entity) {
    NodeRel rel = absoluteToRelative(entity, pt.x, pt.y);
    rel = clampRelativeNodeToShape(rel, entity);
    return relativeToAbsolute(entity, rel);
}

void switchShape(Entity* entity, Shape newShape) {
    entity->shapetype = newShape;
    if (newShape == RECTANGLE) {
        entity->width = 200;
        entity->height = 200;
    } else if (newShape == CIRCLE) {
        entity->width = 200;
        entity->height = 200;
    } else if (newShape == TRIANGLE) {
        entity->width = 150;
        entity->height = 150;
    }
    for (int i = 0; i < entity->nodeCount; ++i) {
        entity->nodesRel[i] = clampRelativeNodeToShape(entity->nodesRel[i], entity);
        SDL_FPoint abs = relativeToAbsolute(entity, entity->nodesRel[i]);
        entity->nodes[i].x = abs.x;
        entity->nodes[i].y = abs.y;
    }
    updateAppendagePositions(entity);
}

bool isEntityOnGround(Entity* entity, float groundY = 700.0f) {
    if (!entity->isHandOrFoot || entity->shapetype != RECTANGLE) return false;
    float hw = entity->width / 2.0f;
    float hh = entity->height / 2.0f;
    SDL_FPoint corners[4] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
    float bottomY = entity->Ypos;
    for (int i = 0; i < 4; ++i) {
        float rx = corners[i].x * cos(entity->rotation) - corners[i].y * sin(entity->rotation);
        float ry = corners[i].x * sin(entity->rotation) + corners[i].y * cos(entity->rotation);
        float absY = entity->Ypos + ry;
        bottomY = std::max(bottomY, absY);
    }
    bool onGround = bottomY >= groundY - 5.0f; // Increased tolerance to 5.0f
    printf("Checking if entity is on ground: x=%.2f, y=%.2f, bottomY=%.2f, groundY=%.2f, rotation=%.2f, onGround=%d\n",
           entity->Xpos, entity->Ypos, bottomY, groundY, entity->rotation, onGround);
    return onGround;
}

void updateAppendagePositions(Entity* entity) {
    if (!entity) {
        printf("Error: updateAppendagePositions called with null entity\n");
        return;
    }
    generateNodes(entity);
    for (auto& app : entity->appendages) {
        if (!app) {
            printf("Error: null appendage in updateAppendagePositions\n");
            continue;
        }
        if (app->coreNodeIndex >= 0 && app->coreNodeIndex < entity->nodeCount) {
            float nodeX = entity->nodes[app->coreNodeIndex].x;
            float nodeY = entity->nodes[app->coreNodeIndex].y;
            // Use appendage's rotation, as in old version
            float desiredX = nodeX + app->offsetX * cos(app->rotation) - app->offsetY * sin(app->rotation);
            float desiredY = nodeY + app->offsetX * sin(app->rotation) + app->offsetY * cos(app->rotation);

            if (app->isHandOrFoot && app->isLeg) {
                // Keep new version's dynamic maxDistance and offset updates
                float dx = desiredX - nodeX;
                float dy = desiredY - nodeY;
                float distance = sqrt(dx * dx + dy * dy);
                float maxDistance = app->height / 2.0f + 10.0f; // Dynamic max distance
                if (distance > maxDistance && distance > 0.0001f) {
                    float scale = maxDistance / distance;
                    desiredX = nodeX + dx * scale;
                    desiredY = nodeY + dy * scale;
                    // Update offsets to reflect constrained position
                    float rot = -app->rotation; // Use appendage's rotation
                    app->offsetX = (desiredX - nodeX) * cos(rot) - (desiredY - nodeY) * sin(rot);
                    app->offsetY = (desiredX - nodeX) * sin(rot) + (desiredY - nodeY) * cos(rot);
                }
            }

            app->Xpos = desiredX;
            app->Ypos = desiredY;
            app->onGround = isEntityOnGround(app.get());
            updateAppendagePositions(app.get());
            printf("Updated appendage: x=%.2f, y=%.2f, coreNode=%d, isHandOrFoot=%d, isLeg=%d, onGround=%d, parentX=%.2f, parentY=%.2f, nodes=%d\n",
                   app->Xpos, app->Ypos, app->coreNodeIndex, app->isHandOrFoot, app->isLeg, app->onGround,
                   entity->Xpos, entity->Ypos, app->nodeCount);
        } else {
            printf("Invalid coreNodeIndex %d for appendage, nodeCount=%d\n",
                   app->coreNodeIndex, entity->nodeCount);
        }
    }
}

bool addNodeToEntity(Entity* entity, float mouseX, float mouseY) {
    if (!entity) {
        printf("Error: addNodeToEntity called with null entity\n");
        return false;
    }
    if (entity->nodeCount >= MAX_NODES) {
        printf("Cannot add node: MAX_NODES (%d) reached\n", MAX_NODES);
        return false;
    }
    printf("Checking point in entity shape: x=%.2f, y=%.2f, entity x=%.2f, y=%.2f, shape=%d, rotation=%.2f\n",
           mouseX, mouseY, entity->Xpos, entity->Ypos, entity->shapetype, entity->rotation);
    if (!pointInEntityShape(mouseX, mouseY, entity)) {
        printf("Failed to add node: not in entity shape, x=%.2f, y=%.2f\n", mouseX, mouseY);
        for (auto& app : entity->appendages) {
            if (addNodeToEntity(app.get(), mouseX, mouseY)) {
                return true;
            }
        }
        return false;
    }
    NodeRel rel = absoluteToRelative(entity, mouseX, mouseY);
    rel = clampRelativeNodeToShape(rel, entity);
    SDL_FPoint abs = relativeToAbsolute(entity, rel);
    // Skip second pointInEntityShape check to avoid strictness
    entity->nodesRel[entity->nodeCount] = rel;
    entity->nodes[entity->nodeCount].x = abs.x;
    entity->nodes[entity->nodeCount].y = abs.y;
    entity->nodeCount++;
    printf("Added node %d at x=%.2f, y=%.2f\n", entity->nodeCount - 1, abs.x, abs.y);
    updateAppendagePositions(entity);
    return true;
}

bool shouldRemoveAppendage(const std::unique_ptr<Entity>& entity) {
    return entity && entity->coreNodeIndex == -1;
}

void removeNodeFromEntity(Entity* entity, float mouseX, float mouseY) {
    if (!entity) {
        printf("Error: removeNodeFromEntity called with null entity\n");
        return;
    }
    printf("Checking remove node: x=%.2f, y=%.2f, nodeCount=%d\n", mouseX, mouseY, entity->nodeCount);
    for (int i = 0; i < entity->nodeCount; i++) {
        float dx = mouseX - entity->nodes[i].x;
        float dy = mouseY - entity->nodes[i].y;
        float distance = dx * dx + dy * dy;
        printf("Node %d: x=%.2f, y=%.2f, distance=%.2f\n", i, entity->nodes[i].x, entity->nodes[i].y, distance);
        if (distance <= 100.0f) { // Relaxed to 10 pixels
            printf("Removing node %d at x=%.2f, y=%.2f\n", i, entity->nodes[i].x, entity->nodes[i].y);
            for (int j = i; j < entity->nodeCount - 1; ++j) {
                entity->nodes[j] = entity->nodes[j + 1];
                entity->nodesRel[j] = entity->nodesRel[j + 1];
            }
            entity->nodeCount--;
            for (auto& app : entity->appendages) {
                if (app->coreNodeIndex > i) {
                    app->coreNodeIndex--;
                } else if (app->coreNodeIndex == i) {
                    app->coreNodeIndex = -1;
                }
            }
            entity->appendages.erase(
                std::remove_if(entity->appendages.begin(), entity->appendages.end(), shouldRemoveAppendage),
                entity->appendages.end());
            updateAppendagePositions(entity);
            return;
        }
    }
    for (auto& app : entity->appendages) {
        removeNodeFromEntity(app.get(), mouseX, mouseY);
    }
}

void collectEntityGeometry(Entity* entity, std::vector<RenderBatch>& batches) {
    RenderBatch* batch = nullptr;
    for (auto& b : batches) {
        if (b.shapeType == entity->shapetype) {
            batch = &b;
            break;
        }
    }
    if (!batch) {
        batches.push_back({{}, {}, entity->shapetype});
        batch = &batches.back();
    }int baseIndex = batch->vertices.size();
SDL_Color color = entity->isHandOrFoot ? SDL_Color{255, 255, 0, 255} : entity->color;
float cx = entity->Xpos;
float cy = entity->Ypos;
float rot = entity->rotation;

if (entity->shapetype == RECTANGLE) {
    float hw = entity->width / 2.0f;
    float hh = entity->height / 2.0f;
    SDL_FPoint points[4] = {{-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}};
    for (int i = 0; i < 4; ++i) {
        SDL_Vertex v;
        v.position.x = cx + points[i].x * cos(rot) - points[i].y * sin(rot);
        v.position.y = cy + points[i].x * sin(rot) + points[i].y * cos(rot);
        v.color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        batch->vertices.push_back(v);
    }
    int indices[6] = {0, 1, 2, 2, 3, 0};
    for (int idx : indices) batch->indices.push_back(baseIndex + idx);
} else if (entity->shapetype == CIRCLE) {
    const int segments = 32;
    float radius = entity->width / 2.0f;
    float angleStep = 2.0f * M_PI / segments;
    SDL_Vertex center;
    center.position = {cx, cy};
    center.color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
    batch->vertices.push_back(center);
    for (int i = 0; i < segments; ++i) {
        SDL_Vertex v;
        float angle = i * angleStep;
        v.position.x = cx + radius * cos(angle);
        v.position.y = cy + radius * sin(angle);
        v.color = center.color;
        batch->vertices.push_back(v);
        if (i < segments - 1) {
            batch->indices.push_back(baseIndex);
            batch->indices.push_back(baseIndex + i + 1);
            batch->indices.push_back(baseIndex + i + 2);
        }
    }
    batch->indices.push_back(baseIndex);
    batch->indices.push_back(baseIndex + segments);
    batch->indices.push_back(baseIndex + 1);
} else if (entity->shapetype == TRIANGLE) {
    float s = entity->width / 2.0f;
    SDL_FPoint points[3] = {{0, -s}, {-s, s}, {s, s}};
    for (int i = 0; i < 3; ++i) {
        SDL_Vertex v;
        v.position.x = cx + points[i].x * cos(rot) - points[i].y * sin(rot);
        v.position.y = cy + points[i].x * sin(rot) + points[i].y * cos(rot);
        v.color = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        batch->vertices.push_back(v);
    }
    batch->indices.push_back(baseIndex);
    batch->indices.push_back(baseIndex + 1);
    batch->indices.push_back(baseIndex + 2);
}

for (auto& app : entity->appendages) {
    collectEntityGeometry(app.get(), batches);
}}

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

void drawEntity(SDL_Renderer* renderer, Entity* entity) {
    int Xpos = (int)entity->Xpos;
    int Ypos = (int)entity->Ypos;
    int width = entity->width;
    int height = entity->height;
    float rot = entity->rotation;
    SDL_Color color = entity->isHandOrFoot ? SDL_Color{255, 255, 0, 255} : entity->color; // Yellow for hands/feet
    printf("Drawing entity: shape=%d, x=%d, y=%d, w=%d, h=%d, rotation=%.2f, isHandOrFoot=%d\n",
           entity->shapetype, Xpos, Ypos, width, height, rot, entity->isHandOrFoot);
    if (entity->texture) {
        SDL_FRect dst = {entity->Xpos - entity->width / 2.0f, entity->Ypos - entity->height / 2.0f, 
                        (float)entity->width, (float)entity->height};
        SDL_RenderTextureRotated(renderer, entity->texture, nullptr, &dst, 
                         entity->rotation * 180.0f / M_PI, nullptr, SDL_FLIP_NONE);
        printf("Drawing entity with texture at x=%.2f, y=%.2f, w=%d, h=%d, rotation=%.2f\n",
               entity->Xpos, entity->Ypos, entity->width, entity->height, entity->rotation);
    }
    else {
    switch (entity->shapetype) {
        case RECTANGLE: {
            SDL_Vertex vertices[4];
            float hw = width / 2.0f;
            float hh = height / 2.0f;
            float cx = Xpos;
            float cy = Ypos;
            SDL_FPoint points[4] = {
                {-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}
            };
            for (int i = 0; i < 4; ++i) {
                float rx = points[i].x * cos(rot) - points[i].y * sin(rot);
                float ry = points[i].x * sin(rot) + points[i].y * cos(rot);
                vertices[i].position.x = cx + rx;
                vertices[i].position.y = cy + ry;
                vertices[i].color.r = color.r / 255.0f;
                vertices[i].color.g = color.g / 255.0f;
                vertices[i].color.b = color.b / 255.0f;
                vertices[i].color.a = color.a / 255.0f;
            }
            int indices[6] = {0, 1, 2, 2, 3, 0};
            SDL_RenderGeometry(renderer, NULL, vertices, 4, indices, 6);
            break;
        }
        case CIRCLE: {
            drawFilledCircle(renderer, Xpos, Ypos, width / 2, color, 0.0f);
            break;
        }
        case TRIANGLE: {
            int s = entity->width;
            SDL_Point p1 = {Xpos, Ypos - s / 2};
            SDL_Point p2 = {Xpos - s / 2, Ypos + s / 2};
            SDL_Point p3 = {Xpos + s / 2, Ypos + s / 2};
            drawFilledTriangle(renderer, p1, p2, p3, color, rot);
            break;
        }
    }
}
}

void drawEntityWithNodesAndLines(SDL_Renderer* renderer, Entity* entity) {
    std::vector<RenderBatch> batches;
    collectEntityGeometry(entity, batches);for (const auto& batch : batches) {
    SDL_RenderGeometry(renderer, NULL, batch.vertices.data(), batch.vertices.size(),
                       batch.indices.data(), batch.indices.size());
   /* printf("Rendered batch for shape %d: %zu vertices, %zu indices\n",
           batch.shapeType, batch.vertices.size(), batch.indices.size());*/
}

// Draw nodes
for (int i = 0; i < entity->nodeCount; i++) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_FRect nodeRect = {entity->nodes[i].x - 3, entity->nodes[i].y - 3, 6, 6};
    SDL_RenderFillRect(renderer, &nodeRect);
}

// Draw lines to appendages
for (auto& app : entity->appendages) {
    if (app->coreNodeIndex >= 0 && app->coreNodeIndex < entity->nodeCount) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_FPoint coreNode = {entity->nodes[app->coreNodeIndex].x, entity->nodes[app->coreNodeIndex].y};
        SDL_FPoint appEdge = {app->Xpos, app->Ypos - app->height / 2.0f};
        SDL_RenderLine(renderer, coreNode.x, coreNode.y, appEdge.x, appEdge.y);
        drawEntityWithNodesAndLines(renderer, app.get());
    }
}}

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