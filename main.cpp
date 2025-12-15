#include <algorithm>   // for std::max
#include <vector>      // for std::vector used by power pillar wiring
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global animation variables
float boatPos = -120.0f;
float boatSpeed = 1.2f;

// Function prototypes
void drawThreeDDALamps();

// Window dimensions
const float V_WIDTH = 800.0f;
const float V_HEIGHT = 600.0f;

// Animation state variables
float waterTime = 0.0f;
float trainPos = -520.0f;
float trainSpeed = 2.8f;
bool paused = false;

// Timer for traffic signals
float trafficTimer = 0.0f;

// Random float helper function [0,1]
float frandf() {
    return (float)rand() / (float)RAND_MAX;
}

// ==================== BASIC SHAPES ====================

// Draw a filled rectangle
void drawRect(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
}

// Draw a filled ellipse
void drawEllipse(float cx, float cy, float rx, float ry, int segs = 48,
                 float r = 1, float g = 1, float b = 1, float a = 1.0f) {
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for(int i = 0; i <= segs; i++) {
            float t = (float)i / (float)segs * 2.0f * (float)M_PI;
            glVertex2f(cx + cosf(t) * rx, cy + sinf(t) * ry);
        }
    glEnd();
}

// Draw a radial glow effect
void drawRadialGlow(float cx, float cy, float innerR, float outerR, int se,
                    float r, float g, float b) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_TRIANGLE_FAN);
        glColor4f(r, g, b, 0.35f);
        glVertex2f(cx, cy);
        for(int i = 0; i <= se; i++) {
            float th = 2.0f * M_PI * i / (float)se;
            glColor4f(r, g, b, 0.04f);
            glVertex2f(cx + cosf(th) * outerR, cy + sinf(th) * outerR);
        }
    glEnd();
    glDisable(GL_BLEND);
}

// Draw a line using DDA algorithm
void drawLineDDA(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;

    float steps = fabs(dx) > fabs(dy) ? fabs(dx) : fabs(dy);
    float xInc = dx / steps;
    float yInc = dy / steps;

    float x = x1;
    float y = y1;

    glBegin(GL_POINTS);
        for(int i = 0; i <= steps; i++) {
            glVertex2f(x, y);
            x += xInc;
            y += yInc;
        }
    glEnd();
}

// Draw a line using DDA algorithm (for lamp-specific drawing)
void drawLineDDA_Lamp(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;

    float steps = fabs(dx) > fabs(dy) ? fabs(dx) : fabs(dy);
    float xInc = dx / steps;
    float yInc = dy / steps;

    float x = x1;
    float y = y1;

    glBegin(GL_POINTS);
        for(int i = 0; i <= steps; i++) {
            glVertex2f(x, y);
            x += xInc;
            y += yInc;
        }
    glEnd();
}

// ==================== SINGLE LAMP POST (TALLER + DEEPER LIGHT) ====================

// Draw a DDA-based lamp post with taller pole and deeper, warmer light glow
void drawDDALampPost(float x, float groundY) {
    glPointSize(2.0f);

    // Height configuration
    float poleHeight   = 170.0f;          // was 120.0f -> now taller
    float armY         = groundY + poleHeight;
    float headBottomY  = armY - 12.0f;    // lamp head length ~12px
    float glowCenterY  = armY - 22.0f;    // glow center a bit below the head

    // Pole
    glColor3f(0.35f, 0.35f, 0.38f);
    drawLineDDA_Lamp(x, groundY, x, armY);

    // Horizontal arm
    drawLineDDA_Lamp(x, armY,
                     x + 22.0f, armY);

    // Lamp head
    glColor3f(1.0f, 0.95f, 0.65f);
    drawLineDDA_Lamp(x + 22.0f, armY,
                     x + 22.0f, headBottomY);

    // Deeper, layered glow (repositioned according to new height)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float lx = x + 22.0f;  // lamp center X
    float ly = glowCenterY; // lamp center Y (higher than before)

    // Core bright area (small, very intense)
    drawEllipse(lx, ly,
                7.5f, 6.0f, 32,
                1.0f, 0.99f, 0.88f, 1.0f);

    // Mid halo (main visible glow)
    drawEllipse(lx, ly,
                16.0f, 12.0f, 32,
                1.0f, 0.93f, 0.72f, 0.55f);

    // Outer soft halo
    drawEllipse(lx, ly,
                30.0f, 20.0f, 32,
                1.0f, 0.86f, 0.55f, 0.26f);

    // Wide subtle radial glow
    drawRadialGlow(lx, ly, 10.0f, 60.0f, 32,
                   1.0f, 0.85f, 0.50f);

    glDisable(GL_BLEND);
}

