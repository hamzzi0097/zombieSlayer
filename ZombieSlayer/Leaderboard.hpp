#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <chrono>
#include <algorithm>
#include "HttpClient.hpp"
#include "MiniJson.hpp"

// [Leaderboard]
// GameOver 시 점수를 Firebase Realtime DB에 업로드하고 TOP 10을 받아온다.
// 통신은 detach된 워커 스레드에서 수행하고, 메인 스레드는 절대 블로킹되지 않는다.
//  - 워커가 결과 저장소(SharedState)를 shared_ptr로 함께 소유 → 라운드가 바뀌어도 안전.
//  - 진행 중 워커 수(Registry)를 추적 → 프로그램 종료 시점에만 '짧게 한정' 대기해서
//    통신 도중 프로세스가 정리되며 발생할 수 있는 위험을 없앤다(게임 중 멈춤은 없음).
class Leaderboard {
public:
    struct Entry { std::string name; int score; };
    enum class Status { Idle, Loading, Done, Failed };

    // TODO: 본인 Firebase Realtime DB 호스트로 교체 (스킴/끝슬래시 제외)
    //   예: "my-game-default-rtdb.firebaseio.com"
    static constexpr const wchar_t* HOST = L"spit-master-default-rtdb.asia-southeast1.firebasedatabase.app";

    // 종료 시 최대 대기 시간(초). 보통은 inFlight==0이라 즉시 반환하고,
    // 게임오버 직후 네트워크가 느린 상태에서 종료할 때만 이 시간만큼만 기다린다.
    static constexpr int SHUTDOWN_DRAIN_SECONDS = 6;

    // 워커와 메인이 공유하는 결과 저장소. shared_ptr로 수명을 함께 관리한다.
    struct SharedState {
        std::mutex          mtx;
        std::atomic<Status> status{ Status::Idle };
        std::vector<Entry>  entries;
    };

    // 진행 중인 워커 수 추적용. shared_ptr이라 Leaderboard가 먼저 파괴돼도
    // 살아있는 워커가 함께 소유하는 동안 유효하다.
    struct Registry {
        std::mutex              mtx;
        std::condition_variable cv;
        int                     inFlight = 0;
    };

    Leaderboard()
        : m_state(std::make_shared<SharedState>())
        , m_registry(std::make_shared<Registry>()) {}

    // 종료 시점에만 호출됨. 진행 중 워커가 끝날 때까지 한정 대기(없으면 즉시 반환).
    ~Leaderboard() {
        std::unique_lock<std::mutex> lk(m_registry->mtx);
        m_registry->cv.wait_for(lk, std::chrono::seconds(SHUTDOWN_DRAIN_SECONDS),
            [this] { return m_registry->inFlight == 0; });
        // 시간 내 못 끝낸 워커가 있더라도 detach된 상태이고 자기 자원만 만지므로
        // 그대로 두고 진행한다(OS가 프로세스 종료 시 정리).
    }

    // GameOver 진입 시 호출. 블로킹 없이 새 워커를 시작한다.
    void Submit(int score) {
        // 새 저장소 생성 후 교체. (옛 워커가 아직 돌아도 옛 저장소에만 쓰므로 안전)
        auto state = std::make_shared<SharedState>();
        state->status = Status::Loading;
        m_state = state;

        auto registry = m_registry;
        {
            std::lock_guard<std::mutex> lk(registry->mtx);
            registry->inFlight++;
        }

        // 워커가 state·registry를 함께 소유 → 메인이 무엇을 하든 안전하게 동작/정리.
        std::thread([state, registry, score]() {
            Worker(state, score);
            std::lock_guard<std::mutex> lk(registry->mtx);
            registry->inFlight--;
            registry->cv.notify_all();
        }).detach();
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
    std::shared_ptr<Registry>    m_registry;

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
