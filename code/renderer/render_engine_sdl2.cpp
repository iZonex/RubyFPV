#include "render_engine_sdl2.h"
#include <algorithm>
#include <vector>
#include <cstring>

RenderEngineSDL2::RenderEngineSDL2()
{
    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return;
    }

    // Initialize SDL_image subsystem
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        SDL_Log("Unable to initialize SDL_image: %s", IMG_GetError());
        SDL_Quit();
        return;
    }

    // Get display mode for current screen to determine resolution
    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) != 0)
    {
        SDL_Log("Unable to get current display mode: %s", SDL_GetError());
        SDL_Quit();
        return;
    }

    // Store the screen width and height
    m_iRenderWidth = displayMode.w;
    m_iRenderHeight = displayMode.h;

    // Create a window in fullscreen mode with desktop resolution
    m_pWindow = SDL_CreateWindow("SDL2 Render Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 m_iRenderWidth, m_iRenderHeight, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!m_pWindow)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        SDL_Quit();
        return;
    }

    // Create a renderer for the window
    m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (!m_pRenderer)
    {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(m_pWindow);
        SDL_Quit();
        return;
    }

    // Initialize image and icon arrays
    m_pImageTextures.fill(nullptr);
    m_pIconTextures.fill(nullptr);

    // Log fullscreen mode activation
    SDL_Log("Running in fullscreen mode at resolution: %d x %d", m_iRenderWidth, m_iRenderHeight);
}

RenderEngineSDL2::~RenderEngineSDL2()
{
    // Free all loaded images and icons
    for (auto& texture : m_pImageTextures)
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
    for (auto& texture : m_pIconTextures)
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
    // Destroy renderer and window
    if (m_pRenderer)
    {
        SDL_DestroyRenderer(m_pRenderer);
        m_pRenderer = nullptr;
    }
    if (m_pWindow)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }
    // Quit SDL_image and SDL subsystems
    IMG_Quit();
    SDL_Quit();
}

uint32_t RenderEngineSDL2::loadImage(const char* szFile)
{
    // Load an image from a file and store it in the image array
    if (m_iCountImages >= MAX_RAW_IMAGES)
        return 0;

    SDL_Surface* imgSurface = IMG_Load(szFile);
    if (!imgSurface)
    {
        SDL_Log("Unable to load image %s: %s", szFile, IMG_GetError());
        return 0;
    }

    SDL_Texture* imgTexture = SDL_CreateTextureFromSurface(m_pRenderer, imgSurface);
    SDL_FreeSurface(imgSurface);

    if (!imgTexture)
    {
        SDL_Log("Unable to create texture from %s: %s", szFile, SDL_GetError());
        return 0;
    }

    m_pImageTextures[m_iCountImages] = imgTexture;
    m_ImageIds[m_iCountImages] = m_CurrentImageId++;
    return m_ImageIds[m_iCountImages++];
}

void RenderEngineSDL2::freeImage(uint32_t idImage)
{
    for (int i = 0; i < m_iCountImages; ++i)
    {
        if (m_ImageIds[i] == idImage)
        {
            SDL_DestroyTexture(m_pImageTextures[i]);
            for (int j = i; j < m_iCountImages - 1; ++j)
            {
                m_pImageTextures[j] = m_pImageTextures[j + 1];
                m_ImageIds[j] = m_ImageIds[j + 1];
            }
            m_pImageTextures[m_iCountImages - 1] = nullptr;
            --m_iCountImages;
            return;
        }
    }
}

uint32_t RenderEngineSDL2::loadIcon(const char* szFile)
{
    if (m_iCountIcons >= MAX_RAW_ICONS)
        return 0;

    SDL_Surface* imgSurface = IMG_Load(szFile);
    if (!imgSurface)
    {
        SDL_Log("Unable to load icon %s: %s", szFile, IMG_GetError());
        return 0;
    }

    SDL_Texture* iconTexture = SDL_CreateTextureFromSurface(m_pRenderer, imgSurface);
    SDL_FreeSurface(imgSurface);

    if (!iconTexture)
    {
        SDL_Log("Unable to create texture from %s: %s", szFile, SDL_GetError());
        return 0;
    }

    m_pIconTextures[m_iCountIcons] = iconTexture;
    m_IconIds[m_iCountIcons] = m_CurrentIconId++;
    return m_IconIds[m_iCountIcons++];
}

