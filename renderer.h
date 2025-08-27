#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <vector>
#include "entity.h" // For Entity, Shape, SDL_Color, etc.

struct RenderBatch {
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    Shape shapeType;
};

class Renderer {
private:
    SDL_Renderer* sdl_renderer_;

public:
    Renderer(SDL_Renderer* sdl_renderer) : sdl_renderer_(sdl_renderer) {}

    SDL_Renderer* getSDLRenderer() const { return sdl_renderer_; }

    void setDrawColor(SDL_Color color) {
        SDL_SetRenderDrawColor(sdl_renderer_, color.r, color.g, color.b, color.a);
    }

    void clear(SDL_Color color) {
        setDrawColor(color);
        SDL_RenderClear(sdl_renderer_);
    }

    void present() {
        SDL_RenderPresent(sdl_renderer_);
    }

    void drawLine(float x1, float y1, float x2, float y2) {
        SDL_RenderLine(sdl_renderer_, x1, y1, x2, y2);
    }

    void renderGeometry(const SDL_Vertex* vertices, int num_vertices, const int* indices, int num_indices) {
        SDL_RenderGeometry(sdl_renderer_, nullptr, vertices, num_vertices, indices, num_indices);
    }

    void renderTextureRotated(SDL_Texture* texture, const SDL_FRect* dst, double angle, const SDL_FPoint* point, SDL_FlipMode flip) {
        SDL_RenderTextureRotated(sdl_renderer_, texture, nullptr, dst, angle, point, flip);
    }

    void drawRect(const SDL_FRect* rect) {
        SDL_RenderRect(sdl_renderer_, rect);
    }

    void fillRect(const SDL_FRect* rect) {
        SDL_RenderFillRect(sdl_renderer_, rect);
    }

 SDL_Texture* createTexture(SDL_PixelFormat format, SDL_TextureAccess access, int w, int h) {
        return SDL_CreateTexture(sdl_renderer_, format, access, w, h);
    }

    void setTextureBlendMode(SDL_Texture* texture, SDL_BlendMode blendMode) {
        SDL_SetTextureBlendMode(texture, blendMode);
    }

    void setTextureScaleMode(SDL_Texture* texture, SDL_ScaleMode scaleMode) {
        SDL_SetTextureScaleMode(texture, scaleMode);
    }

    void setRenderTarget(SDL_Texture* texture) {
        SDL_SetRenderTarget(sdl_renderer_, texture);
    }

    void drawFilledCircle(int cx, int cy, int radius, SDL_Color color, float rotation);
    void drawFilledTriangle(SDL_Point p1, SDL_Point p2, SDL_Point p3, SDL_Color color, float rotation);
    void drawEntity(Entity* entity);
    void drawEntityWithNodesAndLines(Entity* entity);
};

void collectEntityGeometry(Entity* entity, std::vector<RenderBatch>& batches);

#endif // RENDERER_H