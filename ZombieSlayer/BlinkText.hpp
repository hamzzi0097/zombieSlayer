#pragma once
#include <cmath>
#include <string>
#include "ObjectBase.hpp"
#include "TextUI.hpp"

// [BlinkText 컴포넌트]
// TextUI 한 줄을 sine 곡선으로 밝기(RGB)를 펄스시켜 깜빡이게 한다.
// alpha 블렌딩에 의존하지 않고 RGB만 어둡게/밝게 → 어두운 배경에서 자연스러운 점멸.
//
// 로비의 "PRESS SPACE" 같은 시작 유도 문구에 사용.
class BlinkText : public Component {
public:
    BlinkText(ShaderSet shaders, const std::string& text,
              float cx, float cy, float height, XMFLOAT4 color, float speed = 4.5f)
        : m_base(color), m_speed(speed) {
        m_label = new TextUI(shaders, text, cx, cy, height, color, TextUI::Align::Center);
    }

    ~BlinkText() { delete m_label; }

    void Start() override { m_label->Start(); }

    void Update(float dt) override {
        m_t += dt * m_speed;
        // 0.1 ~ 1.0 사이로 밝기 펄스
        float k = 0.55f + 0.45f * sinf(m_t);
        m_label->SetColor(XMFLOAT4(m_base.x * k, m_base.y * k, m_base.z * k, 1.0f));
    }

    void Render() override { m_label->Render(); }
    void Input()  override {}

private:
    TextUI*  m_label = nullptr;
    XMFLOAT4 m_base;          // 원본 색
    float    m_speed;         // 펄스 속도
    float    m_t = 0.0f;
};
