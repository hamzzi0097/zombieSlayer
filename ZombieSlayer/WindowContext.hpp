#pragma once
#include "Framework.hpp"

class WindowContext {
public:
    HWND hWnd;
    int Width, Height;
    LPCWSTR windowName;
    bool isFullscreen = false;   // borderless 전체화면 여부

    // 창모드 공통 스타일 (크기 조절·최대화 비활성 — Initialize/토글이 공유)
    static constexpr DWORD WINDOWED_STYLE =
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;

    WindowContext(LPCWSTR winName = L"DX11 Engine")
        : windowName(winName), hWnd(nullptr), Width(800), Height(600) {
    }

    ~WindowContext() {
        UnregisterClass(L"DX11Engine", GetModuleHandle(NULL));
    }

    bool Initialize(HINSTANCE hInst, int w, int h, LRESULT(CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM)) {
        Width = w; Height = h;

        WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = wndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"DX11Engine";

        if (!RegisterClassEx(&wc)) return false;

        RECT rc = { 0, 0, w, h };
        AdjustWindowRect(&rc, WINDOWED_STYLE, FALSE);

        hWnd = CreateWindow(L"DX11Engine", windowName, WINDOWED_STYLE,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
            NULL, NULL, hInst, NULL);

        if (!hWnd) return false;

        ShowWindow(hWnd, SW_SHOW);
        return true;
    }

    // 전체화면(borderless) <-> 1920x1080 창모드 토글.
    // DXGI 독점 전체화면(SetFullscreenState) 대신 창 스타일을 팝업으로 바꿔
    // 모니터를 덮는 방식 — alt-tab/멀티모니터에 안정적이고 타이틀바도 사라진다.
    // 호출 후 Width/Height가 실제 클라이언트 크기로 갱신되므로, 호출자는
    // GraphicsContext::Resize(Width, Height)로 백버퍼를 맞춰주면 된다.
    void ToggleFullscreen() {
        if (!isFullscreen) {
            // → borderless 전체화면: 현재 창이 속한 모니터 전체를 덮음
            HMONITOR hmon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfo(hmon, &mi);

            SetWindowLongPtr(hWnd, GWL_STYLE, (LONG_PTR)(WS_POPUP | WS_VISIBLE));
            SetWindowPos(hWnd, HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            isFullscreen = true;
        }
        else {
            // → 1920x1080 창모드 (좌상단 약간 띄워 배치)
            RECT rc = { 0, 0, 1920, 1080 };
            AdjustWindowRect(&rc, WINDOWED_STYLE, FALSE);

            SetWindowLongPtr(hWnd, GWL_STYLE, (LONG_PTR)WINDOWED_STYLE);
            SetWindowPos(hWnd, HWND_NOTOPMOST, 50, 50,
                rc.right - rc.left, rc.bottom - rc.top,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            isFullscreen = false;
        }

        // 실제 클라이언트 영역을 읽어 Width/Height 갱신 (뷰포트·종횡비의 기준)
        RECT cr;
        GetClientRect(hWnd, &cr);
        Width = cr.right - cr.left;
        Height = cr.bottom - cr.top;
    }
};