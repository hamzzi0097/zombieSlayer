#pragma once
#include "ObjectBase.hpp"
#include "MeshRenderer.hpp"
#include "Material.hpp"
#include "MeleeMonsterController.hpp"
#include "RangedMonsterController.hpp"
#include <functional>

enum class BombState
{
    Fuse,
    Exploding,
    Dead
};

class BombController : public Component
{
private:
    std::vector<GameObject*>* world;
    ColorMaterial* fallbackMaterial = nullptr;
    TextureMaterial* bombMaterial = nullptr;
    TextureMaterial* effectMaterial = nullptr;
    Mesh* effectMesh = nullptr;

    BombState state = BombState::Fuse;

    float fuseTime = 2.0f;
    float fuseTimer = 2.0f;

    float explosionDuration = 0.7f;
    float explosionTimer = 0.7f;
    float explosionRadius = 0.35f;

    float startScale = 0.06f;
    int damage = 5;

public:
    // 폭발 시점에 호출되는 콜백 (이펙트 연출용). GameLoop/BombSpawner가 배선.
    // BombController는 무엇이 연결됐는지(ScreenShakeEffect 등) 모른다 — 결합도 0.
    std::function<void()> onExplode;

    BombController(std::vector<GameObject*>* world, ShaderSet colorShader, ShaderSet textureShader,
        Texture* bombTexture = nullptr, Texture* effectTexture = nullptr, Mesh* effectMesh = nullptr)
    {
        this->world = world;
        this->effectMesh = effectMesh;

        if (bombTexture && effectTexture)
        {
            bombMaterial = new TextureMaterial(textureShader, bombTexture);
            effectMaterial = new TextureMaterial(textureShader, effectTexture);
        }
        else
        {
            fallbackMaterial = new ColorMaterial(colorShader, XMFLOAT4(1.0f, 0.8f, 0.1f, 1.0f));
        }
    }

    ~BombController()
    {
        delete fallbackMaterial;
        fallbackMaterial = nullptr;
        delete bombMaterial;
        bombMaterial = nullptr;
        delete effectMaterial;
        effectMaterial = nullptr;
    }

    Material* GetMaterial() const
    {
        if (bombMaterial) return bombMaterial;
        return fallbackMaterial;
    }

    void Start() override
    {
        startScale = pOwner->scale.x;
    }

    void Input() override {}

    void Update(float dt) override
    {
        if (state == BombState::Fuse)
        {
            fuseTimer -= dt;
            UpdateFuseBlink();

            if (fuseTimer <= 0.0f)
            {
                ApplyDamage();
                if (onExplode) onExplode();
                state = BombState::Exploding;
                explosionTimer = explosionDuration;
                SwitchToExplosionEffect();
            }
        }
        else if (state == BombState::Exploding)
        {
            explosionTimer -= dt;
            UpdateExplosionEffect();

            if (explosionTimer <= 0.0f)
            {
                state = BombState::Dead;
            }
        }
        else if (state == BombState::Dead)
        {
            pOwner->isObjDead = true;
        }
    }

    void Render() override {}

private:
    void SetVisualAlpha(float alpha)
    {
        if (bombMaterial && state == BombState::Fuse)
            bombMaterial->SetTint(XMFLOAT4(1.0f, 1.0f, 1.0f, alpha));

        if (effectMaterial && state == BombState::Exploding)
            effectMaterial->SetTint(XMFLOAT4(1.0f, 1.0f, 1.0f, alpha));

        if (fallbackMaterial)
        {
            if (state == BombState::Fuse)
                fallbackMaterial->SetColor(XMFLOAT4(1.0f, 0.8f, 0.1f, alpha));
            else
                fallbackMaterial->SetColor(XMFLOAT4(0.9f, 0.9f, 0.9f, alpha));
        }
    }

    void SwitchToExplosionEffect()
    {
        MeshRenderer* renderer = pOwner->GetComponent<MeshRenderer>();

        if (renderer && effectMaterial)
        {
            if (effectMesh)
                renderer->pMeshData = effectMesh;

            renderer->pMaterial = effectMaterial;
        }

        pOwner->scale = { explosionRadius * 0.35f, explosionRadius * 0.35f, 1.0f };
        SetVisualAlpha(0.9f);
    }

    void UpdateFuseBlink()
    {
        float elapsed = fuseTime - fuseTimer;
        float progress = elapsed / fuseTime;

        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        float blinkInterval = 0.35f - progress * 0.28f;
        if (blinkInterval < 0.07f) blinkInterval = 0.07f;

        int blinkStep = (int)(elapsed / blinkInterval);
        float alpha = (blinkStep % 2 == 0) ? 1.0f : 0.25f;

        SetVisualAlpha(alpha);
    }

    void UpdateExplosionEffect()
    {
        float progress = 1.0f - explosionTimer / explosionDuration;

        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        float smokeStartScale = explosionRadius * 0.35f;
        float scale = smokeStartScale + (explosionRadius - smokeStartScale) * progress;
        pOwner->scale = { scale, scale, 1.0f };

        float alpha = 0.9f * (1.0f - progress);
        SetVisualAlpha(alpha);
    }

    void ApplyDamage()
    {
        if (!world) return;

        float radiusSq = explosionRadius * explosionRadius;

        for (auto obj : *world)
        {
            if (!obj || obj == pOwner || obj->isObjDead) continue;

            float dx = obj->pos.x - pOwner->pos.x;
            float dy = obj->pos.y - pOwner->pos.y;
            float distSq = dx * dx + dy * dy;

            if (distSq > radiusSq) continue;

            if (MeleeMonsterController* melee = obj->GetComponent<MeleeMonsterController>())
                melee->getDamaged(damage);

            if (RangedMonsterController* ranged = obj->GetComponent<RangedMonsterController>())
                ranged->getDamaged(damage);
        }
    }
};
