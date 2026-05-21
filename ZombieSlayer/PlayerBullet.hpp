#pragma once
#include "ObjectBase.hpp"

// player ÅºÈ¯ °ü·Ã class
class PlayerBullet : public Component
{
    XMFLOAT2 moveDir;
    float moveSpeed;

public:
    PlayerBullet(XMFLOAT2 dir, float speed = 5.0f) : Component()
    {
        moveDir = dir;
        moveSpeed = speed;

        float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);

        if (len > 0.0f)
        {
            moveDir.x /= len;
            moveDir.y /= len;
        }
    }

    void Start(GraphicsContext* gfx) override
    {
    }

    void Input() override
    {
    }

    void Update(float dt) override
    {
        pOwner->pos.x += moveDir.x * moveSpeed * dt;
        pOwner->pos.y += moveDir.y * moveSpeed * dt;
    }

    void Render(GraphicsContext* gfx) override
    {
    }
};
