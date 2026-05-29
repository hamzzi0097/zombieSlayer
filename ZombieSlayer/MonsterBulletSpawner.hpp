#pragma once
#include "ObjectBase.hpp"
#include "Collider.hpp"
#include "MonsterBullet.hpp"
#include "MeshRenderer.hpp"
#include "RangedMonsterControl.hpp"


class MonsterBulletSpawner : public Component {
private:
    std::vector<GameObject*>* pendingObjects;
    Mesh* bulletMesh;
    Material* bulletMaterial;
    GameObject* player;
    float attackCooldown = 0.0f;      // 남은 쿨타임
    const float ATTACK_INTERVAL = 0.8f; // 발사 간격 (초)
public:
    MonsterBulletSpawner(std::vector<GameObject*>* pendingObjects, Mesh* bulletMesh, Material* bulletMaterial, GameObject* player) {
        this->pendingObjects = pendingObjects;
        this->bulletMesh = bulletMesh;
        this->bulletMaterial = bulletMaterial;
        this->player = player;
    }
    void Start() override
    {
    }

    void Input() override
    {
    }

    void Update(float dt) override
    {
        if (attackCooldown > 0.0f)
            attackCooldown -= dt;

        if (attackCooldown <= 0.0f) {
            RangedMonsterControl* rangedMonster = this->pOwner->GetComponent<RangedMonsterControl>();
            if (rangedMonster && rangedMonster->getState() == RangedState::ATTACK) {
                SpawnBullet();
                attackCooldown = ATTACK_INTERVAL;
            }
        }
    }

    void Render() override
    {
    }

    void SpawnBullet()
    {

        GameObject* bullet = new GameObject(pOwner->pos.x, pOwner->pos.y, pOwner->pos.z);

        bullet->scale = { 0.01f, 0.01f, 1.0f };
        bullet->AddComponent(new MeshRenderer(bulletMesh, bulletMaterial));
        bullet->AddComponent(new MonsterBullet(player, 0.5f));
        bullet->AddComponent(new CircleCollider(1.0f, CollisionLayer::MonsterBullet));

        pendingObjects->push_back(bullet);
    }


};