#pragma once
#include "ObjectBase.hpp"
#include "Logger.hpp"

enum class  DeadState
{
    TRACE,
    DISTORY
};

class DeadMonsterController : public Component {
private:
    DeadState deadState;
    XMFLOAT2 moveDir;
    GameObject* player;
    float moveSpeed;
    float distoryTime;

public:
    DeadMonsterController(GameObject* player, float moveSpeed = 3.0f) {
        deadState = DeadState::TRACE;
        this->player = player;
        this->moveSpeed = moveSpeed;
        moveDir = { 0,0 };
        distoryTime = 2.0f;
    }

    void Start()override {


        moveDir.x = player->pos.x - this->pOwner->pos.x;
        moveDir.y = player->pos.y - this->pOwner->pos.y;
        float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);

        if (len > 0.0f)
        {
            moveDir.x /= len;
            moveDir.y /= len;

            float angle = std::atan2(moveDir.y, moveDir.x);
            this->pOwner->rot.z = angle + (3.141592f / 2.0f);
        }
        this->isStarted = true;
    }

    void Input()override {

    }

    void Update(float dt) override {

        switch (deadState)
        {

        case DeadState::TRACE:
            //플레이어 반대 이동
            distoryTime -= dt;
            this->pOwner->pos.x -= moveDir.x * moveSpeed * dt;
            this->pOwner->pos.y -= moveDir.y * moveSpeed * dt;
            if (distoryTime < 0)
                ChangeState(DeadState::DISTORY);

            break;
        case DeadState::DISTORY:
            pOwner->isObjDead = true;
            break;
        }
    }

    void Render() override {

    }

    void ChangeState(DeadState nextState) {
        deadState = nextState;
    }


};
