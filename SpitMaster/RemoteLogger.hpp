#pragma once
#include <windows.h>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <ctime>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "HttpClient.hpp"
#include "MiniJson.hpp"

// Firebase Realtime DB에 중요한 로그를 비동기로 저장한다.
// Logger는 콘솔 출력만 즉시 수행하고, 네트워크 전송은 이 큐 워커가 처리한다.
class RemoteLogger {
public:
    static constexpr const wchar_t* HOST =
        L"spit-master-default-rtdb.asia-southeast1.firebasedatabase.app";

    static RemoteLogger& Get()
    {
        static RemoteLogger instance;
        return instance;
    }

    RemoteLogger(const RemoteLogger&) = delete;
    RemoteLogger& operator=(const RemoteLogger&) = delete;

    void Submit(const char* level, const char* file, const char* func, const char* message)
    {
        auto state = m_state;
        if (!state) return;

        LogEvent e;
        e.level = level ? level : "";
        e.file = file ? file : "";
        e.func = func ? func : "";
        e.message = message ? message : "";
        e.timestampMs = NowMs();

        {
            std::lock_guard<std::mutex> lk(state->mtx);
            if (state->stopping) return;

            if (state->queue.size() >= MAX_QUEUE_SIZE)
                state->queue.pop_front();

            state->queue.push_back(std::move(e));
        }

        state->cv.notify_one();
    }

    const std::string& GetSessionId() const { return m_sessionId; }

private:
    struct LogEvent {
        std::string level;
        std::string file;
        std::string func;
        std::string message;
        long long timestampMs = 0;
    };

    struct SharedState {
        std::mutex mtx;
        std::condition_variable cv;
        std::deque<LogEvent> queue;
        bool stopping = false;
    };

    static constexpr size_t MAX_QUEUE_SIZE = 128;

    std::shared_ptr<SharedState> m_state;
    std::string m_sessionId;

    RemoteLogger()
        : m_state(std::make_shared<SharedState>())
        , m_sessionId(BuildSessionId())
    {
        auto state = m_state;
        std::string sessionId = m_sessionId;
        std::thread([state, sessionId]() {
            WorkerLoop(state, sessionId);
        }).detach();
    }

    ~RemoteLogger()
    {
        auto state = m_state;
        if (!state) return;

        {
            std::lock_guard<std::mutex> lk(state->mtx);
            state->stopping = true;
        }
        state->cv.notify_one();
    }

    static long long NowMs()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    static std::string BuildSessionId()
    {
        time_t now = time(nullptr);
        tm t;
        localtime_s(&t, &now);

        char buf[64];
        snprintf(buf, sizeof(buf), "%04d%02d%02d-%02d%02d%02d-%lu",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec,
            (unsigned long)GetCurrentProcessId());
        return buf;
    }

    static std::wstring ToWideAscii(const std::string& s)
    {
        return std::wstring(s.begin(), s.end());
    }

    static std::string BuildBody(const LogEvent& e, const std::string& sessionId)
    {
        std::string body;
        body.reserve(e.message.size() + 256);
        body += "{";
        body += "\"sessionId\":\"" + MiniJson::Escape(sessionId) + "\",";
        body += "\"timestampMs\":" + std::to_string(e.timestampMs) + ",";
        body += "\"level\":\"" + MiniJson::Escape(e.level) + "\",";
        body += "\"file\":\"" + MiniJson::Escape(e.file) + "\",";
        body += "\"func\":\"" + MiniJson::Escape(e.func) + "\",";
        body += "\"message\":\"" + MiniJson::Escape(e.message) + "\"";
        body += "}";
        return body;
    }

    static void WorkerLoop(std::shared_ptr<SharedState> state, const std::string& sessionId)
    {
        const std::wstring path = L"/logs/" + ToWideAscii(sessionId) + L".json";

        for (;;) {
            LogEvent e;
            {
                std::unique_lock<std::mutex> lk(state->mtx);
                state->cv.wait(lk, [&]() {
                    return state->stopping || !state->queue.empty();
                });

                if (state->queue.empty()) {
                    if (state->stopping) break;
                    continue;
                }

                e = std::move(state->queue.front());
                state->queue.pop_front();
            }

            bool ok = false;
            HttpClient::Request(HOST, path, L"POST", BuildBody(e, sessionId), ok);
        }
    }
};
