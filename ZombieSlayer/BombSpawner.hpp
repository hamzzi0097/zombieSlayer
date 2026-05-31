#pragma once
#include "ObjectBase.hpp"
#include "MeshRenderer.hpp"
#include "Bomb.hpp"
#include "Logger.hpp"

class BombSpawner : public Component
{
private:
    std::vector<GameObject*>* world;
    std::vector<GameObject*>* pendingObjects;

    Mesh* bombMesh;
    ShaderSet shader;

    bool wasRightMouseDown = false;
    float cooldown = 1.0f;
    float cooldownTimer = 0.0f;

public:
    BombSpawner(std::vector<GameObject*>* world, std::vector<GameObject*>* pendingObjects, Mesh* bombMesh, ShaderSet shader)
    {
        this->world = world;
        this->pendingObjects = pendingObjects;
        this->bombMesh = bombMesh;
        this->shader = shader;
    }

    void Start() override {}

    void Input() override
    {
        bool isRightMouseDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

        if (isRightMouseDown && !wasRightMouseDown && cooldownTimer <= 0.0f)
        {
            SpawnBomb();
            cooldownTimer = cooldown;
        }

        wasRightMouseDown = isRightMouseDown;
    }

    void Update(float dt) override
    {
        if (cooldownTimer > 0.0f)
            cooldownTimer -= dt;
    }

    void Render() override {}

private:
    void SpawnBomb()
    {
        ColorMaterial* bombMaterial = new ColorMaterial(shader, XMFLOAT4(1.0f, 0.8f, 0.1f, 1.0f));

        GameObject* bomb = new GameObject(
            pOwner->pos.x,
            pOwner->pos.y,
            pOwner->pos.z
        );

        bomb->scale = { 0.06f, 0.06f, 1.0f };
        bomb->AddComponent(new MeshRenderer(bombMesh, bombMaterial));
        bomb->AddComponent(new Bomb(world, bombMaterial));

        pendingObjects->push_back(bomb);

        LOG_INFO("Bomb placed.");
    }
};