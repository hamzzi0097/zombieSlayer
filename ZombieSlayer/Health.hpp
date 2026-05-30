#pragma once
#include "ObjectBase.hpp"
#include <functional>

// [Health 컴포넌트]
// 목숨 기반 체력 시스템. TakeDamage() 호출 시 목숨 1 감소.
// onDamaged 콜백을 외부에서 등록하면 피격 시 자동으로 호출된다.
class Health : public Component {
public:
    std::function<void()> onDamaged;    // 피격 시 호출될 콜백 (외부에서 주입)

    Health(int maxLives = 3) : m_maxLives(maxLives), m_lives(maxLives) {}

    void TakeDamage() {
        if (m_lives <= 0) return;
        m_lives--;
        LOG_DEBUG("Player life : %d", m_lives);

        if (onDamaged) onDamaged();     // 콜백 호출 (HitEffect 등)

        if (m_lives <= 0) {
            m_lives = 0;
            pOwner->isObjDead = true;
            LOG_DEBUG("Player Dead!");
        }
    }

    int  GetLives()    const { return m_lives; }
    int  GetMaxLives() const { return m_maxLives; }
    bool IsDead()      const { return m_lives <= 0; }

    void Start()          override {}
    void Input()          override {}
    void Update(float dt) override {}
    void Render()         override {}

private:
    int m_maxLives;
    int m_lives;
};
