#pragma once
#include <cstdio>
#include "ObjectBase.hpp"
#include "TextUI.hpp"

// [ResultUI 컴포넌트]
// GameOver 화면 중앙에 최종 성적(생존 시간 / 킬 수)을 표시.
// gameOverUI 캔버스에 부착. StatsUI와 동일하게 GameLoop의 m_killCount/m_playTime
// 주소를 받아 매 프레임 읽는다(GameOver 동안 값은 고정되어 있음).
//
// 내부에 TextUI 2개(시간/킬, 모두 가운데 정렬)를 소유하고 SetText로 갱신한다.
class ResultUI : public Component
{
public:
    ResultUI(ShaderSet shaders, const int* killCount, const float* playTime)
        : m_killCount(killCount), m_playTime(playTime)
    {
        // GAME OVER(위) 아래에 시간·킬을 세로로 배치
        m_timeLabel = new TextUI(shaders, "TIME 00:00", 0.0f,  0.02f, 0.10f,
                                 XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f), TextUI::Align::Center);
        m_killLabel = new TextUI(shaders, "KILLS 0",    0.0f, -0.16f, 0.10f,
                                 XMFLOAT4(1.0f, 0.85f, 0.1f, 1.0f), TextUI::Align::Center);
    }

    ~ResultUI() {
        delete m_timeLabel;
        delete m_killLabel;
    }

    void Start() override {
        m_timeLabel->Start();
        m_killLabel->Start();
    }

    void Render() override {
        int totalSec = (m_playTime ? (int)*m_playTime : 0);
        int minutes  = totalSec / 60; if (minutes > 99) minutes = 99;
        int seconds  = totalSec % 60;
        char timeBuf[24];
        snprintf(timeBuf, sizeof(timeBuf), "TIME %02d:%02d", minutes, seconds);
        m_timeLabel->SetText(timeBuf);

        int kills = (m_killCount ? *m_killCount : 0); if (kills > 999) kills = 999;
        char killBuf[24];
        snprintf(killBuf, sizeof(killBuf), "KILLS %d", kills);
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
