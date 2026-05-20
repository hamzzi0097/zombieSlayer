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

public:
  void start() {
    meleeState = state::TRACE;
  }
  void update() {
    switch (meleeState)
    {
    case state::TRACE:
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