void RenderEngineSDL2::freeIcon(uint32_t idIcon)
{
    for (int i = 0; i < m_iCountIcons; ++i)
    {
        if (m_IconIds[i] == idIcon)
        {
            SDL_DestroyTexture(m_pIconTextures[i]);
            for (int j = i; j < m_iCountIcons - 1; ++j)
            {
                m_pIconTextures[j] = m_pIconTextures[j + 1];
                m_IconIds[j] = m_IconIds[j + 1];
            }
            m_pIconTextures[m_iCountIcons - 1] = nullptr;
            --m_iCountIcons;
            return;
        }
    }
}

int RenderEngineSDL2::getImageWidth(uint32_t uImageId)
{
    for (int i = 0; i < m_iCountImages; ++i)
    {
        if (m_ImageIds[i] == uImageId)
        {
            int w;
            SDL_QueryTexture(m_pImageTextures[i], NULL, NULL, &w, NULL);
            return w;
        }
    }
    return 0;
}

int RenderEngineSDL2::getImageHeight(uint32_t uImageId)
{
    for (int i = 0; i < m_iCountImages; ++i)
    {
        if (m_ImageIds[i] == uImageId)
        {
            int h;
            SDL_QueryTexture(m_pImageTextures[i], NULL, NULL, NULL, &h);
            return h;
        }
    }
    return 0;
}

void RenderEngineSDL2::changeImageHue(uint32_t uImageId, uint8_t newR, uint8_t newG, uint8_t newB)
{
    // Find the image texture
    for (int i = 0; i < m_iCountImages; ++i)
    {
        if (m_ImageIds[i] == uImageId)
        {
            SDL_Texture* texture = m_pImageTextures[i];

            // Get texture format and size
            Uint32 format;
            int w, h;
            SDL_QueryTexture(texture, &format, NULL, &w, &h);

            // Create a surface to manipulate pixels
            SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, format);
            if (!surface)
            {
                SDL_Log("Unable to create surface: %s", SDL_GetError());
                return;
            }

            // Set the render target to the texture
            SDL_SetRenderTarget(m_pRenderer, texture);

            // Render texture to surface
            if (SDL_RenderReadPixels(m_pRenderer, NULL, format, surface->pixels, surface->pitch) != 0)
            {
                SDL_Log("Unable to read pixels from texture: %s", SDL_GetError());
                SDL_FreeSurface(surface);
                SDL_SetRenderTarget(m_pRenderer, NULL);
                return;
            }

            SDL_SetRenderTarget(m_pRenderer, NULL);

            // Lock the surface
            if (SDL_MUSTLOCK(surface))
                SDL_LockSurface(surface);

            // Manipulate pixel data to change hue
            Uint32* pixels = static_cast<Uint32*>(surface->pixels);
            int pixelCount = w * h;

            for (int j = 0; j < pixelCount; ++j)
            {
                Uint8 r, g, b, a;
                SDL_GetRGBA(pixels[j], surface->format, &r, &g, &b, &a);

                // Convert RGB to HSV
                float fr = r / 255.0f;
                float fg = g / 255.0f;
                float fb = b / 255.0f;

                float max = std::max({ fr, fg, fb });
                float min = std::min({ fr, fg, fb });
                float delta = max - min;

                float h = 0.0f;
                if (delta != 0.0f)
                {
                    if (max == fr)
                        h = fmod(((fg - fb) / delta), 6.0f);
                    else if (max == fg)
                        h = ((fb - fr) / delta) + 2.0f;
                    else
                        h = ((fr - fg) / delta) + 4.0f;
                    h *= 60.0f;
                    if (h < 0.0f)
                        h += 360.0f;
                }

                float s = (max == 0.0f) ? 0.0f : delta / max;
                float v = max;

                // Replace hue with new hue
                h = (newR / 255.0f) * 360.0f;

                // Convert HSV back to RGB
                float c = v * s;
                float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
                float m = v - c;

                float r1, g1, b1;

                if (h >= 0 && h < 60)
                {
                    r1 = c; g1 = x; b1 = 0;
                }
                else if (h >= 60 && h < 120)
                {
                    r1 = x; g1 = c; b1 = 0;
                }
                else if (h >= 120 && h < 180)
                {
                    r1 = 0; g1 = c; b1 = x;
                }
                else if (h >= 180 && h < 240)
                {
                    r1 = 0; g1 = x; b1 = c;
                }
                else if (h >= 240 && h < 300)
                {
                    r1 = x; g1 = 0; b1 = c;
                }
                else
                {
                    r1 = c; g1 = 0; b1 = x;
                }

                Uint8 rFinal = static_cast<Uint8>((r1 + m) * 255);
                Uint8 gFinal = static_cast<Uint8>((g1 + m) * 255);
                Uint8 bFinal = static_cast<Uint8>((b1 + m) * 255);

                pixels[j] = SDL_MapRGBA(surface->format, rFinal, gFinal, bFinal, a);
            }

            if (SDL_MUSTLOCK(surface))
                SDL_UnlockSurface(surface);

            // Update texture with new pixel data
            SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);

            SDL_FreeSurface(surface);
            return;
        }
    }
}

