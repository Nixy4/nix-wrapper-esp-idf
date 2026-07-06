#pragma once

#include <functional>
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "wrapper/logger.hpp"
#include "wrapper/i2c.hpp"
#include "wrapper/i2s.hpp"

namespace wrapper
{
class Speaker  // no codec
{
    Logger& logger_;
    I2sBus* i2s_bus_ = nullptr;

    float volume_ = 0.0f;
    bool mute_ = false;

   public:
    Speaker(Logger& logger);
    ~Speaker();

    Logger& GetLogger() const { return logger_; }
    I2sBus& GetI2sBus() const { return *i2s_bus_; }

    bool Init(I2sBus& i2s_bus);
    bool Deinit();

    bool SetSoftVolume(float volume);
    float GetSoftVolume() const { return volume_; }

    bool SetMute(bool mute);
    bool IsMuted(bool& mute) const { return mute_; }

    bool Enable();
    bool Disable();
    bool IsEnabled(bool& enable);

    bool Write(const void* data, size_t size);

    template <typename T>
    bool Write(const std::vector<T>& data)
    {
        return Write(data.data(), data.size() * sizeof(T));
    }
};

class Microphone  // no codec
{
    Logger& logger_;
    I2sBus* i2s_bus_ = nullptr;

    float volume_ = 0.0f;
    bool mute_ = false;

   public:
    Microphone(Logger& logger);
    ~Microphone();

    Logger& GetLogger() const { return logger_; }
    I2sBus& GetI2sBus() const { return *i2s_bus_; }

    bool Init(I2sBus& i2s_bus);
    bool Deinit();

    bool SetSoftVolume(float volume);
    float GetSoftVolume() const { return volume_; }

    bool SetMute(bool mute);
    bool IsMuted(bool& mute) const { return mute_; }

    bool Enable();
    bool Disable();
    bool IsEnabled(bool& enable);

    bool Read(void* data, size_t size);

    template <typename T>
    bool Read(std::vector<T>& data, size_t count)
    {
        data.resize(count);
        return Read(data.data(), count * sizeof(T));
    }
};

class SpeakerCodec
{
    Logger& logger_;
    I2sBus* i2s_bus_ = nullptr;
    const audio_codec_data_if_t* i2s_data_if_ = nullptr;
    const audio_codec_gpio_if_t* i2s_gpio_if_ = nullptr;

    const audio_codec_ctrl_if_t* spk_ctrl_if_ = nullptr;
    const audio_codec_if_t* spk_codec_if_ = nullptr;
    esp_codec_dev_handle_t spk_codec_dev_handle_ = nullptr;
    bool spk_enabled_ = false;

   public:
    SpeakerCodec(Logger& logger);
    ~SpeakerCodec();

    Logger& GetLogger() const { return logger_; }
    I2sBus& GetI2sBus() const { return *i2s_bus_; }

    const audio_codec_data_if_t* GetDataInterface() const { return i2s_data_if_; }
    const audio_codec_gpio_if_t* GetGpioInterface() const { return i2s_gpio_if_; }
    const audio_codec_ctrl_if_t* GetCtrlInterface() const { return spk_ctrl_if_; }
    const audio_codec_if_t* GetCodecInterface() const { return spk_codec_if_; }

    // operations
    bool Init(I2sBus& i2s_bus);
    bool AddSpeaker(I2cBus& i2c_bus, uint8_t addr, std::function<esp_err_t()> codec_new_func);

    void SetCodecInterface(const audio_codec_if_t* codec_if) { spk_codec_if_ = codec_if; }
    void SetCodecDeviceHandle(esp_codec_dev_handle_t handle) { spk_codec_dev_handle_ = handle; }

    bool SetVolume(int vol);
    bool GetVolume(int& vol);

    bool SetMute(bool mute);
    bool IsMuted(bool& mute);

    bool Enable();
    bool Disable();
    bool IsEnabled(bool& enable);

    bool Write(const void* data, size_t size);

    template <typename T>
    bool Write(std::vector<T>& data)
    {
        return Write(data.data(), data.size() * sizeof(T));
    }
};

class MicrophoneCodec
{
    Logger& logger_;
    I2sBus* i2s_bus_ = nullptr;
    const audio_codec_data_if_t* i2s_data_if_ = nullptr;
    const audio_codec_gpio_if_t* i2s_gpio_if_ = nullptr;

    const audio_codec_ctrl_if_t* mic_ctrl_if_ = nullptr;
    const audio_codec_if_t* mic_codec_if_ = nullptr;
    esp_codec_dev_handle_t mic_codec_dev_handle_ = nullptr;
    bool mic_enabled_ = false;

   public:
    MicrophoneCodec(Logger& logger);
    ~MicrophoneCodec();

    Logger& GetLogger() const { return logger_; }
    I2sBus& GetI2sBus() const { return *i2s_bus_; }

