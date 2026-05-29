#pragma once
#include "ObjectBase.hpp"

// 충돌 대상을 구분하기 위한 레이어
enum class CollisionLayer
{
    Player,
    PlayerBullet,
    MeleeMonster,
    RangedMonster,
    MonsterBullet
};

enum class ColliderType
{
    Circle,
    Box
};

class Collider : public Component
{
public:
    CollisionLayer layer;

    Collider(CollisionLayer layer) {
        this->layer = layer;
    }

    virtual ColliderType GetType() const = 0;
    virtual bool CheckCollision(Collider* collision) = 0;
};

// Circle Collision 컴포넌트
class CircleCollider : public Collider
{
public:
    float radius;

    CircleCollider(float radius, CollisionLayer layer) : Collider(layer)
    {
        this->radius = radius;
    }

    ColliderType GetType() const override
    {
        return ColliderType::Circle;
    }

    // Collision 충돌 여부 판단
    bool CheckCollision(Collider* other) override
    {
        // 일단 원<->원 사이의 충돌만 판단
        if (other->GetType() != ColliderType::Circle)
            return false;

        CircleCollider* otherCircle = dynamic_cast<CircleCollider*>(other);
        if (!otherCircle)
            return false;

        float dx = pOwner->pos.x - otherCircle->pOwner->pos.x;
        float dy = pOwner->pos.y - otherCircle->pOwner->pos.y;

        float distSquare = dx * dx + dy * dy;
        float radiusSum = GetWorldRadius() + otherCircle->GetWorldRadius();

        // Circle Collision 충돌 : (두 원의 중심 사이 거리의 제곱 <= 두 원의 반지름 합의 제곱) -> 충돌
        return distSquare <= radiusSum * radiusSum;
    }

    // GameObject의 scale 반영 실제 월드 반지름
    float GetWorldRadius()
    {
        return radius * pOwner->scale.x;
    }

    void Start() override {}

    void Input() override {}

    void Update(float dt) override {}

    void Render() override {}

};
