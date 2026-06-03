#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#pragma comment(lib, "winhttp.lib")

// [HttpClient]
// WinHTTP 기반 동기 HTTPS 요청 래퍼. 블로킹 호출이므로 반드시 워커 스레드에서 사용한다.
// host : "xxx-default-rtdb.firebaseio.com" (스킴/끝슬래시 제외)
// path : "/leaderboard.json?orderBy=..." (앞에 '/' 포함)
// method: L"GET" 또는 L"POST"
// body : 요청 본문(POST용). GET이면 빈 문자열.
// outOk: 요청 송수신 성공 여부.
// 반환 : 응답 본문 문자열(실패 시 빈 문자열).
class HttpClient {
public:
    static std::string Request(const std::wstring& host,
                               const std::wstring& path,
                               const std::wstring& method,
                               const std::string& body,
                               bool& outOk) {
        outOk = false;
        std::string result;

        HINTERNET hSession = WinHttpOpen(L"ZombieSlayer/1.0",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return result;

        // resolve/connect/send/receive 타임아웃(ms) — 게임이 오래 매달리지 않게
        WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
            INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(),
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result; }

        const wchar_t* headers = L"Content-Type: application/json\r\n";
        BOOL sent = WinHttpSendRequest(hRequest, headers, (DWORD)-1L,
            (LPVOID)(body.empty() ? nullptr : body.data()),
            (DWORD)body.size(), (DWORD)body.size(), 0);

        if (sent && WinHttpReceiveResponse(hRequest, nullptr)) {
            for (;;) {
                DWORD avail = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &avail)) break;
                if (avail == 0) break;
                std::string buf(avail, '\0');
                DWORD read = 0;
                if (!WinHttpReadData(hRequest, &buf[0], avail, &read)) break;
                buf.resize(read);
                result += buf;
            }
            outOk = true;
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }
};
