#include "ui.hxx"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#pragma comment (lib, "d3d11.lib")

#include <d3d11.h>
#include <Windows.h>
#include <WinUser.h>

static ID3D11Device* d3dDevice = nullptr;
static ID3D11DeviceContext* d3dDeviceContext = nullptr;
static IDXGISwapChain* d3dSwapChain = nullptr;
static UINT resizeWidth = 0, resizeHeight = 0;
static ID3D11RenderTargetView* mainRenderTargetView = nullptr;

static bool done;
static WNDCLASSEXW wc;
static HWND handle;

bool CreateDeviceD3D(HWND handle);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam);

ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

typedef struct State {
    bool showDeviceList;
} State;

static State state;

void Render();

bool UI::InitializeBackend() {
	wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Simutrain Console", nullptr };
	::RegisterClassExW(&wc);
	handle = ::CreateWindowW(wc.lpszClassName, L"FFB Tool", WS_OVERLAPPEDWINDOW, 100, 100, WINDOW_INITIAL_WIDTH, WINDOW_INITIAL_HEIGHT, nullptr, nullptr, wc.hInstance, nullptr);

	if (!CreateDeviceD3D(handle)) {
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
	}

	::ShowWindow(handle, SW_SHOWDEFAULT);
	::UpdateWindow(handle);

    IMGUI_CHECKVERSION();
    UI::context = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(handle);
    ImGui_ImplDX11_Init(d3dDevice, d3dDeviceContext);
    return true;
}


// Forward message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Window Message Handler
//
// Handles messages sent to the application from Windows.
//
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

	switch (msg) {
		// Handle window resize messages
	case WM_SIZE:
		// Handle minimizing by doing nothing
		if (wParam == SIZE_MINIMIZED) {
			return 0;
		}

		// Make resize
		resizeWidth = (UINT)LOWORD(lParam);
		resizeHeight = (UINT)HIWORD(lParam);
		return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) {
            return 0;
        }
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
	}

	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width  = WINDOW_INITIAL_WIDTH;
    sd.BufferDesc.Height = WINDOW_INITIAL_HEIGHT;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 120;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    UINT createDeviceFlags = 0;

    createDeviceFlags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if (_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;

    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, 
        D3D_DRIVER_TYPE_HARDWARE, 
        nullptr,
        createDeviceFlags, 
        featureLevelArray, 
        2, 
        D3D11_SDK_VERSION, 
        &sd, 
        &d3dSwapChain, 
        &d3dDevice, 
        &featureLevel, 
        &d3dDeviceContext
	);

    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, 
            D3D_DRIVER_TYPE_WARP, 
            nullptr, 
            createDeviceFlags, 
            featureLevelArray, 
            2, 
            D3D11_SDK_VERSION, 
            &sd, 
            &d3dSwapChain, 
            &d3dDevice,
            &featureLevel, 
            &d3dDeviceContext
		);

    if (res != S_OK)
        return false;

    return true;
}

void UI::TeardownBackend() {

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(handle);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (d3dSwapChain) { d3dSwapChain->Release(); d3dSwapChain = nullptr; }
    if (d3dDeviceContext) { d3dDeviceContext->Release(); d3dDeviceContext = nullptr; }
    if (d3dDevice) { d3dDevice->Release(); d3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer;
    d3dSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &mainRenderTargetView);

    backBuffer->Release();
}


void CleanupRenderTarget()
{
    if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = nullptr; }
}


void UI::Loop() {

	while (!done) {
		MSG msg;

		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				done = true;
			}
		}

		if (done) {
			break;
		}

        // Handle resizing
        if (resizeWidth != 0 && resizeHeight != 0) {
            CleanupRenderTarget();
            d3dSwapChain->ResizeBuffers(0, resizeWidth, resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            resizeWidth = resizeHeight = 0;
            CreateRenderTarget();
        }

        // Begin a new frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

        UI::Render();

        // Render the frame
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        d3dDeviceContext->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        d3dDeviceContext->ClearRenderTargetView(mainRenderTargetView, clear_color_with_alpha);
        ImGui::Render();

        ImDrawData* drawData = ImGui::GetDrawData();
        
        ImGui_ImplDX11_RenderDrawData(drawData);
        d3dSwapChain->Present(1, 0);
	}
}

