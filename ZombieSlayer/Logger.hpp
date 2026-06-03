#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <fstream>
#include <string>
#include "RemoteLogger.hpp"

class Logger {
public:
    enum class Level { Debug = 0, Info, Warn, Error };

    static Logger& Get() {
        static Logger instance;
        return instance;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void SetMinLevel(Level lv) { m_minLevel = lv; }
    void SetRemoteEnabled(bool enabled) { m_remoteEnabled = enabled; }
    void SetRemoteMinLevel(Level lv) { m_remoteMinLevel = lv; }

    // file, func는 매크로가 __FILE__ / __FUNCTION__으로 자동 주입
    void Log(Level lv, const char* file, const char* func, const char* fmt, ...) {
        if (lv < m_minLevel) return;

        char msgBuf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
        va_end(args);

        char timeBuf[32];
        time_t now = time(nullptr);
        tm t;
        localtime_s(&t, &now);
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);

        // __FILE__ 경로에서 파일명만 추출
        const char* fileName = file;
        for (const char* p = file; *p; ++p)
            if (*p == '\\' || *p == '/') fileName = p + 1;

        char line[1200];
        snprintf(line, sizeof(line), "[%s] %s [%s | %s] %s\n",
            timeBuf, TagOf(lv), fileName, func, msgBuf);

        SetConsoleColor(lv);
        fputs(line, stdout);
        ResetConsoleColor();

        if (m_file.is_open())
            m_file << line << std::flush;

        if (m_remoteEnabled && lv >= m_remoteMinLevel)
            RemoteLogger::Get().Submit(NameOf(lv), fileName, func, msgBuf);
    }

private:
    Logger() = default;

    Level         m_minLevel = Level::Debug;
    Level         m_remoteMinLevel = Level::Info;
    bool          m_remoteEnabled = true;
    std::ofstream m_file;
    HANDLE        m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    static const char* NameOf(Level lv) {
        switch (lv) {
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
        }
        return "UNKNOWN";
    }

    static const char* TagOf(Level lv) {
        switch (lv) {
        case Level::Debug: return "[DEBUG]";
        case Level::Info:  return "[INFO] ";
        case Level::Warn:  return "[WARN] ";
        case Level::Error: return "[ERROR]";
        }
        return "[?]";
    }

    void SetConsoleColor(Level lv) {
        WORD color;
        switch (lv) {
        case Level::Debug: color = FOREGROUND_INTENSITY;                                      break;
        case Level::Info:  color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;                   break;
        case Level::Warn:  color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;  break;
        case Level::Error: color = FOREGROUND_RED | FOREGROUND_INTENSITY;                     break;
        default:           color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;       break;
        }
        SetConsoleTextAttribute(m_hConsole, color);
    }

    void ResetConsoleColor() {
        SetConsoleTextAttribute(m_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
};

// __FILE__, __FUNCTION__은 컴파일러가 자동 주입 — 수동 입력 불필요
#define LOG_DEBUG(fmt, ...) Logger::Get().Log(Logger::Level::Debug, __FILE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  Logger::Get().Log(Logger::Level::Info,  __FILE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  Logger::Get().Log(Logger::Level::Warn,  __FILE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Logger::Get().Log(Logger::Level::Error, __FILE__, __FUNCTION__, fmt, ##__VA_ARGS__)
