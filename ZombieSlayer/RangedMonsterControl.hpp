#pragma once
#include "ObjectBase.hpp"
#include "Collider.hpp"

enum class RangedState
{
  TRACE,
  DEAD,
  ATTACK
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
    moveDir.x = player->pos.x - this->pOwner->pos.x;
    moveDir.y = player->pos.y - this->pOwner->pos.y;
    float len = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y);

    if (len > 0.0f)
    {
      moveDir.x /= len;
      moveDir.y /= len;
    } 
    if (0.8f <= len && len <= 0.82f) {
      ChangeState(RangedState::ATTACK);
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
    case RangedState::ATTACK:
      if ( len > 0.82f) {
        ChangeState(RangedState::TRACE);
      }

      break;
    }
  }

  void Render() override {

  }

  void ChangeState(RangedState nextState) {
    rangedState = nextState;
  }
  void getDamaged(int damage) {
    hp -= damage;
    if (hp <= 0) {
      ChangeState(RangedState::DEAD);
    }
  }
  void OnCollision(GameObject* obj) override
  {
    Collider* curObject = obj->GetComponent<Collider>();
    if (curObject && curObject->layer == CollisionLayer::Player) {
      ChangeState(RangedState::ATTACK);
    }
  }
  RangedState getState() {
    return rangedState;
  }
};