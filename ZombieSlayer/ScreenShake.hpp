#pragma once
#include "ObjectBase.hpp"
#include <cstdlib>

class ScreenShake : public Component
{
private:
    static ScreenShake* s_screenShake;

    float duration = 0.0f;
    float timer = 0.0f;
    float power = 0.0f;

    XMFLOAT2 offset = { 0.0f, 0.0f };

public:
    ScreenShake()
    {
        s_screenShake = this;
    }

    ~ScreenShake()
    {
        if (s_screenShake == this)
            s_screenShake = nullptr;
    }

    static void Trigger(float shakeDuration = 0.25f, float shakePower = 0.04f)
    {
        if (!s_screenShake) return;

        s_screenShake->duration = shakeDuration;
        s_screenShake->timer = shakeDuration;
        s_screenShake->power = shakePower;
    }

    static XMFLOAT2 GetOffset()
    {
        if (!s_screenShake) return { 0.0f, 0.0f };
        return s_screenShake->offset;
    }

    static XMFLOAT2 GetPixelOffset(float width, float height)
    {
        XMFLOAT2 shakeOffset = GetOffset();
        return {
            shakeOffset.x * width * 0.5f,
            shakeOffset.y * height * 0.5f
        };
    }

    void Start() override {}

    void Input() override {}

    void Update(float dt) override
    {
        if (timer <= 0.0f)
        {
            offset = { 0.0f, 0.0f };
            return;
        }

        timer -= dt;

        float progress = duration > 0.0f ? timer / duration : 0.0f;
        float currentPower = power * progress;

        float randomX = ((rand() % 200) / 100.0f - 1.0f) * currentPower;
        float randomY = ((rand() % 200) / 100.0f - 1.0f) * currentPower;

        offset = { randomX, randomY };

        if (timer <= 0.0f)
            offset = { 0.0f, 0.0f };
    }

    void Render() override {}
};
