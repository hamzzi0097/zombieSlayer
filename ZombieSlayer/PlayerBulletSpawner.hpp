#pragma once
#include "ObjectBase.hpp"
#include "MeshRenderer.hpp"
#include "PlayerBullet.hpp"

class PlayerBulletSpawner : public Component
{
    std::vector<GameObject*>* world;
    Mesh* bulletMesh;
    Material* bulletMaterial;

    HWND hWnd;
    int* windowWidth;
    int* windowHeight;

    bool wasLeftMouseDown;
    float spawnDelay;

public:
    PlayerBulletSpawner(std::vector<GameObject*>* world, Mesh* bulletMesh,
        Material* bulletMaterial, HWND hWnd, int* width, int* height) : Component()
    {
        this->world = world;
        this->bulletMesh = bulletMesh;
        this->bulletMaterial = bulletMaterial;
        this->hWnd = hWnd;
        this->windowWidth = width;
        this->windowHeight = height;

        wasLeftMouseDown = false;
        spawnDelay = 0.5f;
    }

    void Start(GraphicsContext* gfx) override
    {
    }

    void Input() override
    {
        bool isLeftMouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (isLeftMouseDown && !wasLeftMouseDown)
        {
            SpawnBullet();
        }

        wasLeftMouseDown = isLeftMouseDown;
    }

    void Update(float dt) override
    {
    }

    void Render(GraphicsContext* gfx) override
    {
    }

private:
    XMFLOAT2 GetMousePosition()
    {
        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hWnd, &mousePos);

        XMFLOAT2 mouseWorldPos;
        mouseWorldPos.x = ((float)mousePos.x / (float)(*windowWidth)) * 2.0f - 1.0f;
        mouseWorldPos.y = 1.0f - ((float)mousePos.y / (float)(*windowHeight)) * 2.0f;

        return mouseWorldPos;
    }

    void SpawnBullet()
    {
        XMFLOAT2 mouseWorldPos = GetMousePosition();

        XMFLOAT2 fireDir = { mouseWorldPos.x - pOwner->pos.x, mouseWorldPos.y - pOwner->pos.y };

        float len = sqrtf(fireDir.x * fireDir.x + fireDir.y * fireDir.y);

        if (len >= 0.0f)
        {
            fireDir.x /= len;
            fireDir.y /= len;

            GameObject* bullet = new GameObject(
                pOwner->pos.x,
                pOwner->pos.y,
                pOwner->pos.z
            );

            bullet->scale = { 0.03f, 0.03f, 1.0f };
            bullet->AddComponent(new MeshRenderer(bulletMesh, bulletMaterial));
            bullet->AddComponent(new PlayerBullet(fireDir));

            world->push_back(bullet);
        }


    }
};