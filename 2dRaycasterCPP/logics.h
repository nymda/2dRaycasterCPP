#pragma once
#include "structures.h"

ImVec2 normalise(ImVec2 vector);
ImVec2 angleToVector(float angle);
ImVec2 vectorToPoint(ImVec2 source, ImVec2 target);
float angleToPoint(ImVec2 source, ImVec2 target);
float crossProduct(const ImVec2& a, const ImVec2& b);
ImVec2 lineCenter(line l);
float distance(ImVec2 p1, ImVec2 p2);
float calculateNormalAngle(line l);
float calculateReflectionAngle(line l, float incomingAngle);
bool intersect(line* a, line* b, ImVec2* out);
bool castRay(ImVec2 origin, float angle, float maxDistance, hitInfo* hitInf, int depth = 0, float reflectionAddedDistance = 0);