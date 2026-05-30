#pragma once
#include "ObjectBase.hpp"

// [UIManager]
// UI 컴포넌트를 담는 전용 GameObject.
// AddComponent()로 UI 컴포넌트를 직접 추가하고,
// Update()/Render()를 호출하면 내부 컴포넌트들이 순서대로 실행된다.
// world와 독립적으로 GameLoop가 직접 소유한다.
class UIManager : public GameObject {
public:
    UIManager() : GameObject(0.0f, 0.0f, 0.0f) {}
};
