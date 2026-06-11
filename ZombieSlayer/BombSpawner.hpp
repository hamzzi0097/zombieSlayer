#pragma once
#include "ObjectBase.hpp"
#include "MeshRenderer.hpp"
#include "BombController.hpp"
#include "Logger.hpp"
#include "ScreenShakeEffect.hpp"
#include "AudioSource.hpp"

class BombSpawner : public Component
{
private:
    std::vector<GameObject*>* world;
    std::vector<GameObject*>* pendingObjects;

    Mesh* bombMesh;
    Mesh* effectMesh;
    ShaderSet colorShader;
    ShaderSet textureShader;
    Texture* bombTexture;
    Texture* effectTexture;
    ScreenShakeEffect* screenShake = nullptr;   // 참조 (소유 X) — 폭발 흔들림 배선용
    AudioSource* deploymentAudioSource = nullptr; // 참조 (소유 X)
    AudioSystem* audioSystem = nullptr;            // 참조 (소유 X)
    const SoundClip* explosionSound = nullptr;     // 참조 (소유 X)

    bool wasRightMouseDown = false;
    float cooldown = 1.0f;
    float cooldownTimer = 0.0f;

public:
    BombSpawner(std::vector<GameObject*>* world, std::vector<GameObject*>* pendingObjects,
        Mesh* bombMesh, ShaderSet colorShader, ShaderSet textureShader,
        Texture* bombTexture = nullptr, Texture* effectTexture = nullptr, Mesh* effectMesh = nullptr,
        ScreenShakeEffect* screenShake = nullptr,
        AudioSource* deploymentAudioSource = nullptr,
        AudioSystem* audioSystem = nullptr,
        const SoundClip* explosionSound = nullptr)
    {
        this->world = world;
        this->pendingObjects = pendingObjects;
        this->bombMesh = bombMesh;
        this->effectMesh = effectMesh ? effectMesh : bombMesh;
        this->colorShader = colorShader;
        this->textureShader = textureShader;
        this->bombTexture = bombTexture;
        this->effectTexture = effectTexture;
        this->screenShake = screenShake;
        this->deploymentAudioSource = deploymentAudioSource;
        this->audioSystem = audioSystem;
        this->explosionSound = explosionSound;
    }

    void Start() override {}

    // 폭탄 쿨다운 진행도. 0 = 방금 사용, 1 = 사용 준비 완료.
    // UI(BombCooldownUI)가 매 프레임 읽어 파이 게이지를 채움.
    float GetCooldownRatio() const
    {
        if (cooldownTimer <= 0.0f) return 1.0f;
        return 1.0f - (cooldownTimer / cooldown);
    }

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
        GameObject* bomb = new GameObject(
            pOwner->pos.x,
            pOwner->pos.y,
            pOwner->pos.z
        );

        BombController* bombComponent = new BombController(
            world, colorShader, textureShader,
            bombTexture, effectTexture, effectMesh
        );

        AudioSource* explosionAudioSource = nullptr;
        if (audioSystem && explosionSound)
        {
            explosionAudioSource =
                new AudioSource(audioSystem, explosionSound, 0.8f);
            bomb->AddComponent(explosionAudioSource);
        }

        // BombController는 연출 구현을 모르고 폭발 이벤트만 발생시킨다.
        ScreenShakeEffect* shake = screenShake;
        bombComponent->onExplode = [shake, explosionAudioSource]() {
            if (shake)
                shake->Trigger(0.25f, 0.04f);
            if (explosionAudioSource)
                explosionAudioSource->PlayOneShot();
        };
        float bombScale = bombTexture ? 0.08f : 0.06f;
        bomb->scale = { bombScale, bombScale, 1.0f };
        bomb->AddComponent(new MeshRenderer(bombMesh, bombComponent->GetMaterial()));
        bomb->AddComponent(bombComponent);

        pendingObjects->push_back(bomb);

        if (deploymentAudioSource)
            deploymentAudioSource->PlayOneShot();

        LOG_INFO("Bomb placed.");
    }
};
