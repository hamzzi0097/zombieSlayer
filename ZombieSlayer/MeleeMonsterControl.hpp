#pragma once
#include "ObjectBase.hpp"
#include "Collider.hpp"
#include "Logger.hpp"
#include "PlayerHealth.hpp"

enum class  MeleeState
{
  TRACE,
  DEAD,
  ATTACK
};

class MeleeMonsterControl : public Component {
private:
  MeleeState meleeState;
    XMFLOAT2 moveDir;
    GameObject* player;
    float moveSpeed;
    int hp;

public:
    MeleeMonsterControl(GameObject* player, float moveSpeed = 3.0f) {
        meleeState = MeleeState::TRACE;
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
              moveDir.x = player->pos.x - this->pOwner->pos.x;
        moveDir.y = player->pos.y - this->pOwner->pos.y;
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);

        if (len > 0.0f)
        {
            moveDir.x /= len;
            moveDir.y /= len;
        }
        switch (meleeState)
        {

        case MeleeState::TRACE:
            //플레이어한테 이동

            this->pOwner->pos.x += moveDir.x * moveSpeed * dt;
            this->pOwner->pos.y += moveDir.y * moveSpeed * dt;
            break;
        case MeleeState::DEAD:
            pOwner->isObjDead = true;
            break;
        case MeleeState::ATTACK:
            PlayerHealth* playerHealth = player->GetComponent<PlayerHealth>();

            if (playerHealth)
            {
                playerHealth->TakeDamage();
            }

            ChangeState(MeleeState::TRACE);
            break;
        }
    }

    void Render() override {

    }

    void ChangeState(MeleeState nextState) {
        meleeState = nextState;
        LOG_DEBUG("Next State : %d", nextState);
    }
    void getDamaged(int damage) {
        hp -= damage;
        if (hp <= 0) {
            ChangeState(MeleeState::DEAD);
        }
    }
    void OnCollision(GameObject* obj) override
    {
        Collider* curObject = obj->GetComponent<Collider>();
        if (curObject && curObject->layer == CollisionLayer::Player)
        {
            PlayerHealth* playerHealth = obj->GetComponent<PlayerHealth>();

            if (playerHealth && playerHealth->IsInvincible())
            {
                return;
            }

            ChangeState(MeleeState::ATTACK);
        }
    }
};