// ==================== SKY, HALFTONE, CLOUDS ====================

// FIXED: Draw vertical gradient with proper two-quad method
void drawVerticalGradient(float x, float y, float w, float h,
                          float rTop, float gTop, float bTop,
                          float rMid, float gMid, float bMid,
                          float rBot, float gBot, float bBot) {
    // Split into two quads to avoid triangular artifacts
    float midY = y + h * 0.50f;

    // Top quad (top color -> mid color)
    glBegin(GL_QUADS);
        glColor3f(rTop, gTop, bTop); glVertex2f(x,      y + h);  // top-left
        glColor3f(rTop, gTop, bTop); glVertex2f(x + w,  y + h);  // top-right
        glColor3f(rMid, gMid, bMid); glVertex2f(x + w,  midY);   // mid-right
        glColor3f(rMid, gMid, bMid); glVertex2f(x,      midY);   // mid-left
    glEnd();

    // Bottom quad (mid color -> bottom color)
    glBegin(GL_QUADS);
        glColor3f(rMid, gMid, bMid); glVertex2f(x,      midY);   // mid-left
        glColor3f(rMid, gMid, bMid); glVertex2f(x + w,  midY);   // mid-right
        glColor3f(rBot, gBot, bBot); glVertex2f(x + w,  y);      // bot-right
        glColor3f(rBot, gBot, bBot); glVertex2f(x,      y);      // bot-left
    glEnd();
}

// Draw the sky gradient background
void drawSky() {
    glShadeModel(GL_SMOOTH);
    // Teal top -> purple mid -> warm horizon
    drawVerticalGradient(0, 0, V_WIDTH, V_HEIGHT,
                         0.02f, 0.12f, 0.18f,    // top: deep teal-blue
                         0.28f, 0.12f, 0.36f,    // mid: violet
                         1.0f, 0.62f, 0.34f);    // bottom: warm orange
    // Subtle darker vignette near top corners (push eye to center)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawRect(0, V_HEIGHT * 0.82f, V_WIDTH, V_HEIGHT * 0.18f,
             0.0f, 0.0f, 0.06f, 0.12f);
    glDisable(GL_BLEND);
}

// Draw a halftone band effect
void drawHalftoneBand() {
    float bandY = V_HEIGHT * 0.38f;
    int rows = 6;
    int cols = 120;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int r = 0; r < rows; r++) {
        for(int c = 0; c < cols; c++) {
            if ((c + r) % 2 != 0) continue;
            float x = (float)c / (float)cols * V_WIDTH + frandf() * 2.0f;
            float y = bandY + (r - rows/2) * 6.0f + frandf() * 3.0f;
            drawRect(x, y, 2.8f, 2.8f, 0.95f, 0.9f, 0.7f, 0.35f);
        }
    }
    glDisable(GL_BLEND);
}

// Draw a cloud with multiple layers
void drawCloud(float cx, float cy, float scale = 1.0f, float alpha = 0.6f) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float tintR = 0.92f, tintG = 0.88f, tintB = 0.95f;
    drawEllipse(cx, cy, 120.0f * scale, 34.0f * scale, 48,
                tintR, tintG, tintB, 0.18f * alpha);
    drawEllipse(cx - 80.0f * scale, cy + 8.0f * scale,
                92.0f * scale, 28.0f * scale, 40,
                tintR, tintG, tintB, 0.16f * alpha);
    drawEllipse(cx + 78.0f * scale, cy + 6.0f * scale,
                96.0f * scale, 26.0f * scale, 40,
                tintR, tintG, tintB, 0.16f * alpha);
    drawEllipse(cx - 36.0f * scale, cy - 18.0f * scale,
                78.0f * scale, 22.0f * scale, 36,
                tintR, tintG, tintB, 0.12f * alpha);
    drawEllipse(cx + 36.0f * scale, cy - 20.0f * scale,
                82.0f * scale, 20.0f * scale, 36,
                tintR, tintG, tintB, 0.12f * alpha);
    drawEllipse(cx - 20.0f * scale, cy + 6.0f * scale,
                160.0f * scale, 40.0f * scale, 56,
                1.0f, 0.96f, 0.85f, 0.06f * alpha);
    drawRect(cx - 160.0f * scale, cy - 28.0f * scale,
             320.0f * scale, 6.0f * scale,
             0.02f, 0.02f, 0.04f, 0.03f * alpha);
    glDisable(GL_BLEND);
}

