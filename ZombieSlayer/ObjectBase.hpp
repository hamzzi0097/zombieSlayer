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