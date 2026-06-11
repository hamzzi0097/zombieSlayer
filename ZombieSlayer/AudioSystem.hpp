#pragma once

#include <windows.h>
#include <xaudio2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <vector>
#include <string>
#include <cstdint>
#include "Logger.hpp"

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

class SoundClip {
public:
    SoundClip() = default;
    SoundClip(const SoundClip&) = delete;
    SoundClip& operator=(const SoundClip&) = delete;

    bool Load(const std::wstring& path) {
        m_audioData.clear();
        m_waveFormat.clear();

        IMFSourceReader* reader = nullptr;
        IMFMediaType* requestedType = nullptr;
        IMFMediaType* actualType = nullptr;

        HRESULT hr = MFCreateSourceReaderFromURL(path.c_str(), nullptr, &reader);
        if (FAILED(hr)) {
            LOG_ERROR("Failed to open sound file. hr=0x%08X", hr);
            return false;
        }

        hr = MFCreateMediaType(&requestedType);
        if (SUCCEEDED(hr))
            hr = requestedType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        if (SUCCEEDED(hr))
            hr = requestedType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        if (SUCCEEDED(hr))
            hr = reader->SetCurrentMediaType(
                MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, requestedType);
        if (SUCCEEDED(hr))
            hr = reader->GetCurrentMediaType(
                MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actualType);

        WAVEFORMATEX* waveFormat = nullptr;
        UINT32 waveFormatSize = 0;
        if (SUCCEEDED(hr))
            hr = MFCreateWaveFormatExFromMFMediaType(
                actualType, &waveFormat, &waveFormatSize,
                MFWaveFormatExConvertFlag_Normal);

        if (SUCCEEDED(hr)) {
            const auto* bytes = reinterpret_cast<const uint8_t*>(waveFormat);
            m_waveFormat.assign(bytes, bytes + waveFormatSize);
        }

        if (waveFormat)
            CoTaskMemFree(waveFormat);

        while (SUCCEEDED(hr)) {
            DWORD flags = 0;
            IMFSample* sample = nullptr;

            hr = reader->ReadSample(
                MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                0, nullptr, &flags, nullptr, &sample);

            if (FAILED(hr))
                break;

            if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
                if (sample) sample->Release();
                break;
            }

            if (sample) {
                IMFMediaBuffer* buffer = nullptr;
                hr = sample->ConvertToContiguousBuffer(&buffer);

                if (SUCCEEDED(hr)) {
                    BYTE* data = nullptr;
                    DWORD currentLength = 0;
                    hr = buffer->Lock(&data, nullptr, &currentLength);

                    if (SUCCEEDED(hr)) {
                        m_audioData.insert(
                            m_audioData.end(), data, data + currentLength);
                        buffer->Unlock();
                    }
                }

                if (buffer) buffer->Release();
                sample->Release();
            }
        }

        if (actualType) actualType->Release();
        if (requestedType) requestedType->Release();
        reader->Release();

        if (FAILED(hr) || m_waveFormat.empty() || m_audioData.empty()) {
            LOG_ERROR("Failed to decode sound file. hr=0x%08X", hr);
            m_audioData.clear();
            m_waveFormat.clear();
            return false;
        }

        return true;
    }

    bool IsLoaded() const {
        return !m_waveFormat.empty() && !m_audioData.empty();
    }

    const WAVEFORMATEX* GetFormat() const {
        return IsLoaded()
            ? reinterpret_cast<const WAVEFORMATEX*>(m_waveFormat.data())
            : nullptr;
    }

    const uint8_t* GetData() const {
        return m_audioData.data();
    }

    UINT32 GetDataSize() const {
        return static_cast<UINT32>(m_audioData.size());
    }

private:
    std::vector<uint8_t> m_waveFormat;
    std::vector<uint8_t> m_audioData;
};

class AudioSystem {
public:
    AudioSystem() = default;
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    ~AudioSystem() {
        Shutdown();
    }

    bool Initialize() {
        if (m_xaudio)
            return true;

        HRESULT hr = MFStartup(MF_VERSION);
        if (FAILED(hr)) {
            LOG_ERROR("MFStartup failed. hr=0x%08X", hr);
            return false;
        }
        m_mediaFoundationStarted = true;

        hr = XAudio2Create(&m_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) {
            LOG_ERROR("XAudio2Create failed. hr=0x%08X", hr);
            Shutdown();
            return false;
        }

        hr = m_xaudio->CreateMasteringVoice(&m_masterVoice);
        if (FAILED(hr)) {
            LOG_ERROR("CreateMasteringVoice failed. hr=0x%08X", hr);
            Shutdown();
            return false;
        }

        return true;
    }

    void Shutdown() {
        for (IXAudio2SourceVoice* voice : m_activeVoices) {
            voice->Stop();
            voice->DestroyVoice();
        }
        m_activeVoices.clear();

        if (m_masterVoice) {
            m_masterVoice->DestroyVoice();
            m_masterVoice = nullptr;
        }

        if (m_xaudio) {
            m_xaudio->Release();
            m_xaudio = nullptr;
        }

        if (m_mediaFoundationStarted) {
            MFShutdown();
            m_mediaFoundationStarted = false;
        }
    }

    bool PlayOneShot(const SoundClip& clip, float volume = 1.0f) {
        if (!m_xaudio || !clip.IsLoaded())
            return false;

        IXAudio2SourceVoice* voice = nullptr;
        HRESULT hr = m_xaudio->CreateSourceVoice(&voice, clip.GetFormat());
        if (FAILED(hr))
            return false;

        voice->SetVolume(volume);

        XAUDIO2_BUFFER buffer = {};
        buffer.AudioBytes = clip.GetDataSize();
        buffer.pAudioData = clip.GetData();
        buffer.Flags = XAUDIO2_END_OF_STREAM;

        hr = voice->SubmitSourceBuffer(&buffer);
        if (SUCCEEDED(hr))
            hr = voice->Start();

        if (FAILED(hr)) {
            voice->DestroyVoice();
            return false;
        }

        m_activeVoices.push_back(voice);
        return true;
    }

    void Update() {
        auto it = m_activeVoices.begin();
        while (it != m_activeVoices.end()) {
            XAUDIO2_VOICE_STATE state = {};
            (*it)->GetState(&state);

            if (state.BuffersQueued == 0) {
                (*it)->DestroyVoice();
                it = m_activeVoices.erase(it);
            }
            else {
                ++it;
            }
        }
    }

private:
    IXAudio2* m_xaudio = nullptr;
    IXAudio2MasteringVoice* m_masterVoice = nullptr;
    std::vector<IXAudio2SourceVoice*> m_activeVoices;
    bool m_mediaFoundationStarted = false;
};
