#pragma once
#include <cmath>
#include "ObjectBase.hpp"
#include "TextUI.hpp"

// [BlinkUIUpdater]
// 참조하는 TextUI의 색(RGB 밝기)을 sine로 펄스시켜 깜빡이게 한다(SetColor만 호출).
// alpha 블렌딩에 의존하지 않고 RGB만 어둡게/밝게 → 어두운 배경에서 자연스러운 점멸.
// 라벨 렌더링은 TextUI 자신이 수행 → 컨트롤러는 Render 없음.
class BlinkUIUpdater : public Component {
public:
    BlinkUIUpdater(TextUI* label, XMFLOAT4 baseColor, float speed = 4.5f)
        : m_label(label), m_base(baseColor), m_speed(speed) {}

    void Start() override {}
    void Input() override {}

    void Update(float dt) override {
        if (m_t > 1000.0f) m_t = 0.0f; // 장기 실행 시 t 누적 방지
        m_t += dt * m_speed;
        float k = 0.55f + 0.45f * sinf(m_t); // 0.1 ~ 1.0 밝기 펄스
        if (m_label)
            m_label->SetColor(XMFLOAT4(m_base.x * k, m_base.y * k, m_base.z * k, 1.0f));
    }

    void Render() override {}

private:
    TextUI*  m_label = nullptr; // 참조 (소유 X)
    XMFLOAT4 m_base;            // 원본 색
    float    m_speed;          // 펄스 속도
    float    m_t = 0.0f;
};
