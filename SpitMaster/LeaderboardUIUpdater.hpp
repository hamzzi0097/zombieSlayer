#pragma once
#include <cstdio>
#include <vector>
#include <utility>
#include "ObjectBase.hpp"
#include "TextUI.hpp"
#include "Leaderboard.hpp"

// [LeaderboardUIUpdater]
// GameOver 화면의 TOP 10 라벨(TextUI 10개)을 '참조만' 들고, Leaderboard 워커 스레드
// 상태에 따라 매 프레임 텍스트를 갱신한다. (ResultUIUpdater와 동일 패턴, Render 없음)
class LeaderboardUIUpdater : public Component {
public:
    LeaderboardUIUpdater(Leaderboard* board, std::vector<TextUI*> rows)
        : m_board(board), m_rows(std::move(rows)) {}

    void Start() override {}
    void Input() override {}
    void Render() override {}

    void Update(float dt) override {
        if (!m_board) return;
        const Leaderboard::Status st = m_board->GetStatus();

        if (st == Leaderboard::Status::Loading || st == Leaderboard::Status::Idle) {
            SetRow(0, "LOADING...");
            ClearRowsFrom(1);
            return;
        }
        if (st == Leaderboard::Status::Failed) {
            SetRow(0, "LEADERBOARD N/A");
            ClearRowsFrom(1);
            return;
        }
        // Done
        std::vector<Leaderboard::Entry> e = m_board->GetEntries();
        if (e.empty()) {
            SetRow(0, "NO SCORES YET");
            ClearRowsFrom(1);
            return;
        }
        for (size_t i = 0; i < m_rows.size(); ++i) {
            if (i < e.size()) {
                char buf[48];
                snprintf(buf, sizeof(buf), "%2d. %-10.10s %6d", (int)i + 1, e[i].name.c_str(), e[i].score);
                m_rows[i]->SetText(buf);
            } else {
                m_rows[i]->SetText("");
            }
        }
    }

private:
    void SetRow(size_t i, const char* text) {
        if (i < m_rows.size() && m_rows[i]) m_rows[i]->SetText(text);
    }
    void ClearRowsFrom(size_t start) {
        for (size_t i = start; i < m_rows.size(); ++i)
            if (m_rows[i]) m_rows[i]->SetText("");
    }

    Leaderboard*         m_board = nullptr;
    std::vector<TextUI*> m_rows;
};
