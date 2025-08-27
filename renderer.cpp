#include "Renderer.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void Renderer::drawFilledCircle(int cx, int cy, int radius, SDL_Color color, float rotation) {
    const int sides = 32;
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    float angleStep = 2.0f * M_PI / sides;

    // Center vertex
    vertices.push_back({{static_cast<float>(cx), static_cast<float>(cy)},
                        {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
                        {0.5f, 0.5f}});

    // Perimeter vertices
    for (int i = 0; i <= sides; ++i) {
        float angle = i * angleStep + rotation;
        float x = cx + radius * cos(angle);
        float y = cy + radius * sin(angle);
        vertices.push_back({{x, y},
                            {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
                            {0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)}});
    }

    // Indices for fan
    for (int i = 1; i <= sides; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    renderGeometry(vertices.data(), vertices.size(), indices.data(), indices.size());
}

void Renderer::drawFilledTriangle(SDL_Point p1, SDL_Point p2, SDL_Point p3, SDL_Color color, float rotation) {
    // Rotate points around centroid if needed
    if (rotation != 0.0f) {
        float cx = (p1.x + p2.x + p3.x) / 3.0f;
        float cy = (p1.y + p2.y + p3.y) / 3.0f;
        auto rotatePoint = [&](SDL_Point& pt) {
            float dx = pt.x - cx;
            float dy = pt.y - cy;
            pt.x = static_cast<int>(cx + dx * cos(rotation) - dy * sin(rotation));
            pt.y = static_cast<int>(cy + dx * sin(rotation) + dy * cos(rotation));
        };
        rotatePoint(p1);
        rotatePoint(p2);
        rotatePoint(p3);
    }

    SDL_Vertex vertices[3] = {
        {{static_cast<float>(p1.x), static_cast<float>(p1.y)}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}},
        {{static_cast<float>(p2.x), static_cast<float>(p2.y)}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}},
        {{static_cast<float>(p3.x), static_cast<float>(p3.y)}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}}
    };
    int indices[3] = {0, 1, 2};
    renderGeometry(vertices, 3, indices, 3);
}

void Renderer::drawEntity(Entity* entity) {
    int Xpos = (int)entity->Xpos;
    int Ypos = (int)entity->Ypos;
    int width = entity->width;
    int height = entity->height;
    float rot = entity->rotation;
    SDL_Color color = entity->isHandOrFoot ? SDL_Color{255, 255, 0, 255} : entity->color; // Yellow for hands/feet
    if (entity->texture) {
        SDL_FRect dst = {entity->Xpos - entity->width / 2.0f, entity->Ypos - entity->height / 2.0f, 
                        (float)entity->width, (float)entity->height};
        renderTextureRotated(entity->texture, &dst, entity->rotation * 180.0f / M_PI, nullptr, SDL_FLIP_NONE);
    } else {
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
                renderGeometry(vertices, 4, indices, 6);
                break;
            }
            case CIRCLE: {
                drawFilledCircle(Xpos, Ypos, width / 2, color, 0.0f);
                break;
            }
            case TRIANGLE: {
                int s = entity->width;
                SDL_Point p1 = {Xpos, Ypos - s / 2};
                SDL_Point p2 = {Xpos - s / 2, Ypos + s / 2};
                SDL_Point p3 = {Xpos + s / 2, Ypos + s / 2};
                drawFilledTriangle(p1, p2, p3, color, rot);
                break;
            }
        }
    }
}

void Renderer::drawEntityWithNodesAndLines(Entity* entity) {
    std::vector<RenderBatch> batches;
    collectEntityGeometry(entity, batches);
    for (const auto& batch : batches) {
        renderGeometry(batch.vertices.data(), batch.vertices.size(), batch.indices.data(), batch.indices.size());
    }

    // Draw nodes
    setDrawColor({255, 255, 0, 255});
    for (int i = 0; i < entity->nodeCount; i++) {
        SDL_FRect nodeRect = {entity->nodes[i].x - 3, entity->nodes[i].y - 3, 6, 6};
        fillRect(&nodeRect);
    }

    // Draw lines to appendages
    for (auto& app : entity->appendages) {
        if (app->coreNodeIndex >= 0 && app->coreNodeIndex < entity->nodeCount) {
            setDrawColor({255, 255, 255, 255});
            SDL_FPoint coreNode = {entity->nodes[app->coreNodeIndex].x, entity->nodes[app->coreNodeIndex].y};
            SDL_FPoint appEdge = {app->Xpos, app->Ypos - app->height / 2.0f};
            drawLine(coreNode.x, coreNode.y, appEdge.x, appEdge.y);
            drawEntityWithNodesAndLines(app.get());
        }
    }
}

void collectEntityGeometry(Entity* entity, std::vector<RenderBatch>& batches) {
    RenderBatch batch;
    batch.shapeType = entity->shapetype;
    int baseIndex = 0;

    SDL_Color color = entity->isHandOrFoot ? SDL_Color{255, 255, 0, 255} : entity->color;

    switch (entity->shapetype) {
        case RECTANGLE: {
            float hw = entity->width / 2.0f;
            float hh = entity->height / 2.0f;
            float cx = entity->Xpos;
            float cy = entity->Ypos;
            float rot = entity->rotation;
            SDL_FPoint points[4] = {
                {-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}
            };
            for (int i = 0; i < 4; ++i) {
                float rx = points[i].x * cos(rot) - points[i].y * sin(rot);
                float ry = points[i].x * sin(rot) + points[i].y * cos(rot);
                SDL_Vertex v = {{cx + rx, cy + ry}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}};
                batch.vertices.push_back(v);
            }
            batch.indices = {0, 1, 2, 2, 3, 0};
            break;
        }
        case CIRCLE: {
            int radius = entity->width / 2;
            int cx = static_cast<int>(entity->Xpos);
            int cy = static_cast<int>(entity->Ypos);
            const int sides = 32;
            float angleStep = 2.0f * M_PI / sides;

            // Center vertex
            batch.vertices.push_back({{static_cast<float>(cx), static_cast<float>(cy)},
                                      {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
                                      {0.5f, 0.5f}});

            // Perimeter vertices
            for (int i = 0; i <= sides; ++i) {
                float angle = i * angleStep + entity->rotation;
                float x = cx + radius * cos(angle);
                float y = cy + radius * sin(angle);
                batch.vertices.push_back({{x, y},
                                          {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f},
                                          {0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)}});
            }

            // Indices for fan
            for (int i = 1; i <= sides; ++i) {
                batch.indices.push_back(0);
                batch.indices.push_back(i);
                batch.indices.push_back(i + 1);
            }
            break;
        }
        case TRIANGLE: {
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
            SDL_Vertex v1 = {{rp1.x, rp1.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}};
            SDL_Vertex v2 = {{rp2.x, rp2.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}};
            SDL_Vertex v3 = {{rp3.x, rp3.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}};
            batch.vertices = {v1, v2, v3};
            batch.indices = {0, 1, 2};
            break;
        }
    }
    batches.push_back(batch);

    for (auto& app : entity->appendages) {
        collectEntityGeometry(app.get(), batches);
    }
}