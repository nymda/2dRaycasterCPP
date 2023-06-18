#pragma once
#include "structures.h"

void generateDynamicPolygon(ImVec2 position, float rotation, float scale, int sideCount, bool reflective, int textureID);
void linesAddCircle(ImVec2 origin, float radius, int sides, ImColor colour, bool reflective = false);
void linesAddRectangle(ImVec2 p1, ImVec2 p2, ImColor colour, bool reflective = false);
void linesAddRectangle(ImVec2 origin, float width, float height, ImColor colour, bool reflective = false);