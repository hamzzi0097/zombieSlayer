#pragma once
#include "Framework.hpp"

class CreateMonster {
private:
  std::pair<int, int> left;
  std::pair<int, int> right;
  std::pair<int, int> down;
  std::pair<int, int> up;

public:

  CreateMonster() {
    left = { -1,0 };
    right = { 1,0 };
    up = { 0,1 };
    down = { 0,-1 };
  }

};