void RenderEngineSDL2::startFrame()
{
    SDL_SetRenderDrawColor(m_pRenderer, 0, 0, 0, 255);
    SDL_RenderClear(m_pRenderer);
}

void RenderEngineSDL2::endFrame()
{
    SDL_RenderPresent(m_pRenderer);
}

void RenderEngineSDL2::rotate180()
{
    // Create a texture to capture the current frame
    SDL_Texture* texture = SDL_CreateTexture(m_pRenderer, SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_TARGET, m_iRenderWidth, m_iRenderHeight);
    if (!texture)
    {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        return;
    }

    // Set the render target to the texture and render the current frame onto it
    SDL_SetRenderTarget(m_pRenderer, texture);
    SDL_RenderReadPixels(m_pRenderer, NULL, SDL_PIXELFORMAT_RGBA8888, nullptr, 0);

    // Lock the texture to access its pixel data
    void* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(texture, NULL, &pixels, &pitch) != 0)
    {
        SDL_Log("Unable to lock texture: %s", SDL_GetError());
        SDL_DestroyTexture(texture);
        return;
    }

    // Perform pixel swapping to rotate the image by 180 degrees
    unsigned char* pixelData = static_cast<unsigned char*>(pixels);
    int bytesPerPixel = 4; // Assuming SDL_PIXELFORMAT_RGBA8888

    for (int y = 0; y < m_iRenderHeight / 2; y++)
    {
        for (int x = 0; x < m_iRenderWidth; x++)
        {
            // Calculate pointers to the pixels to swap
            unsigned char* pixel1 = pixelData + y * pitch + x * bytesPerPixel;
            unsigned char* pixel2 = pixelData + (m_iRenderHeight - y - 1) * pitch + (m_iRenderWidth - x - 1) * bytesPerPixel;

            // Swap the pixels
            unsigned char tempPixel[4];
            memcpy(tempPixel, pixel1, bytesPerPixel);
            memcpy(pixel1, pixel2, bytesPerPixel);
            memcpy(pixel2, tempPixel, bytesPerPixel);
        }
    }

    SDL_UnlockTexture(texture);

    // Reset the render target to the default (the window)
    SDL_SetRenderTarget(m_pRenderer, NULL);

    // Clear the renderer
    SDL_RenderClear(m_pRenderer);

    // Render the rotated texture to the screen
    SDL_RenderCopy(m_pRenderer, texture, NULL, NULL);

    // Destroy the texture
    SDL_DestroyTexture(texture);
}

void RenderEngineSDL2::drawImage(float xPos, float yPos, float fWidth, float fHeight, uint32_t imageId)
{
    for (int i = 0; i < m_iCountImages; ++i)
    {
        if (m_ImageIds[i] == imageId)
        {
            SDL_Rect dest;
            dest.x = static_cast<int>(xPos * m_iRenderWidth);
            dest.y = static_cast<int>(yPos * m_iRenderHeight);
            dest.w = static_cast<int>(fWidth * m_iRenderWidth);
            dest.h = static_cast<int>(fHeight * m_iRenderHeight);

            SDL_RenderCopy(m_pRenderer, m_pImageTextures[i], NULL, &dest);
            break;
        }
    }
}

