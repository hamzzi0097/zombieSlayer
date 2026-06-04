#pragma once
#include "ObjectBase.hpp"
#include <cstdlib>

// [ScreenShakeEffect 컴포넌트]
// 화면 흔들림 offset을 계산해 제공한다(직접 렌더하지 않음 — 변환형 이펙트).
// screenShakeObject에 부착되어 매 프레임 Update로 offset을 갱신하고,
// GameLoop::Render가 GetPixelOffset()을 읽어 뷰포트를 통째로 이동시킨다.
//
// Trigger는 BombSpawner가 콜백으로 배선해 인스턴스로 호출하고, GetPixelOffset은
// GameLoop이 핸들로 호출한다. (static 싱글톤 제거 — 프로젝트의 DI/콜백 패턴과 일관)
class ScreenShakeEffect : public Component
{
private:
    float duration = 0.0f;
    float timer = 0.0f;
    float power = 0.0f;

    XMFLOAT2 offset = { 0.0f, 0.0f };

public:
    ScreenShakeEffect() {}

    void Trigger(float shakeDuration = 0.25f, float shakePower = 0.04f)
    {
        duration = shakeDuration;
        timer = shakeDuration;
        power = shakePower;
    }

    // 라운드 시작 시 외부에서 호출. 이전 라운드에서 남은 흔들림 상태를 초기화한다.
    // (영속 객체라 Start()가 재실행되지 않으므로 별도 리셋이 필요)
    void Reset()
    {
        duration = 0.0f;
        timer    = 0.0f;
        power    = 0.0f;
        offset   = { 0.0f, 0.0f };
    }

    XMFLOAT2 GetOffset() const { return offset; }

    XMFLOAT2 GetPixelOffset(float width, float height) const
    {
        return {
            offset.x * width * 0.5f,
            offset.y * height * 0.5f
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
