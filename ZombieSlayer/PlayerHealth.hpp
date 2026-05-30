#pragma once
#include "ObjectBase.hpp"
#include "Logger.hpp"

class PlayerHealth : public Component
{
private:
    int maxLives;
    int currentLives;

    float invincibleRemainTime;
    float invincibleDuration;

public:
    PlayerHealth(int lives = 3, float invincibleDuration = 1.0f)
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
    }

    void Input() override
    {
    }

    void Update(float dt) override
    {
        if (invincibleRemainTime > 0.0f)
        {
            invincibleRemainTime -= dt;

            if (invincibleRemainTime < 0.0f)
                invincibleRemainTime = 0.0f;
        }
    }

    void Render() override
    {
    }
};