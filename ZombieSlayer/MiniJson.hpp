#pragma once
#include <string>

// [MiniJson]
// 리더보드(Firebase Realtime DB) 통신에 필요한 최소 JSON 기능만 담은 경량 파서.
// nlohmann/json(약 25,000줄) 대체용 — 이 게임에서 실제로 쓰는 3가지만 지원한다.
//   1) 문자열 이스케이프 출력            : Escape()
//   2) 최상위 객체의 멤버 개수 세기       : Parser::CountObjectMembers()   (shallow=true 응답)
//   3) 각 멤버 객체에서 name/score 추출   : Parser::ForEachEntry()          (TOP10 응답)
// 객체/배열/문자열/숫자/불리언/null을 파싱하며, 손상된 입력에도 예외 없이 안전하게 멈춘다.
namespace MiniJson {

    // 문자열을 JSON 규칙대로 이스케이프해서 반환.
    inline std::string Escape(const std::string& s) {
        static const char* kHex = "0123456789abcdef";
        std::string out;
        out.reserve(s.size() + 2);
        for (unsigned char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b";  break;
                case '\f': out += "\\f";  break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:
                    if (c < 0x20) { out += "\\u00"; out += kHex[(c >> 4) & 0xF]; out += kHex[c & 0xF]; }
                    else          { out += (char)c; }
            }
        }
        return out;
    }

    class Parser {
    public:
        explicit Parser(const std::string& src) : s(src), i(0) {}

        // 최상위 객체의 멤버 개수. (객체가 아니거나 비었으면 0)
        int CountObjectMembers() {
            i = 0; SkipWs();
            if (Peek() != '{') return 0;
            ++i; SkipWs();
            if (Peek() == '}') return 0;
            int n = 0;
            for (;;) {
                std::string key;
                if (!ParseString(key)) return n;     // 손상 → 지금까지 센 값
                SkipWs();
                if (Peek() != ':') return n;
                ++i;
                if (!SkipValue()) return n;
                ++n;
                SkipWs();
                if (Peek() == ',') { ++i; SkipWs(); continue; }
                break;
            }
            return n;
        }

        // 최상위 객체의 각 멤버 값(중첩 객체)에서 name/score를 뽑아 emit(name, score) 호출.
        template <typename F>
        void ForEachEntry(F&& emit) {
            i = 0; SkipWs();
            if (Peek() != '{') return;
            ++i; SkipWs();
            if (Peek() == '}') return;
            for (;;) {
                std::string key;
                if (!ParseString(key)) return;
                SkipWs();
                if (Peek() != ':') return;
                ++i; SkipWs();
                std::string name; long long sc = 0;
                bool gotName = false, gotScore = false;
                if (Peek() == '{') ReadEntryObject(name, sc, gotName, gotScore);
                else if (!SkipValue()) return;
                if (gotName || gotScore) emit(name, (int)sc);
                SkipWs();
                if (Peek() == ',') { ++i; SkipWs(); continue; }
                break;
            }
        }

    private:
        const std::string& s;
        size_t i;

        char Peek() const { return i < s.size() ? s[i] : '\0'; }
        bool IsSpace(char c) const { return c==' '||c=='\t'||c=='\n'||c=='\r'; }
        void SkipWs() { while (i < s.size() && IsSpace(s[i])) ++i; }

        // '"'에서 시작해 닫는 '"' 다음으로 i 이동, 이스케이프 디코드. 성공 시 true.
        bool ParseString(std::string& out) {
            out.clear();
            if (Peek() != '"') return false;
            ++i;
            while (i < s.size()) {
                char c = s[i++];
                if (c == '"') return true;
                if (c != '\\') { out += c; continue; }
                if (i >= s.size()) return false;
                char e = s[i++];
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': { unsigned cp; if (!ReadHex4(cp)) return false; AppendUtf8(out, cp); break; }
                    default:  out += e; break;
                }
            }
            return false; // 닫는 따옴표 없음
        }

        bool ReadHex4(unsigned& cp) {
            if (i + 4 > s.size()) return false;
            cp = 0;
            for (int k = 0; k < 4; ++k) {
                char c = s[i++]; cp <<= 4;
                if      (c>='0'&&c<='9') cp |= (unsigned)(c-'0');
                else if (c>='a'&&c<='f') cp |= (unsigned)(c-'a'+10);
                else if (c>='A'&&c<='F') cp |= (unsigned)(c-'A'+10);
                else return false;
            }
            return true;
        }

        // BMP 범위만 UTF-8로 인코딩(서로게이트 페어 미지원 — 리더보드 이름엔 불필요).
        static void AppendUtf8(std::string& out, unsigned cp) {
            if (cp < 0x80) { out += (char)cp; }
            else if (cp < 0x800) {
                out += (char)(0xC0 | (cp >> 6));
                out += (char)(0x80 | (cp & 0x3F));
            } else {
                out += (char)(0xE0 | (cp >> 12));
                out += (char)(0x80 | ((cp >> 6) & 0x3F));
                out += (char)(0x80 | (cp & 0x3F));
            }
        }

        // 임의의 값 1개를 건너뛴다. 성공 시 i가 그 값 다음을 가리킴.
        bool SkipValue() {
            SkipWs();
            switch (Peek()) {
                case '"': { std::string t; return ParseString(t); }
                case '{': return SkipContainer('{', '}');
                case '[': return SkipContainer('[', ']');
                default:  return SkipLiteral(); // number / true / false / null
            }
        }

        bool SkipContainer(char open, char close) {
            if (Peek() != open) return false;
            ++i;
            int depth = 1;
            while (i < s.size()) {
                char c = s[i];
                if (c == '"') { std::string t; if (!ParseString(t)) return false; continue; }
                if (c == open)  { ++depth; ++i; continue; }
                if (c == close) { --depth; ++i; if (depth == 0) return true; continue; }
                ++i;
            }
            return false;
        }

        bool SkipLiteral() {
            size_t start = i;
            while (i < s.size() && !IsSpace(s[i]) && s[i]!=',' && s[i]!='}' && s[i]!=']') ++i;
            return i > start;
        }

        long long ParseInt() {
            SkipWs();
            bool neg = (Peek() == '-');
            if (neg) ++i;
            long long v = 0;
            while (i < s.size() && s[i] >= '0' && s[i] <= '9') { v = v*10 + (s[i]-'0'); ++i; }
            while (i < s.size() && !IsSpace(s[i]) && s[i]!=',' && s[i]!='}' && s[i]!=']') ++i; // 소수/지수 꼬리 흘려보냄
            return neg ? -v : v;
        }

        // '{' 위치에서 시작해 name(문자열)/score(정수)만 읽고 객체 '}' 다음으로 i 이동.
        void ReadEntryObject(std::string& name, long long& score, bool& gotName, bool& gotScore) {
            ++i; SkipWs(); // consume '{'
            if (Peek() == '}') { ++i; return; }
            for (;;) {
                std::string key;
                if (!ParseString(key)) { SkipContainer('{','}'); return; }
                SkipWs();
                if (Peek() != ':') { SkipContainer('{','}'); return; }
                ++i; SkipWs();
                if (key == "name" && Peek() == '"')                    { ParseString(name); gotName = true; }
                else if (key == "score" && (Peek()=='-'||(Peek()>='0'&&Peek()<='9'))) { score = ParseInt(); gotScore = true; }
                else if (!SkipValue()) return;
                SkipWs();
                if (Peek() == ',') { ++i; SkipWs(); continue; }
                if (Peek() == '}') { ++i; return; }
                return;
            }
        }
    };

} // namespace MiniJson
