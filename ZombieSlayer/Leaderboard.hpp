#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include "HttpClient.hpp"
#include "json.hpp"

// [Leaderboard]
// GameOver 시 점수를 Firebase Realtime DB에 업로드하고 TOP 10을 받아온다.
// 통신은 워커 스레드에서 수행하고, 메인 스레드는 GetStatus()/GetEntries()로만 접근한다.
class Leaderboard {
public:
    struct Entry { std::string name; int score; };
    enum class Status { Idle, Loading, Done, Failed };

    // TODO: 본인 Firebase Realtime DB 호스트로 교체 (스킴/끝슬래시 제외)
    //   예: "my-game-default-rtdb.firebaseio.com"
    static constexpr const wchar_t* HOST = L"YOUR-PROJECT-default-rtdb.firebaseio.com";

    ~Leaderboard() { Join(); }

    // GameOver 진입 시 호출. 이전 작업 정리 후 워커 스레드 시작.
    void Submit(int score) {
        Join();
        {
            std::lock_guard<std::mutex> lk(m_mtx);
            m_entries.clear();
        }
        m_status = Status::Loading;
        m_worker = std::thread([this, score]() { Worker(score); });
    }

    Status GetStatus() const { return m_status.load(); }

    // 결과 복사본 반환(스레드 안전).
    std::vector<Entry> GetEntries() {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_entries;
    }

    // 워커 스레드 정리. OnExit(GameOver)에서 호출.
    void Join() {
        if (m_worker.joinable()) m_worker.join();
        m_status = Status::Idle;
    }

private:
    std::thread         m_worker;
    std::mutex          m_mtx;
    std::atomic<Status> m_status{ Status::Idle };
    std::vector<Entry>  m_entries;

    void Worker(int score) {
        using nlohmann::json;
        bool ok = false;

        // 1) 현재 항목 수 → 다음 순번 이름 (player1, player2, ...)
        std::string countResp = HttpClient::Request(HOST,
            L"/leaderboard.json?shallow=true", L"GET", "", ok);
        int n = 0;
        if (ok && !countResp.empty() && countResp != "null") {
            try { json j = json::parse(countResp); n = (int)j.size(); }
            catch (...) { n = 0; }
        }
        std::string name = "player" + std::to_string(n + 1);

        // 2) 업로드 (POST → Firebase가 auto-key 발급)
        json body;
        body["name"] = name;
        body["score"] = score;
        HttpClient::Request(HOST, L"/leaderboard.json", L"POST", body.dump(), ok);

        // 3) TOP 10 조회 (orderBy="score"는 %22로 인코딩, limitToLast=10)
        std::string topResp = HttpClient::Request(HOST,
            L"/leaderboard.json?orderBy=%22score%22&limitToLast=10", L"GET", "", ok);

        std::vector<Entry> parsed;
        if (ok && !topResp.empty() && topResp != "null") {
            try {
                json j = json::parse(topResp);
                for (auto& kv : j.items()) {
                    Entry e;
                    e.name  = kv.value().value("name", std::string("?"));
                    e.score = kv.value().value("score", 0);
                    parsed.push_back(e);
                }
                std::sort(parsed.begin(), parsed.end(),
                    [](const Entry& a, const Entry& b) { return a.score > b.score; });
            } catch (...) { parsed.clear(); ok = false; }
        }

        {
            std::lock_guard<std::mutex> lk(m_mtx);
            m_entries = parsed;
        }
        m_status = ok ? Status::Done : Status::Failed;
    }
};
