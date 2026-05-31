#pragma once
#include "ObjectBase.hpp"
#include "MeshRenderer.hpp"
#include "Material.hpp"
#include "MeleeMonsterControl.hpp"
#include "RangedMonsterControl.hpp"

enum class BombState
{
    Fuse,
    Exploding,
    Dead
};

class Bomb : public Component
{
private:
    std::vector<GameObject*>* world;
    ColorMaterial* material;

    BombState state = BombState::Fuse;

    float fuseTime = 2.0f;
    float fuseTimer = 2.0f;

    float explosionDuration = 0.3f;
    float explosionTimer = 0.3f;
    float explosionRadius = 0.35f;

    float startScale = 0.06f;
    int damage = 5;

public:
    Bomb(std::vector<GameObject*>* world, ColorMaterial* material)
    {
        this->world = world;
        this->material = material;
    }

    ~Bomb()
    {
        delete material;
        material = nullptr;
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
                state = BombState::Exploding;
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
    void UpdateFuseBlink()
    {
        if (!material) return;

        float elapsed = fuseTime - fuseTimer;
        float progress = elapsed / fuseTime;

        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        float blinkInterval = 0.35f - progress * 0.28f;
        if (blinkInterval < 0.07f) blinkInterval = 0.07f;

        int blinkStep = (int)(elapsed / blinkInterval);
        float alpha = (blinkStep % 2 == 0) ? 1.0f : 0.25f;

        material->SetColor(XMFLOAT4(1.0f, 0.8f, 0.1f, alpha));
    }

    void UpdateExplosionEffect()
    {
        if (!material) return;

        float progress = 1.0f - explosionTimer / explosionDuration;

        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        float scale = startScale + explosionRadius * progress;
        pOwner->scale = { scale, scale, 1.0f };

        float alpha = 1.0f - progress;
        material->SetColor(XMFLOAT4(1.0f, 0.25f, 0.05f, alpha));
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

            if (MeleeMonsterControl* melee = obj->GetComponent<MeleeMonsterControl>())
                melee->getDamaged(damage);

            if (RangedMonsterControl* ranged = obj->GetComponent<RangedMonsterControl>())
                ranged->getDamaged(damage);
        }
    }
};