void RenderEngineSDL2::bltImage(float xPosDest, float yPosDest, float fWidthDest, float fHeightDest,
                                int iSrcX, int iSrcY, int iSrcWidth, int iSrcHeight, uint32_t uImageId)
{
    for (int i = 0; i < m_iCountImages; ++i)
    {
        if (m_ImageIds[i] == uImageId)
        {
            SDL_Rect src;
            src.x = iSrcX;
            src.y = iSrcY;
            src.w = iSrcWidth;
            src.h = iSrcHeight;

            SDL_Rect dest;
            dest.x = static_cast<int>(xPosDest * m_iRenderWidth);
            dest.y = static_cast<int>(yPosDest * m_iRenderHeight);
            dest.w = static_cast<int>(fWidthDest * m_iRenderWidth);
            dest.h = static_cast<int>(fHeightDest * m_iRenderHeight);

            SDL_RenderCopy(m_pRenderer, m_pImageTextures[i], &src, &dest);
            break;
        }
    }
}

void RenderEngineSDL2::drawIcon(float xPos, float yPos, float fWidth, float fHeight, uint32_t iconId)
{
    for (int i = 0; i < m_iCountIcons; ++i)
    {
        if (m_IconIds[i] == iconId)
        {
            SDL_Rect dest;
            dest.x = static_cast<int>(xPos * m_iRenderWidth);
            dest.y = static_cast<int>(yPos * m_iRenderHeight);
            dest.w = static_cast<int>(fWidth * m_iRenderWidth);
            dest.h = static_cast<int>(fHeight * m_iRenderHeight);

            SDL_RenderCopy(m_pRenderer, m_pIconTextures[i], NULL, &dest);
            break;
        }
    }
}

void RenderEngineSDL2::bltIcon(float xPosDest, float yPosDest, float fWidthDest, float fHeightDest,
                               int iSrcX, int iSrcY, int iSrcWidth, int iSrcHeight, uint32_t iconId)
{
    for (int i = 0; i < m_iCountIcons; ++i)
    {
        if (m_IconIds[i] == iconId)
        {
            SDL_Rect src;
            src.x = iSrcX;
            src.y = iSrcY;
            src.w = iSrcWidth;
            src.h = iSrcHeight;

            SDL_Rect dest;
            dest.x = static_cast<int>(xPosDest * m_iRenderWidth);
            dest.y = static_cast<int>(yPosDest * m_iRenderHeight);
            dest.w = static_cast<int>(fWidthDest * m_iRenderWidth);
            dest.h = static_cast<int>(fHeightDest * m_iRenderHeight);

            SDL_RenderCopy(m_pRenderer, m_pIconTextures[i], &src, &dest);
            break;
        }
    }
}

