#pragma once
#include "Framework.hpp"
#include "MeleeMonster.hpp"

#define WIDTH 430
#define LENGTH 330

class CreateMonster {
private:
    XMFLOAT2 left;
    XMFLOAT2 right;
    XMFLOAT2 down;
    XMFLOAT2 up;
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<int> distrib;
  GameObject* player;
public:


  CreateMonster(GameObject* player) :left({ -WIDTH,0 }),    right({WIDTH, 0}),    up({ 0,LENGTH }),    down  ({ 0,-LENGTH }), gen(rd()), distrib(1, 800),player(player){

  }
  void generationMonster(int monster, int genPos) { //genPos 1은 left, 2는 right, 3은 up, 4는 down
    int positonX=distrib(gen)-400;
    int positonY = distrib(gen)%600 - 300;
    left.y = positonY;
    right.y = positonY;
    up.x = positonX;
    down.x = positonX;
    XMFLOAT2 curPos;
    switch (genPos)
    {
    case 1:
        curPos = left;
        break;
    case 2:
        curPos = right;
        break;
    case 3:
        curPos = up;
        break;
    case 4:
        curPos = down;
        break;
    default:
        break;
    }
    if (monster == 0) {
        MeleeMonster* mon = new MeleeMonster(curPos.x, curPos.y,0, player, 3.0f, 2.0f);
    }
    else if (monster == 1) {

    }
    // 몬스터 생성되는 위치만 네 방향으로 설정, 여기서 하나 불러서 사용하면 될 듯 함
  }
};