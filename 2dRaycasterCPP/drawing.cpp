#include "drawing.h"
#include "globals.h"

void generateDynamicPolygon(ImVec2 position, float rotation, float scale, int sideCount, bool reflective) {
	polygon p;
	p.reflective = reflective;
	p.position = position;
	p.rotation = rotation;
	p.scale = scale;
	p.vertices.clear();
	for (int i = 0; i < sideCount; i++) {
		vert v = { 2.f * pi * (float)i / sideCount, 1.f };
		p.vertices.push_back(v);
	}
	dynamics.push_back(p);
}

void linesAddCircle(ImVec2 origin, float radius, int sides, ImColor colour, bool reflective) {
	float angle = pi * 2.f / (float)sides;

	ImVec2* points = new ImVec2[sides + 1];

	for (int i = 0; i < sides; i++) {
		points[i] = ImVec2(origin.x + (cos((angle * i)) * radius), origin.y + (sin((angle * i)) * radius));
		lines.push_back({ points[i - 1], points[i], colour, reflective });
	}

	lines.push_back({ points[0], points[sides - 1], colour, reflective });

	delete[] points;
}

void linesAddRectangle(ImVec2 p1, ImVec2 p2, ImColor colour, bool reflective) {
	lines.push_back({ p1, ImVec2(p1.x, p2.y), colour, reflective });
	lines.push_back({ ImVec2(p1.x, p2.y), p2, colour, reflective });
	lines.push_back({ p2, ImVec2(p2.x, p1.y), colour, reflective });
	lines.push_back({ ImVec2(p2.x, p1.y), p1, colour, reflective });
}

void linesAddRectangle(ImVec2 origin, float width, float height, ImColor colour, bool reflective) {
	linesAddRectangle(origin, { origin.x + width, origin.y + height }, colour, reflective);
}
