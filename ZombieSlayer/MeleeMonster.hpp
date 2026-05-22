#pragma once
#include "Framework.hpp"
#include "ObjectBase.hpp"


class MeleeMonster : public GameObject {
private:
  enum class  state
  {
    TRACE,
    DEAD,
    ATTACK
  }meleeState;
  GameObject* player;
  XMFLOAT2 moveDir;
  float moveSpeed;
  float r;

public:
  MeleeMonster(float x, float y, float z,GameObject* player,float moveSpeed=3.0f,float r) : GameObject(x, y, z) {
    
    meleeState = state::TRACE;
    this->player = player;
    this->moveSpeed = moveSpeed;
    this->r = r;
  }
  void start()  {


      moveDir.x = player->pos.x - this->pos.x;
      moveDir.y = player->pos.y - this->pos.y;
      float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);

      if (len > 0.0f)
      {
          moveDir.x /= len;
          moveDir.y /= len;
      }
  }
  void update(float dt)  {
    switch (meleeState)
    {
    case state::TRACE:
        this->pos.x += moveDir.x * moveSpeed * dt;
        this->pos.y += moveDir.y * moveSpeed * dt;
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

  void ChangeState(state nextState) {
    meleeState = nextState;
  }


};