#pragma once
#include <vector>
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#define pi 3.14159265358979323846f

struct RGB {
    unsigned char R;
    unsigned char G;
    unsigned char B;
};

struct texture {
    int textureID;
    RGB* data;
    int dataSize;
    int X;
    int Y;
};

struct line {
    ImVec2 p1;
    ImVec2 p2;
    ImColor colour = ImColor(255, 255, 255);
    bool reflective = false;
    bool ignorePrimaryRays = false;
    int textureID = 0;
};

struct hitInfo {
    line* hitLine = 0;
    ImVec2 position = { -1, -1 };
    float distance = -1.f;
    std::vector<float> reflectionDistances = { };
    float distanceFromLineOrigin = -1.f;
    float trueDistanceFromLineOrigin = -1.f;
    ImColor colour = ImColor(0, 0, 0);
    bool applyReflectiveModifier = false;
    int hitDepth = 0;
    bool hitFinished = false;
    int hitLineTextureID = 0;
};

struct vert {
    float angle;
    float distance;
};

struct polygon {
    bool reflective = false;
    ImVec2 position;
    float rotation;
    float scale;
    std::vector<vert> vertices;
    std::vector<line> generateFrame() {
        std::vector<line> lines;
        for (int i = 0; i < vertices.size(); i++) {
            line l;
            l.p1 = { vertices[i].distance * (float)cos(vertices[i].angle + rotation), vertices[i].distance * (float)sin(vertices[i].angle + rotation) };
            l.p2 = { vertices[(i + 1) % vertices.size()].distance * (float)cos(vertices[(i + 1) % vertices.size()].angle + rotation), vertices[(i + 1) % vertices.size()].distance * (float)sin(vertices[(i + 1) % vertices.size()].angle + rotation) };
            l.p1.x *= scale;
            l.p1.y *= scale;
            l.p2.x *= scale;
            l.p2.y *= scale;
            l.p1.x += position.x;
            l.p1.y += position.y;
            l.p2.x += position.x;
            l.p2.y += position.y;
            l.colour = ImColor(100, 100, 255);
            l.reflective = reflective;
            lines.push_back(l);
        }
        return lines;
    }
};
