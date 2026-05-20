#pragma once
#include "Framework.hpp"

#define WIDTH 430
#define LENGTH 330

class CreateMonster {
private:
  std::pair<int, int> left;
  std::pair<int, int> right;
  std::pair<int, int> down;
  std::pair<int, int> up;
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<int> distrib;
public:


  CreateMonster() :left({ -WIDTH,0 }),    right({WIDTH, 0}),    up({ 0,LENGTH }),    down  ({ 0,-LENGTH }), gen(rd()), distrib(1, 800){

  }
  void generationMonster() {
    int positonX=distrib(gen)-400;
    int positonY = distrib(gen)%600 - 300;
    left.second = positonY;
    right.second = positonY;
    up.first = positonX;
    down.first = positonX;
    // 몬스터 생성되는 위치만 네 방향으로 설정, 여기서 하나 불러서 사용하면 될 듯 함
  }
};