#include <iostream>
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <vector>

#include "globals.h"
#include "structures.h"
#include "logics.h"
#include "drawing.h"

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ImVec2 clampBarSize(ImVec2 barPos){
    if (barPos.y < rendererMin.y) {
        //partially out of bounds, above min
        barPos.y = rendererMin.y;
    }

    if (barPos.y > rendererMax.y) {
        //partially out of bounds, below max
        barPos.y = rendererMax.y;
    }

    return barPos;
}

void drawLineSegmented(ImVec2 min, ImVec2 max, ImColor segmentColours[], int segmentColourCount, float brightness) {
    float lineSizeY = max.y - min.y;
    float segmentSizeY = lineSizeY / segmentColourCount;
    for (int i = 0; i < segmentColourCount; i++) {
        ImVec2 segmentMin = { min.x, min.y + (segmentSizeY * i) };
        ImVec2 segmentMax = { max.x, min.y + (segmentSizeY * (i + 1)) };


        if (segmentMax.y < rendererMin.y) {
            //totally out of bounds, above min
            continue;
        }

        if (segmentMin.y > rendererMax.y) {
            //totally out of bounds, below max
            continue;
        }
      
        if (segmentMin.y < rendererMin.y){
      	    //partially out of bounds, above min
            segmentMin.y = rendererMin.y;
        }
      
        if (segmentMax.y > rendererMax.y) {
      	    //partially out of bounds, below max
            segmentMax.y = rendererMax.y;
        }
        
        float newR = segmentColours[i].Value.x * brightness;
        float newG = segmentColours[i].Value.y * brightness;
        float newB = segmentColours[i].Value.z * brightness;

        draw->AddRectFilled(segmentMin, segmentMax, ImColor(newR, newG, newB));
    }
}

ImColor testRainbow[25] = { ImColor(255, 0, 0),
    ImColor(255, 64, 0),
    ImColor(255, 128, 0),
    ImColor(255, 191, 0),
    ImColor(255, 255, 0),
    ImColor(191, 255, 0),
    ImColor(128, 255, 0),
    ImColor(64, 255, 0),
    ImColor(0, 255, 0),
    ImColor(0, 255, 64),
    ImColor(0, 255, 128),
    ImColor(0, 255, 191),
    ImColor(0, 255, 255),
    ImColor(0, 191, 255),
    ImColor(0, 128, 255),
    ImColor(0, 64, 255),
    ImColor(0, 0, 255),
    ImColor(64, 0, 255),
    ImColor(128, 0, 255),
    ImColor(191, 0, 255),
    ImColor(255, 0, 255),
    ImColor(255, 0, 191),
    ImColor(255, 0, 128),
    ImColor(255, 0, 64),
    ImColor(255, 0, 0) 
};

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

