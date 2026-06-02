#pragma once
#include "ObjectBase.hpp"
#include "Collider.hpp"
#include "PlayerHealth.hpp"

enum class RangedState
{
    TRACE,
    DEAD,
    RANGEDATTACK,
    MELEEATTACK
};

class RangedMonsterControl : public Component {
private:
    RangedState rangedState;
    XMFLOAT2 moveDir;
    GameObject* player;
    float moveSpeed;
    int hp;

public:
    RangedMonsterControl(GameObject* player, float moveSpeed = 3.0f) {
        rangedState = RangedState::TRACE;
        this->player = player;
        this->moveSpeed = moveSpeed;
        moveDir = { 0,0 };
        hp = 5;
    }

    void Start()override {


        moveDir.x = player->pos.x - this->pOwner->pos.x;
        moveDir.y = player->pos.y - this->pOwner->pos.y;
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        if (len > 0.0f)
        {
            moveDir.x /= len;
            moveDir.y /= len;
        }
        this->isStarted = true;
    }

    void Input()override {

    }

    void Update(float dt) override {
        if (hp <= 0) {
            ChangeState(RangedState::DEAD);
        }
        moveDir.x = player->pos.x - this->pOwner->pos.x;
        moveDir.y = player->pos.y - this->pOwner->pos.y;
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);

        if (len > 0.0f)
        {
            moveDir.x /= len;
            moveDir.y /= len;

            float angle = std::atan2(moveDir.y, moveDir.x);
            this->pOwner->rot.z = angle - (3.141592f / 2.0f);
        }
        if (len <= 0.8f) {
            ChangeState(RangedState::RANGEDATTACK);
        }
        switch (rangedState)
        {

        case RangedState::TRACE:
            //플레이어한테 이동

            this->pOwner->pos.x += moveDir.x * moveSpeed * dt;
            this->pOwner->pos.y += moveDir.y * moveSpeed * dt;
            break;
        case RangedState::DEAD:
            pOwner->isObjDead = true;
            break;
        case RangedState::RANGEDATTACK:
            if (len > 0.82f) {
                ChangeState(RangedState::TRACE);
            }

            break;
        case RangedState::MELEEATTACK:
            PlayerHealth* playerHealth = player->GetComponent<PlayerHealth>();

            if (playerHealth)
            {
                playerHealth->TakeDamage();
            }
            ChangeState(RangedState::TRACE);
        }
    }

    void Render() override {

    }

    void ChangeState(RangedState nextState) {
        rangedState = nextState;
    }

    void getDamaged(int damage) {
        hp -= damage;
    }

    void OnCollision(GameObject* obj) override
    {
        Collider* curObject = obj->GetComponent<Collider>();
        if (curObject && curObject->layer == CollisionLayer::Player) {
            ChangeState(RangedState::MELEEATTACK);
        }
    }

    RangedState getState() {
        return rangedState;
    }
};