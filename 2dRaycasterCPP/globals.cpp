#include "globals.h"

//yes, yes, globals are bad, shut up

std::vector<line> lines;
std::vector<polygon> dynamics;
ImVec2 player = { -1, -1 };
float playerAngle = pi * 1.5;
bool playerInit = false;
bool linesInit = false;
ImDrawList* draw = 0;

//psuedo-3d window size
float _width = 640 / 1.5;
float _height = 480 / 1.5;

//rendering paramters
float _lumens = 5.f;
float _candella = 10000.f;
float _focalLength = 0.5f;