    const audio_codec_data_if_t* GetDataInterface() const { return i2s_data_if_; }
    const audio_codec_gpio_if_t* GetGpioInterface() const { return i2s_gpio_if_; }
    const audio_codec_ctrl_if_t* GetCtrlInterface() const { return mic_ctrl_if_; }
    const audio_codec_if_t* GetCodecInterface() const { return mic_codec_if_; }

    // operations
    bool Init(I2sBus& i2s_bus);
    bool AddMicrophone(I2cBus& i2c_bus, uint8_t addr, std::function<esp_err_t()> codec_new_func);

    void SetCodecInterface(const audio_codec_if_t* codec_if) { mic_codec_if_ = codec_if; }
    void SetCodecDeviceHandle(esp_codec_dev_handle_t handle) { mic_codec_dev_handle_ = handle; }

    bool SetGain(float gain);
    bool GetGain(float& gain);

    bool SetMute(bool mute);
    bool IsMuted(bool& mute);

    bool Enable();
    bool Disable();
    bool IsEnabled(bool& enable);

    bool Read(void* data, size_t size);

    template <typename T>
    bool Read(std::vector<T>& data, size_t count)
    {
        data.resize(count);
        return Read(data.data(), count * sizeof(T));
    }
};

class AudioCodec
{
    // common
    Logger& logger_;
    I2sBus* i2s_bus_ = nullptr;
    const audio_codec_data_if_t* i2s_data_if_ = nullptr;
    const audio_codec_gpio_if_t* i2s_gpio_if_ = nullptr;
    // speaker
    const audio_codec_ctrl_if_t* spk_audio_codec_ctrl_if_ = nullptr;
    const audio_codec_if_t* spk_audio_codec_if_ = nullptr;
    esp_codec_dev_handle_t spk_codec_dev_handle_ = nullptr;
    bool spk_enabled_ = false;
    // microphone
    const audio_codec_ctrl_if_t* mic_audio_codec_ctrl_if_ = nullptr;
    const audio_codec_if_t* mic_audio_codec_if_ = nullptr;
    esp_codec_dev_handle_t mic_codec_dev_handle_ = nullptr;
    bool mic_enabled_ = false;

   public:
    AudioCodec(Logger& logger);
    ~AudioCodec();

    Logger& GetLogger() const { return logger_; }
    I2sBus& GetI2sBus() const { return *i2s_bus_; }

    const audio_codec_data_if_t* GetDataInterface() const { return i2s_data_if_; }
    const audio_codec_gpio_if_t* GetGpioInterface() const { return i2s_gpio_if_; }
    const audio_codec_ctrl_if_t* GetSpeakerCtrlInterface() const
    {
        return spk_audio_codec_ctrl_if_;
    }
    const audio_codec_ctrl_if_t* GetMicrophoneCtrlInterface() const
    {
        return mic_audio_codec_ctrl_if_;
    }

    void SetSpeakerCodecInterface(const audio_codec_if_t* codec_if)
    {
        spk_audio_codec_if_ = codec_if;
    }
    void SetMicrophoneCodecInterface(const audio_codec_if_t* codec_if)
    {
        mic_audio_codec_if_ = codec_if;
    }
    void SetSpeakerCodecDeviceHandle(esp_codec_dev_handle_t handle)
    {
        spk_codec_dev_handle_ = handle;
    }
    void SetMicrophoneCodecDeviceHandle(esp_codec_dev_handle_t handle)
    {
        mic_codec_dev_handle_ = handle;
    }

    // operations
    bool Init(I2sBus& i2m_bus);

    bool AddSpeaker(I2cBus& i2c_bus, uint8_t addr, std::function<esp_err_t()> codec_new_func);
    bool AddMicrophone(I2cBus& i2c_bus, uint8_t addr, std::function<esp_err_t()> codec_new_func);

    bool SetSpeakerVolume(int vol);
    bool GetSpeakerVolume(int& vol);

    bool SetSpeakerMute(bool mute);
    bool IsSpeakerMuted(bool& mute);

    bool SetMicrophoneGain(float gain);
    bool GetMicrophoneGain(float& gain);

    bool SetMicrophoneMute(bool mute);
    bool IsMicrophoneMuted(bool& mute);

    bool EnableSpeaker(bool enable);
    bool IsSpeakerEnabled(bool& enable);

    bool EnableMicrophone(bool enable);
    bool IsMicrophoneEnabled(bool& enable);

    bool Write(const void* data, size_t size);
    bool Read(void* data, size_t size);

    template <typename T>
    bool Write(const std::vector<T>& data)
    {
        return Write(data.data(), data.size() * sizeof(T));
    }

    template <typename T>
    bool Read(std::vector<T>& data, size_t count)
    {
        data.resize(count);
        return Read(data.data(), count * sizeof(T));
    }

    bool TestSpeaker();
    bool TestMicrophone();
};

}  // namespace wrapper
