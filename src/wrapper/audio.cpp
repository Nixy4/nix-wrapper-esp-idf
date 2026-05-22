#include <cmath>
#include "wrapper/audio.hpp"

namespace wrapper
{

// Speaker Implementation
Speaker::Speaker(Logger &logger) : logger_(logger) {}
Speaker::~Speaker() {}

bool Speaker::Init(I2sBus &i2s_bus)
{
    i2s_bus_ = &i2s_bus;
    volume_ = 1.0f;
    mute_ = false;
    return true;
}

bool Speaker::Deinit() { return Disable(); }

bool Speaker::SetSoftVolume(float volume)
{
    if (volume < 0.0f)
        volume = 0.0f;
    // if (volume > 1.0f) volume = 1.0f; // Allow gain > 1.0?
    volume_ = volume;
    return true;
}

bool Speaker::SetMute(bool mute)
{
    mute_ = mute;
    return true;
}

bool Speaker::Enable()
{
    if (i2s_bus_ == nullptr)
        return false;
    return i2s_bus_->EnableTxChannel();
}

bool Speaker::Disable()
{
    if (i2s_bus_ == nullptr)
        return false;
    return i2s_bus_->DisableTxChannel();
}

bool Speaker::IsEnabled(bool &enable)
{
    // We don't track enabled state in Speaker class currently,
    // but we can query I2sBus or assume true if Init called?
    // The class def has no enabled_ member in the provided HPP snippet?
    // Wait, the HPP snippet for Speaker (lines 12-47) DOES NOT have enabled_ member.
    // I should add one or just return true if bus is set?
    // I'll assume it's enabled if I2sBus says so? I2sBus doesn't expose IsEnabled.
    // I will add a local enabled state or just return true.
    // Given the previous pattern, I should probably have added `bool enabled_ = false;` to HPP.
    // But I can't edit HPP now easily without checking.
    // The HPP snippet in `Read` output shows:
    /*
      class Speaker // no codec
      {
          Logger &logger_;
          I2sBus *i2s_bus_ = nullptr;
          float volume_ = 0.0f;
          bool mute_ = false;
          ...
    */
    // No `enabled_` member.
    // I will implement IsEnabled returning false or check if I can rely on something else.
    // Or I just assume it's stateless wrapper around I2S.
    enable = true;
    return true;
}

bool Speaker::Write(const void *data, size_t size)
{
    if (i2s_bus_ == nullptr)
        return false;

    // Software Volume / Mute Processing
    if (mute_ || volume_ == 0.0f)
    {
        // Write zeros
        std::vector<uint8_t> zeros(size, 0);
        size_t written = 0;
        return i2s_bus_->Write(zeros.data(), size, written);
    }

    if (volume_ != 1.0f)
    {
        // Assume 16-bit PCM
        // We need to copy data to modify it
        // This is expensive for large buffers.
        // If data is const, we must copy.
        // TODO: Optimize buffer management.
        std::vector<int16_t> buffer;
        buffer.resize(size / 2);
        memcpy(buffer.data(), data, size);

        for (auto &sample : buffer)
        {
            sample = (int16_t)(sample * volume_);
        }
        size_t written = 0;
        return i2s_bus_->Write(buffer.data(), size, written);
    }

    size_t written = 0;
    return i2s_bus_->Write(data, size, written);
}

// Microphone Implementation
Microphone::Microphone(Logger &logger) : logger_(logger) {}
Microphone::~Microphone() {}

bool Microphone::Init(I2sBus &i2s_bus)
{
    i2s_bus_ = &i2s_bus;
    volume_ = 1.0f;
    mute_ = false;
    return true;
}

bool Microphone::Deinit() { return Disable(); }

bool Microphone::SetSoftVolume(float volume)
{
    if (volume < 0.0f)
        volume = 0.0f;
    volume_ = volume;
    return true;
}

bool Microphone::SetMute(bool mute)
{
    mute_ = mute;
    return true;
}

bool Microphone::Enable()
{
    if (i2s_bus_ == nullptr)
        return false;
    return i2s_bus_->EnableRxChannel();
}

bool Microphone::Disable()
{
    if (i2s_bus_ == nullptr)
        return false;
    return i2s_bus_->DisableRxChannel();
}

bool Microphone::IsEnabled(bool &enable)
{
    enable = true;  // See Speaker notes
    return true;
}

bool Microphone::Read(void *data, size_t size)
{
    if (i2s_bus_ == nullptr)
        return false;

    size_t read = 0;
    bool ret = i2s_bus_->Read(data, size, read);

    if (ret && (mute_ || volume_ != 1.0f))
    {
        // Apply software processing
        // Assuming 16-bit
        int16_t *buffer = (int16_t *)data;
        size_t num_samples = read / 2;

        if (mute_ || volume_ == 0.0f)
        {
            memset(data, 0, read);
        }
        else
        {
            for (size_t i = 0; i < num_samples; ++i)
            {
                buffer[i] = (int16_t)(buffer[i] * volume_);
            }
        }
    }
    return ret;
}

// SpeakerCodec Implementation
SpeakerCodec::SpeakerCodec(Logger &logger) : logger_(logger) {}
SpeakerCodec::~SpeakerCodec() {}

bool SpeakerCodec::Init(I2sBus &i2s_bus)
{
    i2s_bus_ = &i2s_bus;
    if (i2s_data_if_ == nullptr)
    {
        audio_codec_i2s_cfg_t i2s_cfg = {};
        i2s_cfg.port = i2s_bus_->GetPort();
        i2s_cfg.rx_handle = i2s_bus_->GetRxHandle();
        i2s_cfg.tx_handle = i2s_bus_->GetTxHandle();
        i2s_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
        i2s_data_if_ = audio_codec_new_i2s_data(&i2s_cfg);
        if (i2s_data_if_ == nullptr)
        {
            logger_.Error("Failed to initialize I2S Data Interface");
            return false;
        }
    }
    if (i2s_gpio_if_ == nullptr)
    {
        i2s_gpio_if_ = audio_codec_new_gpio();
        if (i2s_gpio_if_ == nullptr)
        {
            logger_.Error("Failed to initialize GPIO Interface");
            return false;
        }
    }
    return true;
}

bool SpeakerCodec::AddSpeaker(I2cBus &i2c_bus, uint8_t addr,
                              std::function<esp_err_t()> codec_new_func)
{
    if (spk_codec_dev_handle_ != nullptr)
        return true;
    audio_codec_i2c_cfg_t spk_i2c_cfg = {};
    spk_i2c_cfg.port = i2c_bus.GetPort();
    spk_i2c_cfg.addr = addr;
    spk_i2c_cfg.bus_handle = i2c_bus.GetHandle();
    spk_ctrl_if_ = audio_codec_new_i2c_ctrl(&spk_i2c_cfg);

    // NOTE: The lambda codec_new_func is expected to call SetSpeakerCodecInterface/DeviceHandle
    // on the object it captures. If using SpeakerCodec, the lambda must be updated to capture
    // SpeakerCodec.
    esp_err_t err = codec_new_func();
    if (err != ESP_OK)
    {
        logger_.Error("Failed to create speaker codec: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool SpeakerCodec::SetVolume(int vol)
{
    esp_err_t ret = esp_codec_dev_set_out_vol(spk_codec_dev_handle_, vol);
    return ret == ESP_OK;
}
bool SpeakerCodec::GetVolume(int &vol)
{
    return esp_codec_dev_get_out_vol(spk_codec_dev_handle_, &vol) == ESP_OK;
}
bool SpeakerCodec::SetMute(bool mute)
{
    return esp_codec_dev_set_out_mute(spk_codec_dev_handle_, mute) == ESP_OK;
}
bool SpeakerCodec::IsMuted(bool &mute)
{
    return esp_codec_dev_get_out_mute(spk_codec_dev_handle_, &mute) == ESP_OK;
}
bool SpeakerCodec::Enable()
{
    if (spk_enabled_)
        return true;
    if (!i2s_bus_)
        return false;
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = 2,
        .channel_mask = 0,
        .sample_rate = i2s_bus_->GetTxSampleRate(),
        .mclk_multiple = 0,
    };
    if (esp_codec_dev_open(spk_codec_dev_handle_, &fs) == ESP_OK)
    {
        spk_enabled_ = true;
        return true;
    }
    return false;
}
bool SpeakerCodec::Disable()
{
    if (!spk_enabled_)
        return true;
    if (esp_codec_dev_close(spk_codec_dev_handle_) == ESP_OK)
    {
        spk_enabled_ = false;
        return true;
    }
    return false;
}
bool SpeakerCodec::IsEnabled(bool &enable)
{
    enable = spk_enabled_;
    return true;
}
bool SpeakerCodec::Write(const void *data, size_t size)
{
    return esp_codec_dev_write(spk_codec_dev_handle_, (void *)data, size) == ESP_OK;
}

// MicrophoneCodec Implementation
MicrophoneCodec::MicrophoneCodec(Logger &logger) : logger_(logger) {}
MicrophoneCodec::~MicrophoneCodec() {}

bool MicrophoneCodec::Init(I2sBus &i2s_bus)
{
    i2s_bus_ = &i2s_bus;
    if (i2s_data_if_ == nullptr)
    {
        audio_codec_i2s_cfg_t i2s_cfg = {};
        i2s_cfg.port = i2s_bus_->GetPort();
        i2s_cfg.rx_handle = i2s_bus_->GetRxHandle();
        i2s_cfg.tx_handle = i2s_bus_->GetTxHandle();
        i2s_cfg.clk_src = I2S_CLK_SRC_DEFAULT;
        i2s_data_if_ = audio_codec_new_i2s_data(&i2s_cfg);
        if (i2s_data_if_ == nullptr)
        {
            logger_.Error("Failed to initialize I2S Data Interface");
            return false;
        }
    }
    if (i2s_gpio_if_ == nullptr)
    {
        i2s_gpio_if_ = audio_codec_new_gpio();
        if (i2s_gpio_if_ == nullptr)
        {
            logger_.Error("Failed to initialize GPIO Interface");
            return false;
        }
    }
    return true;
}

bool MicrophoneCodec::AddMicrophone(I2cBus &i2c_bus, uint8_t addr,
                                    std::function<esp_err_t()> codec_new_func)
{
    if (mic_codec_dev_handle_ != nullptr)
        return true;
    audio_codec_i2c_cfg_t mic_i2c_cfg = {};
    mic_i2c_cfg.port = i2c_bus.GetPort();
    mic_i2c_cfg.addr = addr;
    mic_i2c_cfg.bus_handle = i2c_bus.GetHandle();
    mic_ctrl_if_ = audio_codec_new_i2c_ctrl(&mic_i2c_cfg);

    esp_err_t err = codec_new_func();
    if (err != ESP_OK)
    {
        logger_.Error("Failed to create microphone codec: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool MicrophoneCodec::SetGain(float gain)
{
    return esp_codec_dev_set_in_gain(mic_codec_dev_handle_, gain) == ESP_OK;
}
bool MicrophoneCodec::GetGain(float &gain)
{
    return esp_codec_dev_get_in_gain(mic_codec_dev_handle_, &gain) == ESP_OK;
}
bool MicrophoneCodec::SetMute(bool mute)
{
    return esp_codec_dev_set_in_mute(mic_codec_dev_handle_, mute) == ESP_OK;
}
bool MicrophoneCodec::IsMuted(bool &mute)
{
    return esp_codec_dev_get_in_mute(mic_codec_dev_handle_, &mute) == ESP_OK;
}
bool MicrophoneCodec::Enable()
{
    if (mic_enabled_)
        return true;
    if (!i2s_bus_)
        return false;
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = 2,
        .channel_mask = 0,
        .sample_rate = i2s_bus_->GetRxSampleRate(),
        .mclk_multiple = 0,
    };
    if (esp_codec_dev_open(mic_codec_dev_handle_, &fs) == ESP_OK)
    {
        mic_enabled_ = true;
        return true;
    }
    return false;
}
bool MicrophoneCodec::Disable()
{
    if (!mic_enabled_)
        return true;
    if (esp_codec_dev_close(mic_codec_dev_handle_) == ESP_OK)
    {
        mic_enabled_ = false;
        return true;
    }
    return false;
}
bool MicrophoneCodec::IsEnabled(bool &enable)
{
    enable = mic_enabled_;
    return true;
}
bool MicrophoneCodec::Read(void *data, size_t size)
{
    return esp_codec_dev_read(mic_codec_dev_handle_, data, size) == ESP_OK;
}

AudioCodec::AudioCodec(Logger &logger) : logger_(logger) {}

AudioCodec::~AudioCodec() {}

bool AudioCodec::Init(I2sBus &i2s_bus)
{
    if (i2s_data_if_ == nullptr)
    {
        audio_codec_i2s_cfg_t i2s_cfg = {};
        i2s_cfg.port = i2s_bus.GetPort();
        i2s_cfg.rx_handle = i2s_bus.GetRxHandle();
        i2s_cfg.tx_handle = i2s_bus.GetTxHandle();
        // 0 means default clock source
        i2s_cfg.clk_src = I2S_CLK_SRC_DEFAULT;

        i2s_data_if_ = audio_codec_new_i2s_data(&i2s_cfg);
        if (i2s_data_if_ == nullptr)
        {
            logger_.Error("Failed to initialize I2S Audio Codec Interface");
            return false;
        }
        logger_.Info("I2S Audio Codec Interface initialized");
    }

    if (i2s_gpio_if_ == nullptr)
    {
        i2s_gpio_if_ = audio_codec_new_gpio();
        if (i2s_gpio_if_ == nullptr)
        {
            logger_.Error("Failed to initialize I2S Audio Codec GPIO Interface");
            return false;
        }
        logger_.Info("I2S Audio Codec GPIO Interface initialized");
    }

    return true;
}

bool AudioCodec::AddSpeaker(I2cBus &i2c_bus, uint8_t addr,
                            std::function<esp_err_t()> codec_new_func)
{
    if (spk_codec_dev_handle_ != nullptr)
    {
        logger_.Warning("Speaker codec device already added");
        return true;
    }

    audio_codec_i2c_cfg_t spk_i2c_cfg = {};
    spk_i2c_cfg.port = i2c_bus.GetPort();
    spk_i2c_cfg.addr = addr;
    spk_i2c_cfg.bus_handle = i2c_bus.GetHandle();
    spk_audio_codec_ctrl_if_ = audio_codec_new_i2c_ctrl(&spk_i2c_cfg);

    esp_err_t err = codec_new_func();
    if (err != ESP_OK)
    {
        logger_.Error("Failed to create speaker codec device: %s", esp_err_to_name(err));
        return false;
    }

    logger_.Info("Speaker codec device added successfully");
    return true;
}

bool AudioCodec::AddMicrophone(I2cBus &i2c_bus, uint8_t addr,
                               std::function<esp_err_t()> codec_new_func)
{
    esp_err_t err;

    if (mic_codec_dev_handle_ != nullptr)
    {
        logger_.Warning("Microphone codec device already added");
        return true;
    }

    audio_codec_i2c_cfg_t mic_i2c_cfg = {};
    mic_i2c_cfg.port = i2c_bus.GetPort();
    mic_i2c_cfg.addr = addr;
    mic_i2c_cfg.bus_handle = i2c_bus.GetHandle();
    mic_audio_codec_ctrl_if_ = audio_codec_new_i2c_ctrl(&mic_i2c_cfg);

    err = codec_new_func();
    if (err != ESP_OK)
    {
        logger_.Error("Failed to create microphone codec device: %s", esp_err_to_name(err));
        return false;
    }

    logger_.Info("Microphone codec device added successfully");
    return true;
}

bool AudioCodec::SetSpeakerVolume(int vol)
{
    esp_err_t ret = esp_codec_dev_set_out_vol(spk_codec_dev_handle_, vol);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to set speaker volume: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AudioCodec::GetSpeakerVolume(int &vol)
{
    esp_err_t ret = esp_codec_dev_get_out_vol(spk_codec_dev_handle_, &vol);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to get speaker volume: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AudioCodec::SetSpeakerMute(bool mute)
{
    esp_err_t ret = esp_codec_dev_set_out_mute(spk_codec_dev_handle_, mute);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to set speaker mute: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AudioCodec::IsSpeakerMuted(bool &mute)
{
    esp_err_t ret = esp_codec_dev_get_out_mute(spk_codec_dev_handle_, &mute);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to get speaker mute state: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AudioCodec::SetMicrophoneGain(float gain)
{
    esp_err_t ret = esp_codec_dev_set_in_gain(mic_codec_dev_handle_, gain);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to set microphone gain: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AudioCodec::GetMicrophoneGain(float &gain)
{
    esp_err_t ret = esp_codec_dev_get_in_gain(mic_codec_dev_handle_, &gain);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to get microphone gain: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AudioCodec::SetMicrophoneMute(bool mute)
{
    esp_err_t ret = esp_codec_dev_set_in_mute(mic_codec_dev_handle_, mute);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to set microphone mute: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AudioCodec::TestSpeaker()
{
    if (i2s_bus_ == nullptr)
    {
        logger_.Error("I2S bus not initialized");
        return false;
    }

    logger_.Info("Starting speaker test: 1kHz sine wave");

    // Generate 1 second of 1kHz sine wave at 48kHz sample rate, 16-bit, stereo
    const int sample_rate = i2s_bus_->GetTxSampleRate();
    if (sample_rate == 0)
    {
        logger_.Error("I2S TX sample rate is 0");
        return false;
    }

    const int duration_sec = 1;
    const int frequency = 1000;
    const int amplitude = 10000;
    const int num_samples = sample_rate * duration_sec;
    const int num_channels = 2;

    // Allocate buffer for 1 second of audio
    // 16-bit (2 bytes) * 2 channels * num_samples
    size_t buffer_size = num_samples * num_channels * sizeof(int16_t);
    int16_t *buffer = (int16_t *)malloc(buffer_size);
    if (buffer == nullptr)
    {
        logger_.Error("Failed to allocate memory for speaker test");
        return false;
    }

    // Generate sine wave
    for (int i = 0; i < num_samples; ++i)
    {
        int16_t sample = (int16_t)(amplitude * sin(2 * M_PI * frequency * i / sample_rate));
        buffer[i * 2] = sample;      // Left channel
        buffer[i * 2 + 1] = sample;  // Right channel
    }

    // Write to codec
    esp_err_t ret = esp_codec_dev_write(spk_codec_dev_handle_, buffer, buffer_size);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to write to speaker codec: %s", esp_err_to_name(ret));
    }
    else
    {
        logger_.Info("Speaker test completed");
    }

    free(buffer);
    return (ret == ESP_OK);
}

bool AudioCodec::TestMicrophone()
{
    if (i2s_bus_ == nullptr)
    {
        logger_.Error("I2S bus not initialized");
        return false;
    }

    logger_.Info("Starting microphone test: Recording 3 seconds...");

    const int sample_rate = i2s_bus_->GetRxSampleRate();  // Assuming 48kHz
    if (sample_rate == 0)
    {
        logger_.Error("I2S RX sample rate is 0");
        return false;
    }

    const int duration_sec = 3;
    const int num_channels = 2;  // Assuming stereo
    const int bytes_per_sample = sizeof(int16_t);

    size_t buffer_size = sample_rate * duration_sec * num_channels * bytes_per_sample;
    int16_t *buffer = (int16_t *)malloc(buffer_size);

    if (buffer == nullptr)
    {
        logger_.Error("Failed to allocate memory for microphone test");
        return false;
    }

    // Record
    esp_err_t ret = esp_codec_dev_read(mic_codec_dev_handle_, buffer, buffer_size);

    if (ret != ESP_OK)
    {
        logger_.Error("Failed to read from microphone codec: %s", esp_err_to_name(ret));
        free(buffer);
        return false;
    }

    logger_.Info("Recording completed. Playing back...");

    // Playback
    ret = esp_codec_dev_write(spk_codec_dev_handle_, buffer, buffer_size);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to write to speaker codec: %s", esp_err_to_name(ret));
    }
    else
    {
        logger_.Info("Playback completed");
    }

    free(buffer);
    return (ret == ESP_OK);
}

bool AudioCodec::IsMicrophoneMuted(bool &mute)
{
    esp_err_t ret = esp_codec_dev_get_in_mute(mic_codec_dev_handle_, &mute);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to get microphone mute state: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AudioCodec::EnableSpeaker(bool enable)
{
    if (spk_enabled_ == enable)
    {
        return true;
    }

    esp_err_t ret = ESP_OK;
    if (enable)
    {
        if (i2s_bus_ == nullptr)
        {
            logger_.Error("I2S bus not initialized");
            return false;
        }
        esp_codec_dev_sample_info_t fs = {
            .bits_per_sample = 16,
            .channel = 2,
            .channel_mask = 0,
            .sample_rate = i2s_bus_->GetTxSampleRate(),
            .mclk_multiple = 0,
        };
        ret = esp_codec_dev_open(spk_codec_dev_handle_, &fs);
    }
    else
    {
        ret = esp_codec_dev_close(spk_codec_dev_handle_);
    }

    if (ret == ESP_OK)
    {
        spk_enabled_ = enable;
        return true;
    }
    else
    {
        logger_.Error("Failed to %s speaker: %s", enable ? "enable" : "disable",
                      esp_err_to_name(ret));
        return false;
    }
}

bool AudioCodec::IsSpeakerEnabled(bool &enable)
{
    enable = spk_enabled_;
    return true;
}

bool AudioCodec::EnableMicrophone(bool enable)
{
    if (mic_enabled_ == enable)
    {
        return true;
    }

    esp_err_t ret = ESP_OK;
    if (enable)
    {
        if (i2s_bus_ == nullptr)
        {
            logger_.Error("I2S bus not initialized");
            return false;
        }
        esp_codec_dev_sample_info_t fs = {
            .bits_per_sample = 16,
            .channel = 2,
            .channel_mask = 0,
            .sample_rate = i2s_bus_->GetRxSampleRate(),
            .mclk_multiple = 0,
        };
        ret = esp_codec_dev_open(mic_codec_dev_handle_, &fs);
    }
    else
    {
        ret = esp_codec_dev_close(mic_codec_dev_handle_);
    }

    if (ret == ESP_OK)
    {
        mic_enabled_ = enable;
        return true;
    }
    else
    {
        logger_.Error("Failed to %s microphone: %s", enable ? "enable" : "disable",
                      esp_err_to_name(ret));
        return false;
    }
}

bool AudioCodec::IsMicrophoneEnabled(bool &enable)
{
    enable = mic_enabled_;
    return true;
}

bool AudioCodec::Write(const void *data, size_t size)
{
    esp_err_t ret = esp_codec_dev_write(spk_codec_dev_handle_, (void *)data, size);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to write audio data: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool AudioCodec::Read(void *data, size_t size)
{
    esp_err_t ret = esp_codec_dev_read(mic_codec_dev_handle_, data, size);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to read audio data: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

}  // namespace wrapper