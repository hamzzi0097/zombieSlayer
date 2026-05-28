#pragma once
#include "Framework.hpp"
#include "GraphicsContext.hpp"

class GameObject;

class Component {
public:
    GameObject* pOwner = nullptr;
    bool isStarted = false;

    virtual void Start() = 0;
    virtual void Input() = 0;
    virtual void Update(float dt) = 0;
    virtual void Render() = 0;
    virtual ~Component() {}
    virtual void OnCollision(GameObject* obj) {}    // 충돌 발생 시 각 컴포넌트에게 제공하는 콜백함수
};

class GameObject {
public:
    XMFLOAT3 pos = { 0, 0, 0 };
    XMFLOAT3 rot = { 0, 0, 0 };
    XMFLOAT3 scale = { 1, 1, 1 };
    bool isObjDead = false;
    std::vector<Component*> components;

    GameObject(float x, float y, float z) {
        pos = { x, y, z };
    }

    ~GameObject() {
        for (auto c : components) delete c;
    }

    void AddComponent(Component* c) {
        c->pOwner = this;
        components.push_back(c);
    }

    // GameObject에 있는 컴포넌트 중 요구하는 타입의 컴포넌트를 찾아 반환
    template<typename T>
    T* GetComponent()
    {
        for (auto c : components)
        {
            if (auto compo = dynamic_cast<T*>(c))
            {
                return compo;
            }
        }

        return nullptr;
    }
    // 충돌 이벤트를 이 오브젝트에 붙은 모든 컴포넌트로 전달
    void OnCollision(GameObject* obj)
    {
        for (auto c : components)
        {
            c->OnCollision(obj);
        }
    }

    void Input() {
        for (auto c : components) c->Input();
    }

    void Update(float dt) {
        for (auto c : components) {
            if (!c->isStarted) {
                c->Start();
                c->isStarted = true;
            }
            c->Update(dt);
        }
    }

    void Render() {
        for (auto c : components) c->Render();
    }
};