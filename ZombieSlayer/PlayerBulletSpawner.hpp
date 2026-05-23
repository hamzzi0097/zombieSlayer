#pragma once
#include "ObjectBase.hpp"
#include "MeshRenderer.hpp"
#include "PlayerBullet.hpp"

// 플레이어의 탄환 오브젝트 생성 컴포넌트
class PlayerBulletSpawner : public Component
{
    std::vector<GameObject*>* pendingObjects;   // GameLoop update 전에 넣어놓을 임시 vector
    Mesh* bulletMesh;
    Material* bulletMaterial;

    HWND hWnd;
    int* windowWidth;
    int* windowHeight;

    bool wasLeftMouseDown;

public:
    PlayerBulletSpawner(std::vector<GameObject*>* pendingObjects, Mesh* bulletMesh,
        Material* bulletMaterial, HWND hWnd, int* width, int* height) : Component()
    {
        this->pendingObjects = pendingObjects;
        this->bulletMesh = bulletMesh;
        this->bulletMaterial = bulletMaterial;
        this->hWnd = hWnd;
        this->windowWidth = width;
        this->windowHeight = height;

        wasLeftMouseDown = false;
    }

    void Start() override
    {
    }

    void Input() override
    {
        // 마우스를 누르고 있는 매 프레임 발사 방지
        bool isLeftMouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        // 현재 프레임 처음 눌린 순간만 탄환 생성
        if (isLeftMouseDown && !wasLeftMouseDown)
        {
            SpawnBullet();
        }

        wasLeftMouseDown = isLeftMouseDown;
    }

    void Update(float dt) override
    {
    }

    void Render() override
    {
    }

private:
    XMFLOAT2 GetMousePosition()
    {
        // 마우스 화면 좌표 -> 현재 게임 창 기준 좌표로 변환
        POINT mousePos;
        GetCursorPos(&mousePos);
        ScreenToClient(hWnd, &mousePos);

        // 게임 월드 좌표를 (-1 ~ 1)로 변환
        XMFLOAT2 mouseWorldPos;
        mouseWorldPos.x = ((float)mousePos.x / (float)(*windowWidth)) * 2.0f - 1.0f;
        mouseWorldPos.y = 1.0f - ((float)mousePos.y / (float)(*windowHeight)) * 2.0f;

        return mouseWorldPos;
    }

    void SpawnBullet()
    {
        // 현재 마우스 월드 좌표를 기준으로 발사 방향 계산.
        XMFLOAT2 mouseWorldPos = GetMousePosition();

        XMFLOAT2 fireDir = { mouseWorldPos.x - pOwner->pos.x, mouseWorldPos.y - pOwner->pos.y };

        // 방향 벡터 정규화 => 탄환 속도를 일정하게 유지
        float len = sqrtf(fireDir.x * fireDir.x + fireDir.y * fireDir.y);

        if (len > 0.0f)
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

            // world 순회 중 직접 추가하지 않고, 다음 Update에서 추가되도록 예약
            pendingObjects->push_back(bullet);
        }

    }
};