void drawRectangleWithTexture(ImVec2 min, ImVec2 max, int textureId) {
	ImVec2 uv0 = ImVec2(0, 0);
	ImVec2 uv1 = ImVec2(1, 1);
	ImGui::GetWindowDrawList()->AddImage((void*)textureId, min, max, uv0, uv1);
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
    float rayMaxDistance = 5000.f;
    
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
            generateDynamicPolygon({ winSize.x / 2.f, winSize.y / 4.f }, 0.f, 75.f, 3);
            
            ImVec2 A = { 30 + (_width), 5};
            ImVec2 B = { 30 + (_width), 30 + (_height)};
            ImVec2 C = { 5, 30 + (_height) };
            ImVec2 D = { 5,  winSize.y - 6 };
            ImVec2 E = { winSize.x - 6,  winSize.y - 6 };
            ImVec2 F = { winSize.x - 6,  5 };

            ImVec2 FA = { winSize.x - 6 - (30 * 5),  75};
            ImVec2 FB = { (30 * 5) + (_width),  75};

            ImVec2 WAA = { 30 + (_width), 30 + (_height) };
            ImVec2 WAB = { 30 + (_width), 100 + 30 + (_height) };
            
            ImVec2 WBA = { 30 + (_width), winSize.y - 6 };
            ImVec2 WBB = { 30 + (_width), winSize.y - 100 - 6 };

            ImVec2 WAAA = { 30 + (_width) - 25, 30 + (_height) };
            ImVec2 WABB = { 30 + (_width) - 25, 100 + 30 + (_height) };

            ImVec2 WBAA = { 30 + (_width) - 25, winSize.y - 6 };
            ImVec2 WBBB = { 30 + (_width) - 25, winSize.y - 100 - 6 };

            lines.push_back({ A, B, ImColor(200, 200, 255) });
            lines.push_back({ WAAA, C, ImColor(255, 255, 255), true });
            lines.push_back({ C, D, ImColor(200, 200, 255) });
            lines.push_back({ D, E, ImColor(200, 200, 255) });
            lines.push_back({ E, F, ImColor(200, 200, 255) });
            lines.push_back({ F, A, ImColor(200, 200, 255) });

            lines.push_back({ WAA, WAB, ImColor(200, 200, 255) });
            lines.push_back({ WAAA, WABB, ImColor(200, 200, 255) });
            lines.push_back({ WABB, WAB, ImColor(200, 200, 255) });

            generateDynamicPolygon({ (D.x + WAAA.x) / 2.f, (WAAA.y + D.y) / 2.f }, 0.f, 25.f, 4);
            
            linesInit = true;
        }
        
        dynamics[0].rotation += 0.005f;
        dynamics[1].rotation -= 0.005f;

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

		ImVec2 rendererCenterLeft = { rendererMin.x + ((rendererMax.x - rendererMin.x) / 2), rendererMin.y + ((rendererMax.y - rendererMin.y) / 2) };
		float rendererBarWidth = _width / (float)cameraRayCount;
		draw->AddRectFilled(rendererMin, rendererMax, ImColor(0, 0, 0));
        
        //draw the floor and ceiling

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

        //draw the actual frame

        float frameTotalBrightness = 0.f;
        float frameBrightnessAverage = 0.f;
        bool overExposure = false;
        for (int i = 0; i < cameraRayCount; i++){
			float distance = distances[i].distance;

            float runningStackedDistance = 0.f;
            for (int d = distances[i].hitDepth; d >= 0; d--) {
               
                if (d > 0) {
                    runningStackedDistance += distances[i].reflectionDistances[distances[i].hitDepth - d];
                }
                
                ImColor colour = d == 0 ? distances[i].colour : ImColor(100, 110 + ((distances[i].hitDepth - d) * 10), 100);

                float apparentSize = d == 0 ? _height * _focalLength / distance : _height * _focalLength / runningStackedDistance;
                float height = apparentSize * _height;
                if (d == 0 && !distances[i].hitFinished) {
                    height = 0.f;
                }

                //if (height < 0.f) { height = 0.f; }
                //if (height > _height) { height = _height; }

                ImVec2 barMin = { rendererMin.x + (rendererBarWidth * (float)i), rendererCenterLeft.y - (height / 2.f) };
                ImVec2 barMax = { rendererMin.x + (rendererBarWidth * (float)i) + rendererBarWidth, rendererCenterLeft.y + (height / 2.f) };

                float brightness = d == 0 ? (_lumens / (distance * distance)) * _candella : (_lumens / (runningStackedDistance * runningStackedDistance)) * _candella;

                if (d == 0) {
                    frameTotalBrightness += brightness;
                    frameBrightnessAverage = frameTotalBrightness / i;

                    if (brightness > 1.f) {
                        overExposure = true;
                    }
                }

                brightness = d == 0 ? fmin(brightness, 1.5f) : fmin(brightness, 0.25f);
                brightness = fmax(brightness, 0.1f);

                float gBrightnessModifier = (float)((float)((distances[i].hitDepth - d) * 5.f) / 255.f);

                float newR = colour.Value.x * brightness;
                float newG = colour.Value.y * brightness;
                float newB = colour.Value.z * brightness;
                float newA = d == 0 ? 1 : 0.5;
                
                if(d == 0){
                    drawLineSegmented(barMin, barMax, testRainbow, 25, brightness);
                }
                else {
                    draw->AddRectFilled(clampBarSize(barMin), clampBarSize(barMax), ImColor(newR, newG + gBrightnessModifier, newB, newA));
                }

            }
		}

        if (overExposure && _lumens > 0.5f) {
            _lumens -= 0.025f;
            printf_s("Lumens: %0.2f | Candella: %0.2f\n", _lumens, _candella);
        }
        else if (frameBrightnessAverage < 0.25f && _lumens < 5.f) {
            _lumens += 0.025f;
            printf_s("Lumens: %0.2f | Candella: %0.2f\n", _lumens, _candella);
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
        if (wParam == VK_OEM_PLUS)
        {
            _focalLength += 0.01f;
            printf_s("FC: %0.2f\n", _focalLength);
        }
        if (wParam == VK_OEM_MINUS)
        {
            _focalLength -= 0.01f;
            printf_s("FC: %0.2f\n", _focalLength);
        }

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