void RenderEngineSDL2::drawLine(float x1, float y1, float x2, float y2)
{
    SDL_SetRenderDrawColor(m_pRenderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(m_pRenderer,
                       static_cast<int>(x1 * m_iRenderWidth),
                       static_cast<int>(y1 * m_iRenderHeight),
                       static_cast<int>(x2 * m_iRenderWidth),
                       static_cast<int>(y2 * m_iRenderHeight));
}

void RenderEngineSDL2::drawRect(float xPos, float yPos, float fWidth, float fHeight)
{
    SDL_Rect rect;
    rect.x = static_cast<int>(xPos * m_iRenderWidth);
    rect.y = static_cast<int>(yPos * m_iRenderHeight);
    rect.w = static_cast<int>(fWidth * m_iRenderWidth);
    rect.h = static_cast<int>(fHeight * m_iRenderHeight);
    SDL_SetRenderDrawColor(m_pRenderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(m_pRenderer, &rect);
}

void RenderEngineSDL2::drawRoundRect(float xPos, float yPos, float fWidth, float fHeight, float fCornerRadius)
{
    // Draw the rounded rectangle using lines and arcs
    float x = xPos * m_iRenderWidth;
    float y = yPos * m_iRenderHeight;
    float w = fWidth * m_iRenderWidth;
    float h = fHeight * m_iRenderHeight;
    float r = fCornerRadius * m_iRenderWidth;

    // Draw sides
    SDL_RenderDrawLine(m_pRenderer, x + r, y, x + w - r, y);
    SDL_RenderDrawLine(m_pRenderer, x + r, y + h, x + w - r, y + h);
    SDL_RenderDrawLine(m_pRenderer, x, y + r, x, y + h - r);
    SDL_RenderDrawLine(m_pRenderer, x + w, y + r, x + w, y + h - r);

    // Draw corners
    drawArc((x + r) / m_iRenderWidth, (y + r) / m_iRenderHeight, fCornerRadius, 180.0f, 270.0f); // Top-left
    drawArc((x + w - r) / m_iRenderWidth, (y + r) / m_iRenderHeight, fCornerRadius, 270.0f, 360.0f); // Top-right
    drawArc((x + r) / m_iRenderWidth, (y + h - r) / m_iRenderHeight, fCornerRadius, 90.0f, 180.0f); // Bottom-left
    drawArc((x + w - r) / m_iRenderWidth, (y + h - r) / m_iRenderHeight, fCornerRadius, 0.0f, 90.0f); // Bottom-right
}

void RenderEngineSDL2::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    drawLine(x1, y1, x2, y2);
    drawLine(x2, y2, x3, y3);
    drawLine(x3, y3, x1, y1);
}

void RenderEngineSDL2::fillTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    // Convert normalized coordinates to pixels
    int px1 = static_cast<int>(x1 * m_iRenderWidth);
    int py1 = static_cast<int>(y1 * m_iRenderHeight);
    int px2 = static_cast<int>(x2 * m_iRenderWidth);
    int py2 = static_cast<int>(y2 * m_iRenderHeight);
    int px3 = static_cast<int>(x3 * m_iRenderWidth);
    int py3 = static_cast<int>(y3 * m_iRenderHeight);

    // Sort the vertices by y-coordinate ascending (py1 <= py2 <= py3)
    if (py1 > py2) { std::swap(py1, py2); std::swap(px1, px2); }
    if (py1 > py3) { std::swap(py1, py3); std::swap(px1, px3); }
    if (py2 > py3) { std::swap(py2, py3); std::swap(px2, px3); }

    int totalHeight = py3 - py1;
    if (totalHeight == 0) return;

    for (int i = 0; i < totalHeight; i++)
    {
        bool secondHalf = i > py2 - py1 || py2 == py1;
        int segmentHeight = secondHalf ? py3 - py2 : py2 - py1;
        float alpha = static_cast<float>(i) / totalHeight;
        float beta = static_cast<float>(i - (secondHalf ? py2 - py1 : 0)) / segmentHeight;

        int ax = px1 + (px3 - px1) * alpha;
        int ay = py1 + i;

        int bx = secondHalf
            ? px2 + (px3 - px2) * beta
            : px1 + (px2 - px1) * beta;
        int by = py1 + i;

        if (ax > bx) std::swap(ax, bx);

        for (int j = ax; j <= bx; j++)
        {
            SDL_RenderDrawPoint(m_pRenderer, j, ay);
        }
    }
}

void RenderEngineSDL2::drawPolyLine(const float* x, const float* y, int count)
{
    for (int i = 0; i < count - 1; ++i)
    {
        drawLine(x[i], y[i], x[i + 1], y[i + 1]);
    }
}

void RenderEngineSDL2::fillPolygon(const float* x, const float* y, int count)
{
    // Convert normalized coordinates to pixels
    std::vector<int> vertexX(count);
    std::vector<int> vertexY(count);
    int minY = m_iRenderHeight, maxY = 0;

    for (int i = 0; i < count; ++i)
    {
        vertexX[i] = static_cast<int>(x[i] * m_iRenderWidth);
        vertexY[i] = static_cast<int>(y[i] * m_iRenderHeight);
        minY = std::min(minY, vertexY[i]);
        maxY = std::max(maxY, vertexY[i]);
    }

    // Clamp minY and maxY to screen bounds
    minY = std::max(minY, 0);
    maxY = std::min(maxY, m_iRenderHeight - 1);

    // Set the drawing color
    SDL_SetRenderDrawColor(m_pRenderer, 255, 255, 255, 255);

    // Scanline algorithm
    for (int y = minY; y <= maxY; y++)
    {
        std::vector<int> nodesX;
        int j = count - 1;

        for (int i = 0; i < count; i++)
        {
            if ((vertexY[i] < y && vertexY[j] >= y) || (vertexY[j] < y && vertexY[i] >= y))
            {
                int nodeX = vertexX[i] + (y - vertexY[i]) * (vertexX[j] - vertexX[i]) / (vertexY[j] - vertexY[i]);
                nodesX.push_back(nodeX);
            }
            j = i;
        }

        std::sort(nodesX.begin(), nodesX.end());

        for (size_t i = 0; i + 1 < nodesX.size(); i += 2)
        {
            int xStart = std::max(nodesX[i], 0);
            int xEnd = std::min(nodesX[i + 1], m_iRenderWidth - 1);
            SDL_RenderDrawLine(m_pRenderer, xStart, y, xEnd, y);
        }
    }
}

