#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include "GameLoop.hpp"
#include "ScreenShake.hpp"
#include "MeshRenderer.hpp"
#include "PlayerControl.hpp"
#include "PlayerBullet.hpp"
#include "PlayerBulletSpawner.hpp"
#include "Logger.hpp"
#include "MonsterSpawner.hpp"
#include "Collider.hpp"
#include "MonsterBulletSpawner.hpp"
#include "HeartUI.hpp"

// GraphicsContext 싱글톤 인스턴스 정의
GraphicsContext* GraphicsContext::s_instance = nullptr;

// -----------------------------------------------------------------------------
// [윈도우 메시지 처리기]
// -----------------------------------------------------------------------------
LRESULT CALLBACK GlobalWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(h, m, w, l);
}

// -----------------------------------------------------------------------------
// [메인 엔트리 포인트]
// -----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int nS)
{
    // 엔진 매니저 초기화
    GameLoop gEngine;
    if (!gEngine.Initialize(hI, GlobalWndProc)) {
        LOG_ERROR("Engine initialization failed.");
        return -1;
    }

    LOG_DEBUG("GameLoop Start!");
    gEngine.Run();

    return 0;
}
