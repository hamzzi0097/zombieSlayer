#pragma once
#include <cstdio>
#include <string>
#include "ObjectBase.hpp"
#include "TextUI.hpp"

// [StatsUI 컴포넌트]
// 화면 상단 좌측: 생존 시간 MM:SS / 우측: 킬 카운트.
// 렌더링은 공용 5x7 블록 폰트(TextUI)에 위임한다 — 게임 전체 폰트 일관성 유지.
//
// 내부에 TextUI 2개(시간/킬)를 소유하고, 매 프레임 현재 값을 문자열로 만들어
// SetText()로 갱신한다. TextUI는 내용이 바뀔 때만 메시를 재생성하므로
// 시간(초당 1회)·킬(드물게)만 갱신되어 비용이 거의 없다.
//
// 정렬: 시간은 좌측 고정(Left), 킬은 우측 고정(Right) → 자릿수가 바뀌어도
//       기준 끝이 흔들리지 않는다.
class StatsUI : public Component
{
public:
    StatsUI(ShaderSet shaders, const int* killCount, const float* playTime)
        : m_killCount(killCount), m_playTime(playTime)
    {
        m_timeLabel = new TextUI(shaders, "00:00", -0.95f, 0.85f, 0.08f,
                                 XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f), TextUI::Align::Left);
        m_killLabel = new TextUI(shaders, "KILL 0", 0.95f, 0.85f, 0.08f,
                                 XMFLOAT4(1.0f, 0.85f, 0.1f, 1.0f), TextUI::Align::Right);
    }

    ~StatsUI() {
        delete m_timeLabel;
        delete m_killLabel;
    }

    void Start() override {
        m_timeLabel->Start();
        m_killLabel->Start();
    }

    void Render() override {
        // 생존 시간 → "MM:SS"
        int totalSec = (m_playTime ? (int)*m_playTime : 0);
        int minutes  = totalSec / 60; if (minutes > 99) minutes = 99;
        int seconds  = totalSec % 60;
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", minutes, seconds);
        m_timeLabel->SetText(timeBuf);

        // 킬 카운트 → "KILL N"
        int kills = (m_killCount ? *m_killCount : 0); if (kills > 999) kills = 999;
        char killBuf[16];
        snprintf(killBuf, sizeof(killBuf), "KILL %d", kills);
        m_killLabel->SetText(killBuf);

        m_timeLabel->Render();
        m_killLabel->Render();
    }

    void Update(float dt) override {}
    void Input()          override {}

private:
    const int*   m_killCount = nullptr;
    const float* m_playTime  = nullptr;
    TextUI*      m_timeLabel = nullptr;
    TextUI*      m_killLabel = nullptr;
};
