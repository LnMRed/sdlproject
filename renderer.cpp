#include "Renderer.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void Renderer::collectLineGeometry(float x1, float y1, float x2, float y2, SDL_Color color, RenderData& data, float thickness) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = std::sqrt(dx * dx + dy * dy); // moet sneller kunnen
    if (length == 0) return;

    // normalize vector
    float nx = dx / length;
    float ny = dy / length;
    float half_thickness = thickness / 2.0f;

    //hier gaan we de quad defineren
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    float a = color.a / 255.0f;

    SDL_Vertex v1 = {{x1 + nx * half_thickness, y1 + nx * half_thickness}, {r, g, b, a}, {0.0f, 0.0f}};
    SDL_Vertex v2 = {{x1 - nx * half_thickness, y1 - nx * half_thickness}, {r, g, b, a}, {0.0f, 0.0f}};
    SDL_Vertex v3 = {{x2 - nx * half_thickness, y2 - nx * half_thickness}, {r, g, b, a}, {0.0f, 0.0f}};
    SDL_Vertex v4 = {{x2 + nx * half_thickness, y2 + ny * half_thickness}, {r, g, b, a}, {0.0f, 0.0f}};

    int baseIdx = data.vertices.size();
    data.vertices.push_back(v1);
    data.vertices.push_back(v2);
    data.vertices.push_back(v3);
    data.vertices.push_back(v4);

    // append indices for two triangles 0 1 2, 2 3 0
    data.indices.push_back(baseIdx + 0);
    data.indices.push_back(baseIdx + 1);
    data.indices.push_back(baseIdx + 2);
    data.indices.push_back(baseIdx + 2);
    data.indices.push_back(baseIdx + 3);
    data.indices.push_back(baseIdx + 0);
}

void Renderer::collectConnectionLinesGeometry(Entity* entity, RenderData& data, SDL_Color color) {
    for (auto& app : entity->appendages) {
        if (app->coreNodeIndex >= 0 && app->coreNodeIndex < entity->nodeCount) {
            float nodeX = entity->nodes[app->coreNodeIndex].x;
            float nodeY = entity->nodes[app->coreNodeIndex].y;

            float appConnectX = app->Xpos;
            float appConnectY = app->Ypos - app->height / 2.0f;  // Connect to top edge
            if (app->isHandOrFoot) {
                appConnectY = app->Ypos;  // Center for hands/feet
            }
            collectLineGeometry(nodeX, nodeY, appConnectX, appConnectY, color, data , 2.0f); // later thickness parameter beter uitwerken
            collectConnectionLinesGeometry(app.get(), data, color);
        }
    }
}

void Renderer::drawFilledCircle(int cx, int cy, int radius, SDL_Color color, float rotation) {
    const int sides = 32;
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    float angleStep = 2.0f * M_PI / sides;

    vertices.push_back({{(float)cx, (float)cy}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.5f, 0.5f}});

    for (int i = 0; i <= sides; ++i) {
        float angle = i * angleStep + rotation;
        float x = cx + radius * cos(angle);
        float y = cy + radius * sin(angle);
        vertices.push_back({{x, y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)}});
    }

    for (int i = 1; i <= sides; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    renderGeometry(vertices.data(), vertices.size(), indices.data(), indices.size());
}

void Renderer::drawFilledTriangle(SDL_Point p1, SDL_Point p2, SDL_Point p3, SDL_Color color, float rotation) {
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
        {{(float)p1.x, (float)p1.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}},
        {{(float)p2.x, (float)p2.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}},
        {{(float)p3.x, (float)p3.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}}
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
    SDL_Color color = entity->isHandOrFoot ? SDL_Color{255, 255, 0, 255} : entity->color;
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
                float s = width / 2.0f;
                SDL_Point p1 = {(int)Xpos, (int)(Ypos - s)};
                SDL_Point p2 = {(int)(Xpos - s), (int)(Ypos + s)};
                SDL_Point p3 = {(int)(Xpos + s), (int)(Ypos + s)};
                drawFilledTriangle(p1, p2, p3, color, rot);
                break;
            }
        }
    }
}