// Draw a layer of procedural clouds
void drawCloudLayer(float baseY, int seed, int count, float alpha,
                    float scaleMin = 0.7f, float scaleMax = 1.2f) {
    srand(seed);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int i = 0; i < count; i++) {
        float cx = frandf() * V_WIDTH;
        float rx = 40.0f + frandf() * 160.0f;
        float ry = 10.0f + frandf() * 40.0f;
        float yoff = (frandf() - 0.5f) * 30.0f;
        float a = alpha * (0.35f + frandf() * 0.45f);
        float tint = 0.9f - frandf() * 0.25f;
        drawEllipse(cx, baseY + yoff + i * 1.5f,
                    rx * (scaleMin + frandf() * (scaleMax - scaleMin)),
                    ry, 36,
                    tint * 0.92f, tint * 0.83f, tint * 1.02f, a);
    }
    glDisable(GL_BLEND);
}

// ==================== BUILDINGS & BRIDGE ====================

// Draw a blocky building with windows
void drawBuildingBlocky(float x, float y, float w, float h,
                        float darkness, int seed) {
    srand(seed);
    drawRect(x, y, w, h,
             darkness * 0.15f, darkness * 0.18f, darkness * 0.22f, 1.0f);
    float marginX = 6.0f, marginY = 10.0f;
    float cw = 12.0f, ch = 10.0f;
    float spx = 6.0f, spy = 8.0f;
    int cols = (int)((w - 2 * marginX) / (cw + spx));
    int rows = (int)((h - 2 * marginY) / (ch + spy));
    for(int r = 0; r < rows; r++) {
        for(int c = 0; c < cols; c++) {
            if ((rand() % 4) == 0) continue;
            float wx = x + marginX + c * (cw + spx);
            float wy = y + marginY + r * (ch + spy);
            float warm = 0.95f - (float)r / (float)rows * 0.45f;
            float bright = 0.4f + frandf() * 0.85f;
            drawRect(wx, wy, cw, ch,
                     warm * 1.0f, warm * 0.8f, 0.45f, 0.85f * bright);
        }
    }
}

// Draw a layer of skyline buildings
void drawSkylineLayer(float baseY, float minW, float maxW,
                      float minH, float maxH, int seed, float darkness) {
    srand(seed);
    float x = -20.0f;
    int i = 0;
    while (x < V_WIDTH + 40.0f) {
        float w = minW + frandf() * (maxW - minW);
        float h = minH + frandf() * (maxH - minH);
        float d = darkness - frandf() * 0.12f;
        drawBuildingBlocky(x, baseY, w, h, d, seed + i * 31);
        x += w + 6.0f + frandf() * 12.0f;
        ++i;
    }
}

// Draw the bridge and water
void drawBridgeAndWater() {
    float bridgeY = 120.0f;
    drawRect(0, 0, V_WIDTH, bridgeY,
             0.02f, 0.03f, 0.06f, 1.0f);
    drawRect(0, bridgeY, V_WIDTH, 72.0f,
             0.06f, 0.06f, 0.09f, 1.0f);
    drawRect(0, bridgeY + 72.0f, V_WIDTH, 6.0f,
             0.03f, 0.03f, 0.05f, 1.0f);

    glLineWidth(2.0f);
    glColor3f(0.14f, 0.14f, 0.16f);
    glBegin(GL_LINES);
        glVertex2f(18.0f, bridgeY + 16.0f);
        glVertex2f(V_WIDTH - 18.0f, bridgeY + 16.0f);
        glVertex2f(18.0f, bridgeY + 30.0f);
        glVertex2f(V_WIDTH - 18.0f, bridgeY + 30.0f);
    glEnd();

    for(float px = 36.0f; px < V_WIDTH; px += 40.0f) {
        drawRect(px - 2.0f, bridgeY, 4.0f, 72.0f,
                 0.07f, 0.07f, 0.09f);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int i = 0; i < 18; i++) {
        float rx = frandf() * V_WIDTH;
        float rw = 30.0f + frandf() * 100.0f;
        float ry = frandf() * (bridgeY * 0.8f);
        float a = 0.02f + frandf() * 0.06f;
        drawRect(rx, ry, rw, 1.0f + frandf() * 3.0f,
                 0.95f, 0.7f, 0.4f, a);
    }
    glDisable(GL_BLEND);
}

