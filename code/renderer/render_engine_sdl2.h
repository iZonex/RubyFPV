#pragma once

#include "render_engine.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstdint>
#include <array>
#include <cmath>

#define MAX_RAW_IMAGES 100
#define MAX_RAW_ICONS 50

class RenderEngineSDL2 : public RenderEngine
{
public:
    RenderEngineSDL2();
    virtual ~RenderEngineSDL2();

    virtual uint32_t loadImage(const char* szFile) override;
    virtual void freeImage(uint32_t idImage) override;
    virtual uint32_t loadIcon(const char* szFile) override;
    virtual void freeIcon(uint32_t idIcon) override;
    virtual int getImageWidth(uint32_t uImageId) override;
    virtual int getImageHeight(uint32_t uImageId) override;
    virtual void changeImageHue(uint32_t uImageId, uint8_t r, uint8_t g, uint8_t b) override;

    virtual void startFrame() override;
    virtual void endFrame() override;
    virtual void rotate180() override;

    virtual void drawImage(float xPos, float yPos, float fWidth, float fHeight, uint32_t imageId) override;
    virtual void bltImage(float xPosDest, float yPosDest, float fWidthDest, float fHeightDest,
                          int iSrcX, int iSrcY, int iSrcWidth, int iSrcHeight, uint32_t uImageId) override;
    virtual void drawIcon(float xPos, float yPos, float fWidth, float fHeight, uint32_t iconId) override;
    virtual void bltIcon(float xPosDest, float yPosDest, float fWidthDest, float fHeightDest,
                         int iSrcX, int iSrcY, int iSrcWidth, int iSrcHeight, uint32_t iconId) override;

    virtual void drawLine(float x1, float y1, float x2, float y2) override;
    virtual void drawRect(float xPos, float yPos, float fWidth, float fHeight) override;
    virtual void drawRoundRect(float xPos, float yPos, float fWidth, float fHeight, float fCornerRadius) override;
    virtual void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3) override;
    virtual void fillTriangle(float x1, float y1, float x2, float y2, float x3, float y3) override;
    virtual void drawPolyLine(const float* x, const float* y, int count) override;
    virtual void fillPolygon(const float* x, const float* y, int count) override;

    virtual void fillCircle(float x, float y, float r) override;
    virtual void drawCircle(float x, float y, float r) override;
    virtual void drawArc(float x, float y, float r, float a1, float a2) override;

protected:
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pRenderer = nullptr;

    std::array<SDL_Texture*, MAX_RAW_IMAGES> m_pImageTextures{};
    std::array<uint32_t, MAX_RAW_IMAGES> m_ImageIds{};
    uint32_t m_CurrentImageId = 1;
    int m_iCountImages = 0;

    std::array<SDL_Texture*, MAX_RAW_ICONS> m_pIconTextures{};
    std::array<uint32_t, MAX_RAW_ICONS> m_IconIds{};
    uint32_t m_CurrentIconId = 1;
    int m_iCountIcons = 0;

    int m_iRenderWidth = 0;
    int m_iRenderHeight = 0;
};