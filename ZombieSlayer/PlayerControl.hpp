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
    float moveSpeed;


public:
    PlayerController() : Component()
    {
        moveDir = { 0, 0 };
        moveSpeed = 2.0f;
    }

    ~PlayerController()
    {
    }

    void Start(GraphicsContext* gfx) override
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
    }

    // [Step 2] 저장된 상태를 바탕으로 데이터 갱신
    void Update(float dt) override
    {
        // 1. 대각선 경우 속도 조절하기 위한 변수
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);

        // 2. 위치 업데이트
        if (len > 0.0f) {
            pOwner->pos.x += (moveDir.x/len) * moveSpeed * dt;
            pOwner->pos.y += (moveDir.y/len) * moveSpeed * dt;
        }

    }

    void Render(GraphicsContext* gfx) override
    {

    }
};