// ==================== POWER PILLARS + WIRES ====================

// Draw a power pillar structure
void drawPowerPillar(float x, float baseY, float height, int seed) {
    srand(seed);
    float halfW = 8.0f;
    float y0 = baseY + 72.0f;
    float yTop = y0 + height;
    drawRect(x - halfW, y0, 4.0f, height,
             0.22f, 0.22f, 0.26f);
    drawRect(x + halfW - 4.0f, y0, 4.0f, height,
             0.22f, 0.22f, 0.26f);
    float armY1 = yTop - height * 0.25f;
    float armY2 = yTop - height * 0.55f;
    drawRect(x - 30.0f, armY1, 60.0f, 4.0f,
             0.16f, 0.16f, 0.18f);
    drawRect(x - 22.0f, armY2, 44.0f, 4.0f,
             0.16f, 0.16f, 0.18f);

    drawRect(x - 34.0f, armY1 + 4.0f, 6.0f, 6.0f,
             0.65f, 0.65f, 0.7f);
    drawRect(x + 28.0f, armY1 + 4.0f, 6.0f, 6.0f,
             0.65f, 0.65f, 0.7f);
    drawRect(x - 26.0f, armY2 + 4.0f, 6.0f, 6.0f,
             0.65f, 0.65f, 0.7f);
    drawRect(x + 20.0f, armY2 + 4.0f, 6.0f, 6.0f,
             0.65f, 0.65f, 0.7f);
}

// Draw power wires between towers
void drawPowerWires(const std::vector<float>& towerXs, float baseY, float height) {
    glLineWidth(2.0f);
    glColor3f(0.06f, 0.06f, 0.08f);

    for (int strand = 0; strand < 3; ++strand) {
        glBegin(GL_LINE_STRIP);
            for (size_t i = 0; i < towerXs.size(); ++i) {
                float x = towerXs[i];
                float topY = baseY + 72.0f + height - (strand * 12.0f);
                float sag = 12.0f * sinf((float)i * 0.6f + strand * 0.9f) * 0.08f;
                glVertex2f(x, topY - fabs(sag));
                if (i + 1 < towerXs.size()) {
                    float nx = (towerXs[i] + towerXs[i+1]) * 0.5f;
                    float midY = topY + 10.0f + (sag * 0.6f);
                    glVertex2f(nx, midY);
                }
            }
        glEnd();
    }
}

// Draw all power pillars and connecting wires
void drawPowerPillarsAndWires() {
    float bridgeY = 120.0f;
    float towerHeight = 220.0f;
    std::vector<float> xs;
    for(float x = 60.0f; x < V_WIDTH - 60.0f; x += 140.0f) {
        drawPowerPillar(x, bridgeY, towerHeight, (int)x);
        xs.push_back(x);
    }
    drawPowerWires(xs, bridgeY, towerHeight);
}

// ==================== TRAFFIC SIGNAL ====================

