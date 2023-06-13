#include <iostream>
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <vector>

#define pi 3.14159265358979323846f

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct simpleColour {
    char R;
    char G;
    char B;
    char A;
};

struct line {
    ImVec2 p1;
    ImVec2 p2;
    ImColor colour = ImColor(255, 255, 255);
    bool reflective = false;
    bool ignorePrimaryRays = false;
};

struct hitInfo {
    ImVec2 position = { -1, -1 };
    float distance = -1.f;
    std::vector<float> reflectionDistances = { };
    ImColor colour = ImColor(0, 0, 0);
    bool applyReflectiveModifier = false;
    int hitDepth = 0;
};


struct vert {
    float angle;
	float distance;
};

struct polygon {
    ImVec2 position;
    float rotation;
    float scale;
	std::vector<vert> vertices;
    std::vector<line> generateFrame() {
		std::vector<line> lines;
		for (int i = 0; i < vertices.size(); i++) {
			line l;
			l.p1 = { vertices[i].distance * cos(vertices[i].angle + rotation), vertices[i].distance * sin(vertices[i].angle + rotation) };
			l.p2 = { vertices[(i + 1) % vertices.size()].distance * cos(vertices[(i + 1) % vertices.size()].angle + rotation), vertices[(i + 1) % vertices.size()].distance * sin(vertices[(i + 1) % vertices.size()].angle + rotation) };
			l.p1.x *= scale;
			l.p1.y *= scale;
			l.p2.x *= scale;
			l.p2.y *= scale;
			l.p1.x += position.x;
			l.p1.y += position.y;
			l.p2.x += position.x;
			l.p2.y += position.y;
            l.colour = i % 2 == 0 ? ImColor(255, 100, 100) : ImColor(100, 100, 255);
            l.reflective = true;
            lines.push_back(l);
		}
		return lines;
    }
};

std::vector<line> lines;
std::vector<polygon> dynamics;
ImVec2 player = { -1, -1 };
float playerAngle = pi * 1.5;
bool playerInit = false;
bool linesInit = false;
ImDrawList* draw = 0;

ImVec2 normalise(ImVec2 vector) {
	float l = sqrt(vector.x * vector.x + vector.y * vector.y);
    return { vector.x / l, vector.y / l };
}

