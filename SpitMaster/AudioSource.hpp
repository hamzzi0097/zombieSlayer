#pragma once

#include "ObjectBase.hpp"
#include "AudioSystem.hpp"

class AudioSource : public Component {
public:
    AudioSource(AudioSystem* audioSystem, const SoundClip* clip, float volume = 1.0f)
        : m_audioSystem(audioSystem), m_clip(clip), m_volume(volume) {}

    bool PlayOneShot() {
        return m_audioSystem && m_clip
            ? m_audioSystem->PlayOneShot(*m_clip, m_volume)
            : false;
    }

    void Start() override {}
    void Input() override {}
    void Update(float dt) override {}
    void Render() override {}

private:
    AudioSystem* m_audioSystem = nullptr;
    const SoundClip* m_clip = nullptr;
    float m_volume = 1.0f;
};
