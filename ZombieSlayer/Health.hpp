#pragma once
#include "ObjectBase.hpp"

// [Health 컴포넌트]
// 목숨 기반 체력 시스템. 공격 1회당 목숨 1 감소, 0이 되면 오브젝트 사망 처리.
class Health : public Component {
public:
    Health(int maxLives = 3) : m_maxLives(maxLives), m_lives(maxLives) {}

    // 데미지 1회 적용. 목숨이 0이 되면 pOwner->isObjDead = true.
    void TakeDamage() {
        if (m_lives <= 0) return;
        m_lives--;
        if (m_lives <= 0) {
            m_lives = 0;
            pOwner->isObjDead = true;
        }
    }

    int GetLives()    const { return m_lives; }
    int GetMaxLives() const { return m_maxLives; }

    bool IsDead()     const { return m_lives <= 0; }

    void Start()           override {}
    void Input()           override {}
    void Update(float dt)  override {}
    void Render()          override {}

private:
    int m_maxLives;
    int m_lives;
};
