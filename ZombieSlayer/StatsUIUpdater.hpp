#pragma once
#include <cstdio>
#include "ObjectBase.hpp"
#include "TextUI.hpp"

// [StatsUIUpdater]
// Playing 상단의 시간/킬 라벨(TextUI)을 '참조만' 들고, 매 프레임 값만 갱신한다.
// 렌더링은 TextUI 자신이(엔진이 Render 호출) 수행 → 컨트롤러는 SetText만, Render 없음.
//
// 유니티의 "scoreText.text = score" 패턴. 라벨과 컨트롤러는 같은 GameObject에
// 함께 부착되어 수명이 동일하므로 참조 포인터가 안전하다.
class StatsUIUpdater : public Component {
public:
    StatsUIUpdater(const int* killCount, const float* playTime,
                   TextUI* timeLabel, TextUI* killLabel)
        : m_killCount(killCount), m_playTime(playTime),
          m_timeLabel(timeLabel), m_killLabel(killLabel) {}

    void Start() override {}
    void Input() override {}

    void Update(float dt) override {
        int totalSec = (m_playTime ? (int)*m_playTime : 0);
        int minutes  = totalSec / 60; if (minutes > 99) minutes = 99;
        int seconds  = totalSec % 60;
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", minutes, seconds);
        if (m_timeLabel) m_timeLabel->SetText(timeBuf);

        int kills = (m_killCount ? *m_killCount : 0); if (kills > 999) kills = 999;
        char killBuf[16];
        snprintf(killBuf, sizeof(killBuf), "KILL %d", kills);
        if (m_killLabel) m_killLabel->SetText(killBuf);
    }

    void Render() override {}

private:
    const int*   m_killCount = nullptr;
    const float* m_playTime  = nullptr;
    TextUI*      m_timeLabel  = nullptr; // 참조 (소유 X)
    TextUI*      m_killLabel  = nullptr; // 참조 (소유 X)
};
