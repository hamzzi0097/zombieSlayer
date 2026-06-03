#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <algorithm>
#include "HttpClient.hpp"
#include "MiniJson.hpp"

// [Leaderboard]
// GameOver 시 점수를 Firebase Realtime DB에 업로드하고 TOP 10을 받아온다.
// 통신은 detach된 워커 스레드에서 수행하고, 메인 스레드는 절대 블로킹되지 않는다.
// 워커가 결과를 쓰는 저장소(SharedState)를 shared_ptr로 함께 소유하므로,
// 메인이 새 작업을 시작하거나 종료해도 옛 워커는 옛 저장소에만 안전하게 접근한다.
class Leaderboard {
public:
    struct Entry { std::string name; int score; };
    enum class Status { Idle, Loading, Done, Failed };

    // TODO: 본인 Firebase Realtime DB 호스트로 교체 (스킴/끝슬래시 제외)
    //   예: "my-game-default-rtdb.firebaseio.com"
    static constexpr const wchar_t* HOST = L"spit-master-default-rtdb.asia-southeast1.firebasedatabase.app";

    // 워커와 메인이 공유하는 상태. shared_ptr로 수명을 함께 관리한다.
    struct SharedState {
        std::mutex          mtx;
        std::atomic<Status> status{ Status::Idle };
        std::vector<Entry>  entries;
    };

    Leaderboard() : m_state(std::make_shared<SharedState>()) {}

    // GameOver 진입 시 호출. 블로킹 없이 새 워커를 시작한다.
    void Submit(int score) {
        // 새 저장소 생성 후 교체. (옛 워커가 아직 돌아도 옛 저장소에만 쓰므로 안전)
        auto state = std::make_shared<SharedState>();
        state->status = Status::Loading;
        m_state = state;

        // 워커가 state를 함께 소유 → 메인이 무엇을 하든 자기 저장소는 살아있다.
        std::thread([state, score]() { Worker(state, score); }).detach();
    }

    Status GetStatus() const { return m_state->status.load(); }

    // 결과 복사본 반환(스레드 안전).
    std::vector<Entry> GetEntries() const {
        std::lock_guard<std::mutex> lk(m_state->mtx);
        return m_state->entries;
    }

    // 더 이상 블로킹 정리가 필요 없다. 호출부 호환을 위해 no-op로 남겨둔다.
    void Join() {}

private:
    std::shared_ptr<SharedState> m_state;

    // 워커는 자신이 소유한 state에만 접근한다(this 캡처 없음).
    static void Worker(std::shared_ptr<SharedState> state, int score) {
        bool ok = false;

        // 1) 현재 항목 수 → 다음 순번 이름 (player1, player2, ...)
        std::string countResp = HttpClient::Request(HOST,
            L"/leaderboard.json?shallow=true", L"GET", "", ok);
        int n = 0;
        if (ok && !countResp.empty() && countResp != "null") {
            n = MiniJson::Parser(countResp).CountObjectMembers();
        }
        std::string name = "player" + std::to_string(n + 1);

        // 2) 업로드 (POST → Firebase가 auto-key 발급)
        std::string body = "{\"name\":\"" + MiniJson::Escape(name)
                         + "\",\"score\":" + std::to_string(score) + "}";
        HttpClient::Request(HOST, L"/leaderboard.json", L"POST", body, ok);

        // 3) TOP 10 조회 (orderBy="score"는 %22로 인코딩, limitToLast=10)
        std::string topResp = HttpClient::Request(HOST,
            L"/leaderboard.json?orderBy=%22score%22&limitToLast=10", L"GET", "", ok);

        std::vector<Entry> parsed;
        if (ok && !topResp.empty() && topResp != "null") {
            MiniJson::Parser(topResp).ForEachEntry([&](const std::string& nm, int sc) {
                parsed.push_back(Entry{ nm.empty() ? "?" : nm, sc });
            });
            std::sort(parsed.begin(), parsed.end(),
                [](const Entry& a, const Entry& b) { return a.score > b.score; });
        }

        {
            std::lock_guard<std::mutex> lk(state->mtx);
            state->entries = parsed;
        }
        state->status = ok ? Status::Done : Status::Failed;
    }
};
