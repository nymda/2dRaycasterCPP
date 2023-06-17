#pragma once
#include "structures.h"

extern std::vector<line> lines;
extern std::vector<polygon> dynamics;
extern ImVec2 player;
extern float playerAngle;
extern bool playerInit;
extern bool linesInit;
extern ImDrawList* draw;

//psuedo-3d window size
extern float _width;
extern float _height;

//rendering paramters
extern float _lumens;
extern float _candella;
extern float _focalLength;