ImVec2 angleToVector(float angle) {
    return { cos(angle), sin(angle) };
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

void generateDynamicPolygon(ImVec2 position, float rotation, float scale, int sideCount) {
	polygon p;
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

void linesAddCircle(ImVec2 origin, float radius, int sides, ImColor colour, bool reflective = false) {
	float angle = pi * 2.f / (float)sides;

	ImVec2* points = new ImVec2[sides + 1];
    
    for (int i = 0; i < sides; i++) {
        points[i] = ImVec2(origin.x + (cos((angle * i)) * radius), origin.y + (sin((angle * i)) * radius));
        lines.push_back({ points[i - 1], points[i], colour, reflective });
    } 

    lines.push_back({ points[0], points[sides - 1], colour, reflective });

    delete[] points;
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

            float tMin = min(t0, t1);
            float tMax = max(t0, t1);

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

bool castRay(ImVec2 origin, float angle, float maxDistance, hitInfo* hitInf, int depth = 0, float reflectionAddedDistance = 0.f) {
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
    
    if (isReflective && maxDistance > 0.f && hitLine != 0) {
		return castRay(closestHit, calculateReflectionAngle(*hitLine, (angle - pi)), maxDistance - distanceToClosestHit, hitInf, depth + 1, reflectionAddedDistance + distanceToClosestHit);
    }
    
	if (distanceToClosestHit < maxDistance) {
        hitInf->distance += reflectionAddedDistance;
        if (depth > 0) {
            hitInf->applyReflectiveModifier = true;
        }
		return true;
	}

    if (depth > 0) {
        hitInf->distance = maxDistance + reflectionAddedDistance;
        hitInf->applyReflectiveModifier = true;
    }
    
	return false; 
}

//psuedo-3d window size
float _width = 640 / 1.5;
float _height = 480 / 1.5;

//fires 60 times per second
void timerCallback(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4) {
    if (GetKeyState(VK_LEFT) < 0) {
        playerAngle -= (pi / 2.f) / 50;
        if (playerAngle < 0.f) {
            playerAngle += (pi * 2.f);
        }
    }

    if (GetKeyState(VK_RIGHT) < 0) {
        playerAngle += (pi / 2.f) / 50;
        if (playerAngle > (pi * 2.f)) {
            playerAngle -= (pi * 2.f);
        }
    }

    if (GetKeyState(VK_UP) < 0) {
        ImVec2 forward = angleToVector(playerAngle);
        player.x += forward.x * 3.f;
        player.y += forward.y * 3.f;
    }

    if (GetKeyState(VK_DOWN) < 0) {
        ImVec2 forward = angleToVector(playerAngle);
        player.x -= forward.x * 3.f;
        player.y -= forward.y * 3.f;
    }

    if (GetKeyState(0x51) < 0) {
        ImVec2 left = angleToVector(playerAngle - (pi / 2.f));
        player.x += left.x * 3.f;
        player.y += left.y * 3.f;
    }

    if (GetKeyState(0x45) < 0) {
        ImVec2 right = angleToVector(playerAngle + (pi / 2.f));
        player.x += right.x * 3.f;
        player.y += right.y * 3.f;
    }
}


int main()
{
    srand(time(NULL));
    
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("2D Raycaster"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("2D Raycaster"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    draw = ImGui::GetForegroundDrawList();
    int cameraRayCount = 128;
    hitInfo* distances = new hitInfo[cameraRayCount] {};
    float rayMaxDistance = 1000.f;
    
    SetTimer(NULL, 1, 1000 / 60, timerCallback);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {     
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 whole_content_size = io.DisplaySize;
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration;

        ImGui::NewFrame();
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize(whole_content_size);
        ImGui::Begin("ImGui", 0, flags);

        //
        // GUI DRAW CODE HERE
        //

		ImVec2 winSize = ImGui::GetWindowSize();
        if (!playerInit) {
			player = { winSize.x / 2, winSize.y / 2 };
            playerInit = true;
        }

        if (!linesInit) {

            for (int i = 0; i < 8; i++) {
				float x1 = rand() % (int)winSize.x;
				float y1 = rand() % (int)winSize.y;
                float size = (rand() % 75) + 25;
				ImColor randColour = ImColor(rand() % 255, rand() % 255, rand() % 255);
                linesAddCircle({ x1, y1 }, size, (rand() % 20) + 3, randColour);
            }      
            
            generateDynamicPolygon({ winSize.x / 2.f, winSize.y / 4.f }, 0.f, 75.f, 3);
            
            ImVec2 A = { 30 + (_width), 5};
            ImVec2 B = { 30 + (_width), 30 + (_height)};
            ImVec2 C = { 5, 30 + (_height) };
            ImVec2 D = { 5,  winSize.y - 6 };
            ImVec2 E = { winSize.x - 6,  winSize.y - 6 };
            ImVec2 F = { winSize.x - 6,  5 };

            ImVec2 FA = { winSize.x - 6 - (30 * 5),  75};
            ImVec2 FB = { (30 * 5) + (_width),  75};
            
            lines.push_back({ A, B, ImColor(200, 200, 255) });
            lines.push_back({ B, C, ImColor(200, 200, 255) });
            lines.push_back({ C, D, ImColor(200, 200, 255) });
            lines.push_back({ D, E, ImColor(200, 200, 255) });
            lines.push_back({ E, F, ImColor(200, 200, 255) });
            lines.push_back({ F, A, ImColor(200, 200, 255), true });

            //lines.push_back({ FA, FB, ImColor(200, 200, 255), true });
            
            linesInit = true;
        }
        
        dynamics[0].rotation += 0.01f;
        
        for (line& cLine : lines) {
            if (cLine.reflective) {
				draw->AddLine(cLine.p1, cLine.p2, ImColor(100, 255, 100));
            }
            else {
                draw->AddLine(cLine.p1, cLine.p2, 0xffffffff);
            }

        }

        for (polygon& p : dynamics) {
            for (line& l : p.generateFrame()) {
                if (l.reflective) {
                    draw->AddLine(l.p1, l.p2, ImColor(100, 255, 100));
                }
                else {
                    draw->AddLine(l.p1, l.p2, 0xffffffff);
                }
            }
        }
        
		draw->AddCircleFilled(player, 5, 0xffffffff);        

        for (int i = 0; i < cameraRayCount; i++) {

            float FOV = (pi / 2.f) / 1.75f;
            float angleStep = FOV / cameraRayCount;
			float offsetAngle = (playerAngle - (FOV / 2.f)) + (angleStep * (float)i);
            
            hitInfo hinf = {};
            
            if (castRay(player, offsetAngle, rayMaxDistance, &hinf)) { }
			distances[i] = hinf;
            //draw->AddLine(player, hinf.position, 0xffffffff);
        }     

        //draw 3d environment
        
        ImVec2 rendererMin = { 15, 15 };
        ImVec2 rendererMax = { 15 + _width, 15 + _height };
		ImVec2 rendererCenterLeft = { rendererMin.x + ((rendererMax.x - rendererMin.x) / 2), rendererMin.y + ((rendererMax.y - rendererMin.y) / 2) };
		float rendererBarWidth = _width / (float)cameraRayCount;
		draw->AddRectFilled(rendererMin, rendererMax, ImColor(0, 0, 0));
        
        bool floor = true;
        if (floor) {
            int floorSegments = 25;
            float floorSegmentHeight = (_height / 2.f) / floorSegments;
            int floorBrightnessMax = 255 / 3;
            int floorBrightnessMin = 25;
            int floorBrightnessStep = (floorBrightnessMax - floorBrightnessMin) / floorSegments;

            for (int i = 0; i < floorSegments; i++) {
                ImVec2 floorMin = { rendererMin.x, rendererCenterLeft.y + (floorSegmentHeight * (float)i) };
                ImVec2 floorMax = { rendererMax.x, rendererCenterLeft.y + (floorSegmentHeight * (float)(i + 1)) };
                ImColor segmentColour = ImColor((floorBrightnessStep * (i)), (floorBrightnessStep * (i)), (floorBrightnessStep * (i)));
                draw->AddRectFilled(floorMin, floorMax, segmentColour);
				draw->AddRectFilled({ floorMin.x, rendererCenterLeft.y - (floorSegmentHeight * (float)(i + 1)) }, { floorMax.x, rendererCenterLeft.y - (floorSegmentHeight * (float)(i)) }, segmentColour);
            }
        }

        for (int i = 0; i < cameraRayCount; i++){
			float distance = distances[i].distance;

            float runningStackedDistance = 0.f;
            for (int d = distances[i].hitDepth; d >= 0; d--) {
                
                float stackedDistance = -1;
                if (d > 0) {
                    stackedDistance = distances[i].reflectionDistances[distances[i].hitDepth - d];
                    runningStackedDistance += stackedDistance;
                }
                
                ImColor colour = d == 0 ? distances[i].colour : ImColor(100, 110, 100);

                float percentageOfMaxDistance = d == 0 ? distance / rayMaxDistance : runningStackedDistance / rayMaxDistance;      
                
                float height = (1.f - percentageOfMaxDistance) * _height;
                if (height < 0.f) { height = 0.f; }

                ImVec2 barMin = { rendererMin.x + (rendererBarWidth * (float)i), rendererCenterLeft.y - (height / 2.f) };
                ImVec2 barMax = { rendererMin.x + (rendererBarWidth * (float)i) + rendererBarWidth, rendererCenterLeft.y + (height / 2.f) };

                float brightness = d == 0 ? (5.0f / (distance * distance)) * 12500.f : (5.0f / (runningStackedDistance * runningStackedDistance)) * 12500.f;
                brightness = d == 0 ? fmin(brightness, 1.5f) : fmin(brightness, 0.25f);

                float newR = colour.Value.x * brightness;
                float newG = colour.Value.y * brightness;
                float newB = colour.Value.z * brightness;
                float newA = d == 0 ? 1 : 0.5;
                
                draw->AddRectFilled(barMin, barMax, ImColor(newR, newG, newB, newA));            
            }
		}
        
        draw->AddRect(rendererMin, rendererMax, 0xffffffff);
        
        ImGui::End();
        ImGui::EndFrame();

        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * 255.0f), (int)((1.f - clear_color.y) * 255.0f), (int)(clear_color.z * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
            ResetDevice();
        }
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) {
        return false;
    }

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) {
        return false;
    }

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_KEYDOWN:
		return 0;
        
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

