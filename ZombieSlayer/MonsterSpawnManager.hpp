#pragma once
#include "Framework.hpp"
#include "MonsterSpawner.hpp"

class MonsterSpawnManager  {
private:
    MonsterSpawner* monSpawn;
public:
    MonsterSpawnManager(std::vector<GameObject*>* pendingObjects, GameObject* player, ShaderSet starShaders, std::vector<Vertex> monsterVertices) 
    {
        monSpawn = new MonsterSpawner(pendingObjects, player, starShaders, monsterVertices);
    }

};
