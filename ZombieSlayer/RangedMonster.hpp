#pragma once
#include "Framework.hpp"
#include "ObjectBase.hpp"


class RangedMonster : public GameObject {
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

      break;
    case state::DEAD:

      break;
    case state::ATTACK:

      break;
    }
  }
};