void RenderEngineSDL2::fillCircle(float x, float y, float r)
{
    // Convert normalized coordinates to pixels
    int centerX = static_cast<int>(x * m_iRenderWidth);
    int centerY = static_cast<int>(y * m_iRenderHeight);
    int radius = static_cast<int>(r * m_iRenderWidth);

    // Set the drawing color
    SDL_SetRenderDrawColor(m_pRenderer, 255, 255, 255, 255);

    int offsetX = 0;
    int offsetY = radius;
    int d = radius - 1;

    while (offsetY >= offsetX)
    {
        // Draw horizontal lines between the points calculated
        SDL_RenderDrawLine(m_pRenderer, centerX - offsetY, centerY + offsetX, centerX + offsetY, centerY + offsetX);
        SDL_RenderDrawLine(m_pRenderer, centerX - offsetX, centerY + offsetY, centerX + offsetX, centerY + offsetY);
        SDL_RenderDrawLine(m_pRenderer, centerX - offsetX, centerY - offsetY, centerX + offsetX, centerY - offsetY);
        SDL_RenderDrawLine(m_pRenderer, centerX - offsetY, centerY - offsetX, centerX + offsetY, centerY - offsetX);

        if (d >= 2 * offsetX)
        {
            d -= 2 * offsetX + 1;
            offsetX++;
        }
        else if (d < 2 * (radius - offsetY))
        {
            d += 2 * offsetY - 1;
            offsetY--;
        }
        else
        {
            d += 2 * (offsetY - offsetX - 1);
            offsetY--;
            offsetX++;
        }
    }
}

void RenderEngineSDL2::drawCircle(float x, float y, float r)
{
    int segments = 100;
    float theta = 0.0f;
    float step = (2.0f * M_PI) / segments;

    int centerX = static_cast<int>(x * m_iRenderWidth);
    int centerY = static_cast<int>(y * m_iRenderHeight);
    int radius = static_cast<int>(r * m_iRenderWidth);

    int prevX = centerX + radius;
    int prevY = centerY;

    for (int i = 0; i <= segments; ++i)
    {
        theta += step;
        int currX = centerX + static_cast<int>(radius * cos(theta));
        int currY = centerY + static_cast<int>(radius * sin(theta));

        SDL_RenderDrawLine(m_pRenderer, prevX, prevY, currX, currY);

        prevX = currX;
        prevY = currY;
    }
}

void RenderEngineSDL2::drawArc(float x, float y, float r, float a1, float a2)
{
    int segments = 100;
    float startAngle = a1 * (M_PI / 180.0f);
    float endAngle = a2 * (M_PI / 180.0f);
    float theta = startAngle;
    float step = (endAngle - startAngle) / segments;

    int centerX = static_cast<int>(x * m_iRenderWidth);
    int centerY = static_cast<int>(y * m_iRenderHeight);
    int radius = static_cast<int>(r * m_iRenderWidth);

    int prevX = centerX + static_cast<int>(radius * cos(theta));
    int prevY = centerY + static_cast<int>(radius * sin(theta));

    for (int i = 0; i <= segments; ++i)
    {
        theta += step;
        int currX = centerX + static_cast<int>(radius * cos(theta));
        int currY = centerY + static_cast<int>(radius * sin(theta));

        SDL_RenderDrawLine(m_pRenderer, prevX, prevY, currX, currY);

        prevX = currX;
        prevY = currY;
    }
}