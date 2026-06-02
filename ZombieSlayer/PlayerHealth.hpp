#pragma once
#include "ObjectBase.hpp"
#include "Logger.hpp"
#include "Material.hpp"
#include "MeshRenderer.hpp"
#include <functional>

class PlayerHealth : public Component
{
private:
    int maxLives;
    int currentLives;

    float invincibleRemainTime;
    float invincibleDuration;

    ColorMaterial* playerMaterial = nullptr;
    TextureMaterial* playerTextureMaterial = nullptr;

    XMFLOAT4 normalColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    XMFLOAT4 blinkColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);

    int blinkCount = 3;

    void SetVisualColor(const XMFLOAT4& color)
    {
        if (playerMaterial)
            playerMaterial->SetColor(color);

        if (playerTextureMaterial)
            playerTextureMaterial->SetTint(color);
    }

public:
    std::function<void()> onDamaged;

    PlayerHealth(int lives = 3, float invincibleDuration = 1.5f)
    {
        this->maxLives = lives;
        this->currentLives = lives;
        this->invincibleRemainTime = 0.0f;
        this->invincibleDuration = invincibleDuration;
    }

    void TakeDamage()
    {
        if (pOwner->isObjDead) return;
        if (IsInvincible()) return;

        currentLives -= 1;
        if (onDamaged) onDamaged();

        LOG_INFO("Player damaged. Lives: %d / %d", currentLives, maxLives);

        if (currentLives <= 0)
        {
            currentLives = 0;
            pOwner->isObjDead = true;
            LOG_INFO("Player dead.");
            return;
        }

        invincibleRemainTime = invincibleDuration;
    }

    bool IsInvincible() const
    {
        return invincibleRemainTime > 0.0f;
    }

    int GetLives() const
    {
        return currentLives;
    }

    int GetMaxLives() const
    {
        return maxLives;
    }

    void Start() override
    {
        MeshRenderer* renderer = pOwner->GetComponent<MeshRenderer>();

        if (renderer)
        {
            playerMaterial = dynamic_cast<ColorMaterial*>(renderer->GetMaterial());
            playerTextureMaterial = dynamic_cast<TextureMaterial*>(renderer->GetMaterial());

            if (playerMaterial)
            {
                normalColor = playerMaterial->color;
            }
            else if (playerTextureMaterial)
            {
                normalColor = playerTextureMaterial->tint;
            }

            if (playerMaterial || playerTextureMaterial)
            {
                blinkColor = normalColor;
                blinkColor.w = 0.5f;
            }
        }
    }

    void Input() override
    {
    }

    void Update(float dt) override
    {
        if (invincibleRemainTime > 0.0f)
        {
            invincibleRemainTime -= dt;

            if (playerMaterial || playerTextureMaterial)
            {
                float elapsed = invincibleDuration - invincibleRemainTime;

                float blinkInterval = invincibleDuration / (blinkCount * 2.0f);
                int blinkStep = (int)(elapsed / blinkInterval);

                if (blinkStep % 2 == 0)
                    SetVisualColor(blinkColor);
                else
                    SetVisualColor(normalColor);
            }

            if (invincibleRemainTime <= 0.0f)
            {
                invincibleRemainTime = 0.0f;

                SetVisualColor(normalColor);
            }
        }
    }

    void Render() override
    {
    }
};
