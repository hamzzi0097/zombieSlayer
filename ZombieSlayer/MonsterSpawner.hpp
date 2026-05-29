#pragma once
#include "Framework.hpp"
#include "MeleeMonsterControl.hpp"
#include "Collider.hpp"
#include "RangedMonsterControl.hpp"

class MonsterSpawner {
private:
    XMFLOAT2 left;
    XMFLOAT2 right;
    XMFLOAT2 down;
    XMFLOAT2 up;
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<int> distrib;
  GameObject* player;
  std::vector<Vertex> monsterVertices;
  Mesh monsterMesh;
  ColorMaterial* monsterMaterial;
public:


  MonsterSpawner(GameObject* player, ShaderSet starShaders, std::vector<Vertex> monsterVertices) :left({ -1.4f,0.0f }),    right({1.4f, 0.0f}),    up({ 0.0f,1.2f }),    down  ({ 0.0f,-1.2f }), gen(rd()), distrib(1, 200),player(player){
    this->monsterVertices = monsterVertices;
    monsterMesh.Create(monsterVertices);

   monsterMaterial=new ColorMaterial(
      starShaders,
      XMFLOAT4(0.8f, 0.2f, 0.2f, 1.0f)
    );
  }

  ~MonsterSpawner() {
    delete monsterMaterial;
    monsterMaterial = nullptr;
  }

  GameObject* generationMonster(int genPos, int monster) { //genPos 1은 left, 2는 right, 3은 up, 4는 down
    float positonX=(distrib(gen)-100)/100.0 ;
    float positonY = (distrib(gen)-100)/100.0 ;
    left.y = positonY;
    right.y = positonY;
    up.x = positonX;
    down.x = positonX;
    XMFLOAT2 curPos = {0,0};
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
      GameObject* monster = new GameObject(curPos.x, curPos.y, 0.0f);
      monster->AddComponent(new MeleeMonsterControl(player, 0.5f));
      monster->AddComponent(new MeshRenderer(&monsterMesh, &*monsterMaterial));
      monster->AddComponent(new CircleCollider(1.0f, CollisionLayer::MeleeMonster));
      monster->scale = { 0.05f,0.05f,1.0f};


        return monster;
    }
    else if (monster == 1) {
      GameObject* monster = new GameObject(curPos.x, curPos.y, 0.0f);
      monster->AddComponent(new RangedMonsterControl(player, 0.5f));
      monster->AddComponent(new MeshRenderer(&monsterMesh, &*monsterMaterial));
      monster->AddComponent(new CircleCollider(1.0f, CollisionLayer::RangedMonster));
      monster->scale = { 0.05f,0.05f,1.0f };
      return monster;
      
    }
    // 몬스터 생성되는 위치만 네 방향으로 설정, 여기서 하나 불러서 사용하면 될 듯 함
  }
};
