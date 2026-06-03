#pragma once
#include "ObjectBase.hpp"
#include "Collider.hpp"
#include "PlayerHealth.hpp"
enum class  BulletState
{
    TRACE,
    DEAD,
    ATTACK
};

class MonsterBullet : public Component {
private:
    BulletState bulletState;
    XMFLOAT2 moveDir;
    GameObject* player;
    float moveSpeed;
    float lifeTime;

public:
    MonsterBullet(GameObject* player, float moveSpeed = 3.0f) {
        bulletState = BulletState::TRACE;
        this->player = player;
        this->moveSpeed = moveSpeed;
        moveDir = { 0.0f,0.0f };
        lifeTime = 5.0f;
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
        switch (bulletState)
        {

        case BulletState::TRACE:

            this->pOwner->pos.x += moveDir.x * moveSpeed * dt;
            this->pOwner->pos.y += moveDir.y * moveSpeed * dt;

            lifeTime -= dt;
            if (lifeTime <= 0.0f)
            {
                ChangeState(BulletState::DEAD);
            }
            break;
        case BulletState::DEAD:
            pOwner->isObjDead = true;
            break;
        case BulletState::ATTACK:
            PlayerHealth* playerHealth = player->GetComponent<PlayerHealth>();

            if (playerHealth)
            {
                playerHealth->TakeDamage();
            }
            else
            {
                player->isObjDead = true;
            }

            pOwner->isObjDead = true;
            break;
        }
    }

    void Render() override {

    }

    void ChangeState(BulletState nextState) {
        bulletState = nextState;
    }
    void OnCollision(GameObject* obj) override
    {
        Collider* curObject = obj->GetComponent<Collider>();
        if (curObject && curObject->layer == CollisionLayer::Player) {
            ChangeState(BulletState::ATTACK);
        }
    }
};