void Renderer::collectAllGeometry(Entity* rootEntity, RenderData& data, bool includeNodes) {
    if (!rootEntity) return;

    Uint16 baseIndex = static_cast<Uint16>(data.vertices.size());
    SDL_Color color = rootEntity->isHandOrFoot ? SDL_Color{255, 255, 0, 255} : rootEntity->color;

    switch (rootEntity->shapetype) {
        case RECTANGLE: {
            float hw = rootEntity->width / 2.0f;
            float hh = rootEntity->height / 2.0f;
            float cx = rootEntity->Xpos;
            float cy = rootEntity->Ypos;
            float rot = rootEntity->rotation;
            SDL_FPoint points[4] = {
                {-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}
            };
            for (int i = 0; i < 4; ++i) {
                float rx = points[i].x * cos(rot) - points[i].y * sin(rot);
                float ry = points[i].x * sin(rot) + points[i].y * cos(rot);
                data.vertices.push_back({{cx + rx, cy + ry}, 
                                        {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, 
                                        {0.0f, 0.0f}});
            }
            data.indices.insert(data.indices.end(), 
                              {baseIndex, baseIndex + 1, baseIndex + 2, 
                               baseIndex + 2, baseIndex + 3, baseIndex});
            break;
        }
        case CIRCLE: {
            int radius = rootEntity->width / 2;
            int cx = static_cast<int>(rootEntity->Xpos);
            int cy = static_cast<int>(rootEntity->Ypos);
            const int sides = 32;
            float angleStep = 2.0f * M_PI / sides;
            data.vertices.push_back({{(float)cx, (float)cy}, 
                                    {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, 
                                    {0.5f, 0.5f}});
            for (int i = 0; i <= sides; ++i) {
                float angle = i * angleStep + rootEntity->rotation;
                float x = cx + radius * cos(angle);
                float y = cy + radius * sin(angle);
                data.vertices.push_back({{x, y}, 
                                        {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, 
                                        {0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)}});
            }
            for (int i = 1; i <= sides; ++i) {
                data.indices.push_back(baseIndex);
                data.indices.push_back(baseIndex + i);
                data.indices.push_back(baseIndex + (i % sides) + 1);
            }
            break;
        }
        case TRIANGLE: {
            float s = rootEntity->width / 2.0f;
            float cx = rootEntity->Xpos;
            float cy = rootEntity->Ypos;
            float rot = rootEntity->rotation;
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
            data.vertices.push_back({{rp1.x, rp1.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
            data.vertices.push_back({{rp2.x, rp2.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
            data.vertices.push_back({{rp3.x, rp3.y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
            data.indices.insert(data.indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2});
            break;
        }
    }

    if (includeNodes) {
        for (int i = 0; i < rootEntity->nodeCount; ++i) {
            float nx = rootEntity->nodes[i].x;
            float ny = rootEntity->nodes[i].y;
            int nodeRadius = 3;
            Uint16 nodeBase = static_cast<Uint16>(data.vertices.size());
            data.vertices.push_back({{nx, ny}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}});
            const int nodeSides = 8;
            for (int j = 0; j <= nodeSides; ++j) {
                float angle = (2 * M_PI * j) / nodeSides;
                float vx = nx + nodeRadius * cos(angle);
                float vy = ny + nodeRadius * sin(angle);
                data.vertices.push_back({{vx, vy}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)}});
            }
            for (int j = 1; j <= nodeSides; ++j) {
                Uint16 c = nodeBase;
                Uint16 p1 = nodeBase + j;
                Uint16 p2 = nodeBase + (j % nodeSides) + 1;
                data.indices.insert(data.indices.end(), {c, p1, p2});
            }
        }
        SDL_Color lineColor = {255, 255, 255, 255};  // hier pakken we de lijnen van de nodes
        collectConnectionLinesGeometry(rootEntity, data, lineColor);
    }

    for (auto& app : rootEntity->appendages) {
        collectAllGeometry(app.get(), data, includeNodes);
    }
}

void Renderer::collectUIGeometry(const std::vector<InputManager::ShapeButton>& shapeButtons,
                                const std::vector<InputManager::EditModeButton>& editModeButtons,
                                const InputManager::ShapeButton& addNodeBtn,
                                const InputManager::ShapeButton& removeNodeBtn,
                                RenderData& data) {
    auto addRect = [&](const SDL_FRect& rect, SDL_Color color) {
        Uint16 baseIndex = static_cast<Uint16>(data.vertices.size());
        float x = rect.x, y = rect.y, w = rect.w, h = rect.h;
        data.vertices.push_back({{x, y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        data.vertices.push_back({{x + w, y}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        data.vertices.push_back({{x + w, y + h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, {0.0f, 0.0f}});
        data.vertices.push_back({{x, y + h}, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f}, {0.0f, 0.0f}});
        data.indices.insert(data.indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});
    };

    for (const auto& btn : shapeButtons) {
        addRect(btn.rect, btn.color);
    }
    for (const auto& btn : editModeButtons) {
        addRect(btn.rect, btn.color);
    }
    addRect(addNodeBtn.rect, addNodeBtn.color);
    addRect(removeNodeBtn.rect, removeNodeBtn.color);
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

            batch.vertices.push_back({{(float)cx, (float)cy}, 
                                     {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, 
                                     {0.5f, 0.5f}});

            for (int i = 0; i <= sides; ++i) {
                float angle = i * angleStep + entity->rotation;
                float x = cx + radius * cos(angle);
                float y = cy + radius * sin(angle);
                batch.vertices.push_back({{x, y}, 
                                         {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f}, 
                                         {0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)}});
            }

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

void Renderer::drawEntityWithNodesAndLines(Entity* entity) {
    drawEntity(entity);
    setDrawColor({255, 255, 255, 255});
    for (int i = 0; i < entity->nodeCount; ++i) {
        SDL_FRect nodeRect = {entity->nodes[i].x - 3, entity->nodes[i].y - 3, 6, 6};
        fillRect(&nodeRect);
    }
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