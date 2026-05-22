#pragma once
#include "ObjectBase.hpp"

// 플레이어가 발사한 탄환의 이동 및 수명 처리를 담당
class PlayerBullet : public Component
{
    XMFLOAT2 moveDir;
    float moveSpeed;    // 초당 이동 속도
    float lifeTime; // 탄환 유지 시간

public:
    PlayerBullet(XMFLOAT2 dir, float speed = 5.0f, float lifeTime = 2.0f) : Component()
    {
        moveDir = dir;
        moveSpeed = speed;
        this->lifeTime = lifeTime;
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);

        if (len > 0.0f)
        {
            moveDir.x /= len;
            moveDir.y /= len;
        }
    }

    void Start() override
    {
    }

    void Input() override
    {
    }

    void Update(float dt) override
    {
        // 매 프레임 지정된 방향으로 탄환 이동
        pOwner->pos.x += moveDir.x * moveSpeed * dt;
        pOwner->pos.y += moveDir.y * moveSpeed * dt;

        lifeTime -= dt;

        // 수명이 끝난 탄환 GameLoop에서 제거
        if (lifeTime <= 0.0f)
        {
            pOwner->isObjDead = true;
        }
    }

    void Render() override
    {
    }
};
