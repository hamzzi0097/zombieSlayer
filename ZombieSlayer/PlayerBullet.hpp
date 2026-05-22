#pragma once
#include "ObjectBase.hpp"

// player 탄환 관련 class
class PlayerBullet : public Component
{
    XMFLOAT2 moveDir;
    float moveSpeed;
    float lifeTime;

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
        pOwner->pos.x += moveDir.x * moveSpeed * dt;
        pOwner->pos.y += moveDir.y * moveSpeed * dt;

        lifeTime -= dt;

        if (lifeTime <= 0.0f)
        {
            pOwner->isObjDead = true;
        }
    }

    void Render() override
    {
    }
};
