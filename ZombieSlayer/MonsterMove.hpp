#pragma once
#include "ObjectBase.hpp"

class MonsterMove : public Component {
private:
  enum class  state
  {
    TRACE,
    DEAD,
    ATTACK
  }meleeState;
  XMFLOAT2 moveDir;
  GameObject* player;
  float moveSpeed;


public:
  MonsterMove(GameObject* player, float moveSpeed = 3.0f) {
    meleeState = state::TRACE;
    this->player = player;
    this->moveSpeed = moveSpeed;
    moveDir = { 0,0 };
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
    switch (meleeState)
    {
    case state::TRACE:
      this->pOwner->pos.x += moveDir.x * moveSpeed * dt;
      this->pOwner->pos.y += moveDir.y * moveSpeed * dt;
      //플레이어한테 이동

      //범위체크=> 여기서 공격할 지, 죽을 지 정함
      break;
    case state::DEAD:
      delete this;
      break;
    case state::ATTACK:
      // 근거리라 일단 보류
      break;
    }
  }

   void Render() override {

  }

  void ChangeState(state nextState) {
    meleeState = nextState;
  }
};