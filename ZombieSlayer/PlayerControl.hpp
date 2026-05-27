#pragma once
#include "ObjectBase.hpp"

// -----------------------------------------------------------------------------
// [플레이어 컨트롤러 컴포넌트]
// - 키보드 입력을 받아 별을 조종하는 예시 컴포넌트
// -----------------------------------------------------------------------------
class PlayerController : public Component
{
    // 입력 상태를 저장하기 위한 멤버 변수 (내부용)
    XMFLOAT2 moveDir;  // x: 좌우, y: 상하
    XMFLOAT2 mouseWorldPos; // 마우스커서의 게임 좌표계
    HWND hWnd;

    float moveSpeed;

    // 게임 창의 너비/높이
    int* windowWidth;
    int* windowHeight;

public:
    PlayerController(HWND hWnd, int* width, int* height) : Component()
    {
        this->hWnd = hWnd;
        this->windowWidth = width;
        this->windowHeight = height;

        moveDir = { 0, 0 };
        mouseWorldPos = { 0, 0 };
        moveSpeed = 2.0f;
    }

    ~PlayerController()
    {
    }

    void Start() override
    {
    }

    // [Step 1] 입력 감지 및 상태 저장
    void Input() override
    {
        // 매 프레임 입력 상태 초기화
        moveDir = { 0, 0 };

        // WASD 입력 (이동)

        if (GetAsyncKeyState('A') & 0x8000) moveDir.x -= 1.0f;
        if (GetAsyncKeyState('D') & 0x8000) moveDir.x += 1.0f;
        if (GetAsyncKeyState('W') & 0x8000) moveDir.y += 1.0f;
        if (GetAsyncKeyState('S') & 0x8000) moveDir.y -= 1.0f;

        UpdateMousePosition();
    }

    // [Step 2] 저장된 상태를 바탕으로 데이터 갱신
    void Update(float dt) override
    {

        // 대각선 경우 속도 조절하기 위한 변수
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);

        // 위치 업데이트
        if (len > 0.0f) {
            pOwner->pos.x += (moveDir.x/len) * moveSpeed * dt;
            pOwner->pos.y += (moveDir.y/len) * moveSpeed * dt;
        }

        // 플레이어 시점 방향
        XMFLOAT2 playerViewPoint = { mouseWorldPos.x - pOwner->pos.x, mouseWorldPos.y - pOwner->pos.y };

        // 플레이어 마우스 방향으로 회전
        float playerAngle = atan2f(playerViewPoint.y, playerViewPoint.x);
        pOwner->rot.z = playerAngle;
        //printf("Rotate dir : %.4f\n", pOwner->rot.z);
    }

    void Render() override
    {

    }

    // 마우스 위치 업데이트
    void UpdateMousePosition()
    {
        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hWnd, &mousePos);

        // 화면 비율에 의한 마우스 위치 보정
        float aspect = (float)(*windowWidth) / (float)(*windowHeight);
        mouseWorldPos.x = ((float)mousePos.x / (float)(*windowWidth)) * 2.0f - 1.0f;
        mouseWorldPos.y = 1.0f - ((float)mousePos.y / (float)(*windowHeight)) * 2.0f;

        if (aspect >= 1.0f) mouseWorldPos.x *= aspect;
        else mouseWorldPos.y /= aspect;
        
        // printf("mouseWorld: %.4f, %.4f\n", mouseWorldPos.x, mouseWorldPos.y);
    }
};
