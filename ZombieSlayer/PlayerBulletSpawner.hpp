#pragma once
#include "ObjectBase.hpp"
#include "MeshRenderer.hpp"
#include "PlayerBullet.hpp"
#include "Collider.hpp"
#include "Logger.hpp"

// 플레이어의 탄환 오브젝트 생성 컴포넌트
class PlayerBulletSpawner : public Component
{
    std::vector<GameObject*>* pendingObjects;   // GameLoop update 전에 넣어놓을 임시 vector
    Mesh* bulletMesh;
    Material* bulletMaterial;

    HWND hWnd;
    int* windowWidth;
    int* windowHeight;

    float bulletScale;
    float bulletColliderRadius;
    float bulletRotationOffset;

    int maxAmmo = 10;
    int currentAmmo = 10;

    bool isReloading = false;
    float reloadTime = 1.5f;
    float reloadTimer = 0.0f;

    bool wasLeftMouseDown;

public:
    PlayerBulletSpawner(std::vector<GameObject*>* pendingObjects, Mesh* bulletMesh,
        Material* bulletMaterial, HWND hWnd, int* width, int* height,
        float bulletScale = 0.03f, float bulletColliderRadius = 1.0f,
        float bulletRotationOffset = 0.0f) : Component()
    {
        this->pendingObjects = pendingObjects;
        this->bulletMesh = bulletMesh;
        this->bulletMaterial = bulletMaterial;
        this->hWnd = hWnd;
        this->windowWidth = width;
        this->windowHeight = height;
        this->bulletScale = bulletScale;
        this->bulletColliderRadius = bulletColliderRadius;
        this->bulletRotationOffset = bulletRotationOffset;

        wasLeftMouseDown = false;
    }

    void Start() override
    {
        maxAmmo = 10;
        currentAmmo = maxAmmo;

        isReloading = false;
        reloadTime = 1.5f;
        reloadTimer = 0.0f;
    }

    void Input() override
    {
        // 마우스를 누르고 있는 매 프레임 발사 방지
        bool isLeftMouseDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        // 현재 프레임 처음 눌린 순간만 탄환 생성
        if (isLeftMouseDown && !wasLeftMouseDown)
        {
            if (!isReloading && currentAmmo > 0)
            {
                SpawnBullet();
                currentAmmo--;
                LOG_INFO("Ammo: %d / %d", currentAmmo, maxAmmo);

                if (currentAmmo == 0)
                {
                    isReloading = true;
                    reloadTimer = reloadTime;
                    LOG_INFO("Reloading... Ammo: %d / %d", currentAmmo, maxAmmo);
                }
            }
        }

        wasLeftMouseDown = isLeftMouseDown;

        if ((GetAsyncKeyState('R') & 0x0001) != 0)
        {
            if (!isReloading && currentAmmo < maxAmmo)
            {
                isReloading = true;
                reloadTimer = reloadTime;
                LOG_INFO("Reloading... Ammo: %d / %d", currentAmmo, maxAmmo);
            }
        }
        
    }

    void Update(float dt) override
    {
        if (isReloading)
        {
            reloadTimer -= dt;

            if (reloadTimer <= 0.0f)
            {
                currentAmmo = maxAmmo;
                isReloading = false;
                reloadTimer = 0.0f;
                LOG_INFO("Reload complete. Ammo: %d / %d", currentAmmo, maxAmmo);
            }
        }
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

        // 화면 비율 반영 월드 좌표로 보정
        XMFLOAT2 mouseWorldPos;
        float aspect = (float)(*windowWidth) / (float)(*windowHeight);

        mouseWorldPos.x = ((float)mousePos.x / (float)(*windowWidth)) * 2.0f - 1.0f;
        mouseWorldPos.y = 1.0f - ((float)mousePos.y / (float)(*windowHeight)) * 2.0f;

        if (aspect >= 1.0f) mouseWorldPos.x *= aspect;
        else mouseWorldPos.y /= aspect;


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

            bullet->scale = { bulletScale, bulletScale, 1.0f };
            bullet->rot.z = atan2f(fireDir.y, fireDir.x) + bulletRotationOffset;
            bullet->AddComponent(new MeshRenderer(bulletMesh, bulletMaterial));
            bullet->AddComponent(new PlayerBullet(fireDir));

            // 탄환 레이어 Collider
            bullet->AddComponent(new CircleCollider(bulletColliderRadius, CollisionLayer::PlayerBullet));

            // world 순회 중 직접 추가하지 않고, 다음 Update에서 추가되도록 예약
            pendingObjects->push_back(bullet);
        }

    }
};
