#include "logics.h"
#include "globals.h"
#include <math.h>

ImVec2 normalise(ImVec2 vector) {
    float l = sqrt(vector.x * vector.x + vector.y * vector.y);
    return { vector.x / l, vector.y / l };
}

ImVec2 angleToVector(float angle) {
    return { (float)cos(angle), (float)sin(angle) };
}

ImVec2 vectorToPoint(ImVec2 source, ImVec2 target) {
    ImVec2 v = { target.x - source.x, target.y - source.y };
    return normalise(v);
}

float angleToPoint(ImVec2 source, ImVec2 target) {
    ImVec2 v = vectorToPoint(source, target);
    return atan2(v.y, v.x);
}

float crossProduct(const ImVec2& a, const ImVec2& b) {
    return a.x * b.y - a.y * b.x;
}

ImVec2 lineCenter(line l) {
    return { l.p1.x + (l.p2.x - l.p1.x) / 2.f, l.p1.y + (l.p2.y - l.p1.y) / 2.f };
}

float distance(ImVec2 p1, ImVec2 p2) {
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

float calculateNormalAngle(line l) {
    ImVec2 normal = ImVec2{ l.p2.x - l.p1.x, l.p2.y - l.p1.y };
    float angle = atan2(normal.y, normal.x);
    angle += pi / 2.f;
    if (angle < 0) angle += pi * 2.f;
    return angle;
}

float calculateReflectionAngle(line l, float incomingAngle) {
    float normalAngle = calculateNormalAngle(l);
    float angle = normalAngle - (incomingAngle - normalAngle);
    return angle;
}

bool intersect(line* a, line* b, ImVec2* out) {
    ImVec2 p = a->p1;
    ImVec2 q = b->p1;
    ImVec2 r = ImVec2{ a->p2.x - a->p1.x, a->p2.y - a->p1.y };
    ImVec2 s = ImVec2{ b->p2.x - b->p1.x, b->p2.y - b->p1.y };

    float rCrossS = crossProduct(r, s);
    ImVec2 qMinusP = ImVec2{ q.x - p.x, q.y - p.y };

    // Check if the lines are parallel or coincident
    if (std::abs(crossProduct(qMinusP, r)) <= std::numeric_limits<float>::epsilon() && std::abs(crossProduct(qMinusP, s)) <= std::numeric_limits<float>::epsilon()) {
        // Check if the lines are collinear and overlapping
        if (crossProduct(qMinusP, r) <= std::numeric_limits<float>::epsilon()) {
            float t0 = (qMinusP.x * r.x + qMinusP.y * r.y) / (r.x * r.x + r.y * r.y);
            float t1 = t0 + (s.x * r.x + s.y * r.y) / (r.x * r.x + r.y * r.y);

            float tMin = std::min(t0, t1);
            float tMax = std::max(t0, t1);

            if (tMax >= 0.f && tMin <= 1.f) {
                out->x = p.x + tMin * r.x;
                out->y = p.y + tMin * r.y;
                return true;
            }
        }

        return false;
    }

    float t = crossProduct(qMinusP, s) / rCrossS;
    float u = crossProduct(qMinusP, r) / rCrossS;

    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        out->x = p.x + t * r.x;
        out->y = p.y + t * r.y;
        return true;
    }

    return false;
}

bool castRay(ImVec2 origin, float angle, float maxDistance, hitInfo* hitInf, int depth, float reflectionAddedDistance) {
    ImVec2 target = ImVec2(origin.x + (cos(angle) * maxDistance), origin.y + (sin(angle) * maxDistance));

    line ray = { origin, target };

    ImVec2 closestHit = target;
    float distanceToClosestHit = maxDistance;
    ImColor closestHitColour = ImColor(0, 0, 0);
    bool isReflective = false;
    line* hitLine = 0;

    std::vector<line> frameMergedLines = {};
    for (line& l : lines) {
        frameMergedLines.push_back(l);
    }

    for (polygon& p : dynamics) {
        for (line& l : p.generateFrame()) {
            frameMergedLines.push_back(l);
        }
    }

    for (line& line : frameMergedLines) {
        ImVec2 hit = { -1, -1 };
        if (intersect(&ray, &line, &hit)) {

            if (depth == 0 && line.ignorePrimaryRays) { continue; }

            float distanceToHit = sqrt(pow(origin.x - hit.x, 2) + pow(origin.y - hit.y, 2));

            if (depth == 0) {
                float mA = playerAngle - angle;
                distanceToHit *= cos(mA);
            }

            if (distanceToHit < distanceToClosestHit && distanceToHit > 0.01f) {
                distanceToClosestHit = distanceToHit;
                closestHit = { hit.x, hit.y };
                closestHitColour = line.colour;
                isReflective = line.reflective;
                hitLine = &line;
            }
        }
    }

    hitInf->position = closestHit;
    hitInf->distance = distanceToClosestHit;
    hitInf->colour = closestHitColour;
    hitInf->applyReflectiveModifier = false;
    hitInf->hitDepth = depth;

    draw->AddLine(origin, closestHit, ImColor(255, 255, 255));

    hitInf->reflectionDistances.push_back(distanceToClosestHit);
    
    if (hitLine) {
		hitInf->hitLine = hitLine;
		hitInf->distanceFromLineOrigin = distance(closestHit, hitLine->p1);
    }
    
    if (isReflective && maxDistance > 0.f && hitLine != 0 && depth <= 50) { //allow a maximum of 50 recursions, this is plenty and too many causes lag and stack overflows
        return castRay(closestHit, calculateReflectionAngle(*hitLine, (angle - pi)), maxDistance - distanceToClosestHit, hitInf, depth + 1, reflectionAddedDistance + distanceToClosestHit);
    }

    if (distanceToClosestHit < maxDistance) {
        hitInf->distance += reflectionAddedDistance;
        if (depth > 0) {
            hitInf->applyReflectiveModifier = true;
        }
        hitInf->hitFinished = true;
        return true;
    }

    if (depth > 0) {
        hitInf->distance = maxDistance + reflectionAddedDistance;
        hitInf->applyReflectiveModifier = true;
    }
    hitInf->hitFinished = false;
    return false;
}
