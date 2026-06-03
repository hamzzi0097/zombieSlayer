#pragma once
#include "Framework.hpp"
#include "MeleeMonsterControl.hpp"
#include "Collider.hpp"
#include "RangedMonsterControl.hpp"
#include "ObjectBase.hpp"
#include "MonsterBulletSpawner.hpp"

class MonsterSpawner:public Component {
private:
    XMFLOAT2 left;
    XMFLOAT2 right;
    XMFLOAT2 down;
    XMFLOAT2 up;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<int> distrib;
    GameObject* player;
    Mesh* monsterMesh;
    std::vector<GameObject*>* pendingObjects;
    int maxMonsters;
    int spawnedCount;
    float timer;
    float spawnInterval;
    Texture         mosquitoTex;
    Texture         stinkBugTex;
    Texture         bulletTex;
    Texture         deadMosquitoTex;
    Texture         deadStinkBugTex;
    TextureMaterial* mosquitoMat = nullptr;
    TextureMaterial* stinkBugMat = nullptr;
    TextureMaterial* bulletMat = nullptr;
    TextureMaterial* deadMosquitoMat = nullptr;
    TextureMaterial* deadStinkBugMat = nullptr;
public:


    MonsterSpawner(std::vector<GameObject*>* pendingObjects, GameObject* player, ShaderSet textureShader,Mesh* monsterMesh) :
        left({ -2.0f,0.0f }),
        right({ 2.0f, 0.0f }),
        up({ 0.0f,1.2f }),
        down({ 0.0f,-1.2f }),
        gen(rd()),
        distrib(1, 200),
        player(player),
        pendingObjects(pendingObjects),
        monsterMesh(monsterMesh),
        maxMonsters(50),
        spawnedCount(0),
        timer(0.0f),
        spawnInterval(2.0f)
    {
        ID3D11Device* dev = GraphicsContext::Get()->Device;

        mosquitoTex.Load(dev, L"Mosquito.png");
        mosquitoTex.CreateSampler(dev);
        mosquitoMat = new TextureMaterial(textureShader, &mosquitoTex);

        stinkBugTex.Load(dev, L"StinkBug.png");
        stinkBugTex.CreateSampler(dev);
        stinkBugMat = new TextureMaterial(textureShader, &stinkBugTex);

        bulletTex.Load(dev, L"StinkBugBullet.png");
        bulletTex.CreateSampler(dev);
        bulletMat = new TextureMaterial(textureShader, &bulletTex);

        deadMosquitoTex.Load(dev, L"DeadMosquito.png");
        deadMosquitoTex.CreateSampler(dev);
        deadMosquitoMat = new TextureMaterial(textureShader, &deadMosquitoTex, XMFLOAT4{ 1.0f,1.0f,1.0f,0.5f });

        deadStinkBugTex.Load(dev, L"DeadStinkBug.png");
        deadStinkBugTex.CreateSampler(dev);
        deadStinkBugMat = new TextureMaterial(textureShader, &deadStinkBugTex, XMFLOAT4{ 1.0f,1.0f,1.0f,0.5f });
    }

    ~MonsterSpawner() {
        delete mosquitoMat;
        delete stinkBugMat;
        delete bulletMat;
        mosquitoMat = nullptr;
        stinkBugMat = nullptr;
        bulletMat = nullptr;
    }
    void Start()override {
    }

    void Input() override {
    
    }

    void Update(float dt) override {
        timer += dt;

        if (timer >= spawnInterval && spawnedCount < maxMonsters) {
            timer = 0.0f;

            // 랜덤 방향, 랜덤 몬스터 종류 생성
            int randomPos = (distrib(gen) % 4) + 1;       // 1~4
            int randomMonster = distrib(gen) % 2;          // 0 or 1

            generationMonster(randomPos, randomMonster);
            spawnedCount++;

            // 시간이 지날수록 간격 짧아지게
            if (spawnInterval > 0.5f)
                spawnInterval -= 0.05f;
        }

    }
    
    void Render() override {

    }

    void generationMonster(int genPos, int monster) { //genPos 1은 left, 2는 right, 3은 up, 4는 down
        float positonX = (distrib(gen) - 100) / 100.0;
        float positonY = (distrib(gen) - 100) / 100.0;
        left.y = positonY;
        right.y = positonY;
        up.x = positonX;
        down.x = positonX;
        XMFLOAT2 curPos = { 0,0 };
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
            monster->AddComponent(new MeshRenderer(monsterMesh, mosquitoMat));
            monster->AddComponent(new CircleCollider(1.0f, CollisionLayer::MeleeMonster));
            monster->scale = { 0.1f,0.1f,1.0f };

            pendingObjects->push_back(monster);
        }
        else if (monster == 1) {
            GameObject* monster = new GameObject(curPos.x, curPos.y, 0.0f);
            monster->AddComponent(new RangedMonsterControl(player, 0.5f));
            monster->AddComponent(new MeshRenderer(monsterMesh, stinkBugMat));
            monster->AddComponent(new CircleCollider(1.0f, CollisionLayer::RangedMonster));
            monster->scale = { 0.1f,0.1f,1.0f };
            monster->AddComponent(new MonsterBulletSpawner(pendingObjects, monsterMesh, bulletMat, player));
            pendingObjects->push_back(monster);
        }
        // 몬스터 생성되는 위치만 네 방향으로 설정, 여기서 하나 불러서 사용하면 될 듯 함
    }

    int getSpawnedCount() const{
        return spawnedCount;
    }
    void setSpawnedCount(int num) {
        spawnedCount = num;
    }

    TextureMaterial* deadMonsterMesh(int n) {
        switch (n) {
        case 0:

            return deadMosquitoMat;
        case 1:

            return deadStinkBugMat;
        }
    }
};