// Draw an animated traffic signal
void drawTrafficSignal(float x, float bridgeY, float phaseOffsetSec) {
    float boxW = 18.0f;
    float boxH = 54.0f;
    float boxX = x - boxW * 0.5f;
    float boxY = bridgeY + 72.0f + 60.0f;
    drawRect(x - 4.0f, bridgeY + 72.0f, 8.0f, 56.0f,
             0.12f, 0.12f, 0.14f);
    drawRect(boxX - 2.0f, boxY - 6.0f, boxW + 4.0f, boxH + 6.0f,
             0.06f, 0.06f, 0.07f, 1.0f);
    drawRect(boxX, boxY, boxW, boxH,
             0.08f, 0.08f, 0.09f, 1.0f);

    float t = fmodf(trafficTimer + phaseOffsetSec, 6.0f);
    bool redOn = (t < 2.5f);
    bool greenOn = (t >= 2.5f && t < 5.0f);
    bool yellowOn = (t >= 5.0f);
    float cx = x;
    float cy_red = boxY + boxH - 10.0f;
    float cy_yel = boxY + boxH * 0.5f;
    float cy_grn = boxY + 10.0f;
    float dim = 0.15f;
    drawEllipse(cx, cy_red, 6.8f, 6.8f, 24,
                dim, 0.0f, 0.0f, 1.0f);
    drawEllipse(cx, cy_yel, 6.8f, 6.8f, 24,
                dim, dim, 0.0f, 1.0f);
    drawEllipse(cx, cy_grn, 6.8f, 6.8f, 24,
                0.0f, dim, 0.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (redOn) {
        drawEllipse(cx, cy_red, 6.8f, 6.8f, 24,
                    1.0f, 0.18f, 0.18f, 1.0f);
        drawRadialGlow(cx, cy_red, 10.0f, 36.0f, 24,
                       1.0f, 0.18f, 0.18f);
    }
    if (greenOn) {
        drawEllipse(cx, cy_grn, 6.8f, 6.8f, 24,
                    0.4f, 1.0f, 0.45f, 1.0f);
        drawRadialGlow(cx, cy_grn, 10.0f, 36.0f, 24,
                       0.4f, 1.0f, 0.45f);
    }
    if (yellowOn) {
        drawEllipse(cx, cy_yel, 6.8f, 6.8f, 24,
                    1.0f, 0.86f, 0.2f, 1.0f);
        drawRadialGlow(cx, cy_yel, 10.0f, 36.0f, 24,
                       1.0f, 0.86f, 0.2f);
    }
    glDisable(GL_BLEND);
}

// Draw all traffic signals
void drawTrafficSignals() {
    float bridgeY = 120.0f;

    // First traffic signal (leftmost)
    drawTrafficSignal(40.0f, bridgeY, 0.0f);

    // Last traffic signal (rightmost)
    drawTrafficSignal(V_WIDTH - 40.0f, bridgeY, 3.0f);
}

// ==================== TRAIN (ANIMATED) ====================

// Draw an animated train
void drawTrain() {
    float trackY = 170.0f;
    glPushMatrix();
        glTranslatef(trainPos, trackY - 8.0f, 0);

        float bodyR = 0.95f, bodyG = 0.72f, bodyB = 0.18f;
        float roofR = 0.14f, roofG = 0.14f, roofB = 0.18f;
        float carW = 140.0f, carH = 64.0f;

        // Draw train cars
        for(int car = 0; car < 4; ++car) {
            drawRect(-car * (carW + 8.0f), 0, carW, carH,
                     bodyR, bodyG, bodyB, 1.0f);
            drawRect(-car * (carW + 8.0f), carH - 12.0f, carW, 12.0f,
                     roofR, roofG, roofB, 1.0f);
            drawRect(-car * (carW + 8.0f), 10.0f, carW, 6.0f,
                     0.92f, 0.58f, 0.16f, 1.0f);

            // Windows
            for(float wx = 12.0f; wx < carW - 12.0f; wx += 34.0f) {
                float wy = 26.0f + frandf() * 2.0f;
                drawRect(-car * (carW + 8.0f) + wx, wy,
                         24.0f, 20.0f,
                         1.0f, 0.95f, 0.45f,
                         0.96f + frandf() * 0.04f);
            }
        }

        // Front light
        drawRect(16.0f, 18.0f, 10.0f, 18.0f,
                 1.0f, 0.98f, 0.78f);
        drawRadialGlow(36.0f, trackY - 8.0f + 26.0f,
                       18.0f, 60.0f, 20,
                       1.0f, 0.95f, 0.6f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Wheels
        for(int w = 0; w < 8; ++w) {
            float wx = -w * 58.0f + 24.0f;
            float wy = -8.0f;
            drawEllipse(wx, wy, 12.0f, 12.0f, 32,
                        0.08f, 0.08f, 0.10f, 1.0f);
            drawEllipse(wx, wy, 5.5f, 5.5f, 24,
                        0.2f, 0.2f, 0.22f, 1.0f);
        }

        // Extra front wheel (first compartment)
        {
            float wx = 92.0f;   // Position near front nose
            float wy = -8.0f;

            drawEllipse(wx, wy, 12.0f, 12.0f, 32,
                        0.08f, 0.08f, 0.10f, 1.0f);  // Rim
            drawEllipse(wx, wy, 5.5f, 5.5f, 24,
                        0.2f, 0.2f, 0.22f, 1.0f);    // Hub
        }

        glDisable(GL_BLEND);
    glPopMatrix();
}

// ==================== DISTANT LIGHTS, SUN ====================

// Draw distant city lights
void drawDistantLights() {
    glPointSize(2.0f);
    glBegin(GL_POINTS);
        for(int i = 0; i < 180; i++) {
            float x = frandf() * V_WIDTH;
            float y = 120.0f + frandf() * 360.0f;
            float b = 0.5f + frandf() * 0.6f;
            glColor3f(0.95f * b, 0.72f * b, 0.45f * b);
            glVertex2f(x, y);
        }
    glEnd();
}

// Draw sun with lens flares
void drawSunAndFlares() {
    float cx = V_WIDTH * 0.33f;
    float cy = V_HEIGHT * 0.36f;
    drawEllipse(cx, cy, 26.0f, 26.0f, 60,
                1.0f, 0.95f, 0.64f, 1.0f);
    drawRadialGlow(cx, cy, 36.0f, 100.0f, 40,
                   1.0f, 0.72f, 0.3f);
    glPushMatrix();
        glTranslatef(cx, cy, 0);
        drawEllipse(0, 0, 220.0f, 18.0f, 32,
                    1.0f, 0.62f, 0.22f, 0.045f);
    glPopMatrix();
}

// ==================== WIRES/POLES (OLD CATENARY) ====================

// Draw poles and overhead wires for train
void drawPolesAndWires() {
    float trackY = 170.0f;
    for(float x = 40.0f; x < V_WIDTH; x += 160.0f) {
        drawRect(x - 5.0f, trackY, 10.0f, 220.0f,
                 0.12f, 0.12f, 0.14f);
        drawRect(x - 24.0f, trackY + 178.0f, 48.0f, 6.0f,
                 0.12f, 0.12f, 0.14f);
    }
    glColor3f(0.22f, 0.22f, 0.26f);
    glBegin(GL_LINES);
        glVertex2f(0, trackY + 184.0f); glVertex2f(V_WIDTH, trackY + 184.0f);
        glVertex2f(0, trackY + 196.0f); glVertex2f(V_WIDTH, trackY + 196.0f);
    glEnd();
}

// Draw moon with glow effect
void drawMoon(float cx, float cy, float radius) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Glow
    glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.9f, 0.9f, 1.0f, 0.25f);
        glVertex2f(cx, cy);
        for(int i = 0; i <= 60; i++) {
            float t = 2.0f * M_PI * i / 60.0f;
            glColor4f(0.9f, 0.9f, 1.0f, 0.0f);
            glVertex2f(cx + cos(t) * radius * 3.0f,
                       cy + sin(t) * radius * 3.0f);
        }
    glEnd();

    // Moon body
    glColor3f(0.97f, 0.97f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for(int i = 0; i <= 60; i++) {
            float t = 2.0f * M_PI * i / 60.0f;
            glVertex2f(cx + cos(t) * radius,
                       cy + sin(t) * radius);
        }
    glEnd();

    glDisable(GL_BLEND);
}

// Draw Japanese-style elevated viaduct
void drawJapaneseViaduct(float trackY) {
    float deckThickness = 22.0f;
    float deckY = trackY - deckThickness;
    float pillarTop = deckY;
    float groundY = 0.0f;

    // ===================== VIADUCT DECK =====================
    drawRect(0, deckY, V_WIDTH, deckThickness,
             0.78f, 0.78f, 0.82f, 1.0f);   // Light concrete

    // Deck edge shadow
    drawRect(0, deckY, V_WIDTH, 3.0f,
             0.55f, 0.55f, 0.58f, 1.0f);

    // ===================== SIDE BARRIERS =====================
    for(float x = 0; x < V_WIDTH; x += 32.0f) {
        drawRect(x, deckY + deckThickness - 6.0f,
                 20.0f, 4.0f,
                 0.62f, 0.62f, 0.65f, 1.0f);
    }

    // ===================== SUPPORT PILLARS =====================
    for(float x = 80.0f; x < V_WIDTH; x += 160.0f) {
        // Main pillar
        drawRect(x - 18.0f, groundY,
                 36.0f, pillarTop - groundY,
                 0.70f, 0.70f, 0.74f, 1.0f);

        // Pillar base
        drawRect(x - 28.0f, groundY,
                 56.0f, 14.0f,
                 0.55f, 0.55f, 0.58f, 1.0f);

        // Top cap
        drawRect(x - 26.0f, pillarTop - 6.0f,
                 52.0f, 6.0f,
                 0.60f, 0.60f, 0.63f, 1.0f);
    }

    // ===================== SHADOW UNDER DECK =====================
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawRect(0, deckY - 6.0f, V_WIDTH, 6.0f,
             0.0f, 0.0f, 0.0f, 0.18f);
    glDisable(GL_BLEND);
}

// Draw animated water reflections
void drawAnimatedWater(float waterTopY) {
    // Base water color
    drawRect(0, 0, V_WIDTH, waterTopY,
             0.06f, 0.18f, 0.32f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Moving horizontal reflection streaks
    for(int i = 0; i < 40; i++) {
        float y = fmodf(i * 14.0f + waterTime * 22.0f, waterTopY);
        float x = fmodf(i * 63.0f + waterTime * 40.0f, V_WIDTH);
        float w = 60.0f + 40.0f * sinf(waterTime + i);
        float a = 0.04f + 0.03f * sinf(waterTime * 1.4f + i);

        drawRect(x, y, w, 2.0f,
                 0.95f, 0.75f, 0.45f, a);
    }

    // Soft vertical shimmer near bridge
    for(int i = 0; i < 12; i++) {
        float x = i * (V_WIDTH / 12.0f) + sinf(waterTime + i) * 8.0f;
        drawRect(x, waterTopY - 12.0f,
                 6.0f, 12.0f,
                 0.9f, 0.7f, 0.4f, 0.08f);
    }

    glDisable(GL_BLEND);
}

// Draw three DDA lamp posts
void drawThreeDDALamps() {
    float groundY = 120.0f;  // Bridge / road height

    drawDDALampPost(180.0f, groundY);
    drawDDALampPost(420.0f, groundY);
    drawDDALampPost(660.0f, groundY);
}

// Draw speed boat
void drawSpeedBoat() {
    float waterY = 65.0f;

    glPushMatrix();
    glTranslatef(boatPos, waterY, 0);

    // Slight scale up
    glScalef(1.4f, 1.4f, 1.0f);

    // ================= MAIN HULL (ANGLED) =================
    glBegin(GL_POLYGON);
        glColor3f(0.12f, 0.12f, 0.15f);   // Dark steel gray
        glVertex2f(0, 2);     // Rear bottom
        glVertex2f(10, 0);
        glVertex2f(95, 0);    // Bottom mid
        glVertex2f(120, 9);   // Sharp bow
        glVertex2f(95, 18);   // Upper hull
        glVertex2f(12, 18);
        glVertex2f(0, 14);    // Rear top
    glEnd();

    // ================= HULL TOP EDGE =================
    glBegin(GL_LINES);
        glColor3f(0.25f, 0.25f, 0.28f);
        glVertex2f(12, 18);
        glVertex2f(95, 18);
    glEnd();

    // ================= BLACK STRIPE =================
    drawRect(14, 7, 70, 3,
             0.0f, 0.0f, 0.0f);

    // ================= CABIN (SLOPED) =================
    glBegin(GL_POLYGON);
        glColor3f(0.88f, 0.88f, 0.90f);
        glVertex2f(30, 18);
        glVertex2f(70, 18);
        glVertex2f(60, 34);
        glVertex2f(34, 34);
    glEnd();

    // ================= FRONT WINDOW =================
    glBegin(GL_POLYGON);
        glColor3f(0.30f, 0.55f, 0.75f);
        glVertex2f(38, 22);
        glVertex2f(56, 22);
        glVertex2f(50, 30);
        glVertex2f(40, 30);
    glEnd();

    // ================= SIDE WINDOW =================
    drawRect(58, 22, 10, 6,
             0.30f, 0.55f, 0.75f);

    glEnd();
    glDisable(GL_BLEND);

    glPopMatrix();
}

// Draw a bat
void drawBat(float cx, float cy, float scale) {
    glPushMatrix();
    glTranslatef(cx, cy, 0);
    glScalef(scale, scale, 1.0f);

    glColor3f(0.05f, 0.05f, 0.07f); // Dark bat color

    // Left wing
    glBegin(GL_TRIANGLES);
        glVertex2f(0, 0);
        glVertex2f(-18, 8);
        glVertex2f(-30, 0);
    glEnd();

    // Right wing
    glBegin(GL_TRIANGLES);
        glVertex2f(0, 0);
        glVertex2f(18, 8);
        glVertex2f(30, 0);
    glEnd();

    // Body
    glBegin(GL_TRIANGLES);
        glVertex2f(-4, 0);
        glVertex2f(4, 0);
        glVertex2f(0, -10);
    glEnd();

    glPopMatrix();
}

// Draw multiple bats in sky
void drawBatsInSky() {
    drawBat(120, 520, 0.7f);
    drawBat(160, 540, 0.5f);
    drawBat(210, 515, 0.6f);

    drawBat(520, 560, 0.8f);
    drawBat(560, 540, 0.6f);

    drawBat(680, 510, 0.7f);
}

// ==================== DISPLAY / UPDATE ====================

// Main display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // 1) Sky + sun + clouds + bands
    drawSky();
    drawBatsInSky();
    drawCloud(220.0f, V_HEIGHT * 0.62f, 1.05f, 0.75f);
    drawCloud(420.0f, V_HEIGHT * 0.66f, 0.82f, 0.55f);
    drawCloud(620.0f, V_HEIGHT * 0.58f, 0.9f, 0.60f);
    drawHalftoneBand();
    drawSunAndFlares();
    drawCloudLayer(V_HEIGHT * 0.62f, 11, 8, 0.42f, 0.8f, 1.1f);
    drawCloudLayer(V_HEIGHT * 0.50f, 23, 10, 0.30f, 0.6f, 1.2f);

    // 2) Distant & mid skylines (STABLE)
    drawSkylineLayer(240.0f, 26.0f, 70.0f, 160.0f, 220.0f, 101, 0.42f);
    drawSkylineLayer(160.0f, 36.0f, 88.0f, 140.0f, 220.0f, 142, 0.28f);

    // 3) Bridge base
    drawBridgeAndWater();

    // 4) Animated water (draw FIRST)
    float bridgeY = 120.0f;
    drawAnimatedWater(bridgeY);

    // 5) Speedboat (draw AFTER water so it's visible)
    drawSpeedBoat();

    // 6) Poles & power infrastructure
    drawPolesAndWires();
    drawPowerPillarsAndWires();

    // 7) Traffic signals
    drawTrafficSignals();

    // 8) Lamp posts (DDA)
    drawThreeDDALamps();

    // 9) Japanese elevated viaduct
    float trackY = 170.0f;
    drawJapaneseViaduct(trackY);

    // 10) Train (ONLY moving object on land)
    drawTrain();

    // 11) Moon
    drawMoon(V_WIDTH * 0.78f, V_HEIGHT * 0.78f, 22.0f);

    // 12) Final city lights
    drawDistantLights();

    glutSwapBuffers();
}

// Update animation states
void update(int) {
    if(!paused) {
        // Advance train position to animate it across the scene
        trainPos += trainSpeed;
        if (trainPos > V_WIDTH + 360.0f) trainPos = -760.0f;

        boatPos += boatSpeed;
        if (boatPos > V_WIDTH + 120.0f)
            boatPos = -150.0f;

        // Advance traffic timer
        trafficTimer += 0.016f; // ~60fps increment
        if (trafficTimer >= 100000.0f)
            trafficTimer = fmodf(trafficTimer, 100000.0f);
    }
    waterTime += 0.016f;   // ~60 FPS water movement

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// Keyboard controls
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 27:  // ESC to exit
            exit(0);
            break;
        case ' ': // Space to pause
            paused = !paused;
            break;
        case '+': // Increase train speed
            trainSpeed += 0.2f;
            break;
        case '-': // Decrease train speed (clamped)
            trainSpeed = std::max(0.2f, trainSpeed - 0.2f);
            break;
    }
}

// Initialize OpenGL settings
void init() {
    srand((unsigned int)time(NULL));
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, V_WIDTH, 0.0, V_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.02f, 1.0f);
}

// Main program entry point
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowSize((int)V_WIDTH, (int)V_HEIGHT);
    glutCreateWindow("Sunset Cityscape");
    init();
    glutDisplayFunc(display);
    glutTimerFunc(0, update, 0);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
