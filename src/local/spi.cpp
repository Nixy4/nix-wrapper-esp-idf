#include "local/spi.hpp"
#include <cstring>

// --- SpiBus ---

using namespace wrapper;

SpiBus::SpiBus(Logger& logger)
    : logger_(logger),
      host_id_(SPI2_HOST),
      initialized_(false),
      config_(SPI2_HOST,                   // host
              GPIO_NUM_NC,                 // mosi
              GPIO_NUM_NC,                 // miso
              GPIO_NUM_NC,                 // sclk
              GPIO_NUM_NC,                 // quadwp
              GPIO_NUM_NC,                 // quadhd
              GPIO_NUM_NC,                 // data4
              GPIO_NUM_NC,                 // data5
              GPIO_NUM_NC,                 // data6
              GPIO_NUM_NC,                 // data7
              false,                       // data_default_level
              4092,                        // max_transfer
              SPICOMMON_BUSFLAG_MASTER,    // bus_flags
              ESP_INTR_CPU_AFFINITY_AUTO,  // isr_cpu
              0,                           // intr_flags
              SPI_DMA_CH_AUTO)
{  // dma
}

SpiBus::~SpiBus() { Deinit(); }

Logger& SpiBus::GetLogger() { return logger_; }

spi_host_device_t SpiBus::GetHostId() const { return host_id_; }

bool SpiBus::Init(const SpiBusConfig& config)
{
    if (initialized_)
    {
        logger_.Warning("Already initialized. Deinitializing first.");
        Deinit();
    }

    // Save config for Reset
    config_ = config;

    esp_err_t ret = spi_bus_initialize(config.host_id, &config, config.dma_chan);
    if (ret == ESP_OK)
    {
        host_id_ = config.host_id;
        initialized_ = true;
        logger_.Info("Initialized (Host: %d, MOSI: %d, MISO: %d, SCLK: %d)", config.host_id,
                     config.mosi_io_num, config.miso_io_num, config.sclk_io_num);
        return true;
    }
    else
    {
        logger_.Error("Failed to initialize: %s", esp_err_to_name(ret));
        return false;
    }
}

bool SpiBus::Deinit()
{
    if (initialized_)
    {
        esp_err_t ret = spi_bus_free(host_id_);
        if (ret == ESP_OK)
        {
            logger_.Info("Deinitialized");
            initialized_ = false;
            return true;
        }
        else
        {
            logger_.Error("Failed to deinitialize: %s", esp_err_to_name(ret));
            return false;
        }
    }
    return true;
}

bool SpiBus::Reset()
{
    if (!initialized_)
    {
        logger_.Error("Cannot reset: Not initialized");
        return false;
    }

    logger_.Info("Resetting...");

    // Deinit current bus
    if (!Deinit())
    {
        return false;
    }

    // Re-init with saved config
    return Init(config_);
}

// --- SpiDevice ---

SpiDevice::SpiDevice(Logger& logger) : logger_(logger), dev_handle_(NULL) {}

SpiDevice::~SpiDevice() { Deinit(); }

Logger& SpiDevice::GetLogger() { return logger_; }

bool SpiDevice::Init(const SpiBus& bus, const SpiDeviceConfig& config)
{
    if (dev_handle_ != NULL)
    {
        logger_.Warning("Device already initialized. Deinitializing first.");
        Deinit();
    }

    // SPI bus must be initialized before adding device
    // We check this implicitly by bus.GetHostId(), but really the user should ensure bus is Init'd.
    // Unlike I2C new driver, SPI driver relies on Host ID.

    esp_err_t ret = spi_bus_add_device(bus.GetHostId(), &config, &dev_handle_);
    if (ret == ESP_OK)
    {
        logger_.Info("Device initialized (CS: %d, Speed: %d Hz)", config.spics_io_num,
                     config.clock_speed_hz);
        return true;
    }
    else
    {
        logger_.Error("Failed to add device: %s", esp_err_to_name(ret));
        return false;
    }
}

bool SpiDevice::Deinit()
{
    if (dev_handle_ != NULL)
    {
        esp_err_t ret = spi_bus_remove_device(dev_handle_);
        if (ret == ESP_OK)
        {
            logger_.Info("Device deinitialized");
            dev_handle_ = NULL;
            return true;
        }
        else
        {
            logger_.Error("Failed to remove device: %s", esp_err_to_name(ret));
            return false;
        }
    }
    return true;
}

bool SpiDevice::Transfer(const std::vector<uint8_t>& tx_data, std::vector<uint8_t>& rx_data)
{
    if (dev_handle_ == NULL)
    {
        return false;
    }

    spi_transaction_t t;
    std::memset(&t, 0, sizeof(t));

    t.length = tx_data.size() * 8;  // length is in bits
    t.tx_buffer = tx_data.data();

    rx_data.resize(tx_data.size());
    t.rx_buffer = rx_data.data();

    return spi_device_transmit(dev_handle_, &t) == ESP_OK;
}

bool SpiDevice::Write(const std::vector<uint8_t>& data)
{
    if (dev_handle_ == NULL)
    {
        return false;
    }

    spi_transaction_t t;
    std::memset(&t, 0, sizeof(t));

    t.length = data.size() * 8;
    t.tx_buffer = data.data();
    t.rx_buffer = NULL;  // No receive

    return spi_device_transmit(dev_handle_, &t) == ESP_OK;
}

bool SpiDevice::Read(size_t len, std::vector<uint8_t>& rx_data)
{
    if (dev_handle_ == NULL)
    {
        return false;
    }

    spi_transaction_t t;
    std::memset(&t, 0, sizeof(t));

    t.length = len * 8;
    t.tx_buffer = NULL;

    rx_data.resize(len);
    t.rx_buffer = rx_data.data();

    return spi_device_transmit(dev_handle_, &t) == ESP_OK;
}

bool SpiDevice::WriteByte(uint8_t data)
{
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &data;
    t.rx_buffer = nullptr;
    return spi_device_transmit(dev_handle_, &t) == ESP_OK;
}

bool SpiDevice::ReadByte(uint8_t& data)
{
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = nullptr;
    t.rx_buffer = &data;
    return spi_device_transmit(dev_handle_, &t) == ESP_OK;
}

bool SpiDevice::WriteRegBytes(uint8_t reg_addr, const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> tx;
    tx.reserve(1 + data.size());
    tx.push_back(reg_addr);
    tx.insert(tx.end(), data.begin(), data.end());

    spi_transaction_t t = {};
    t.length = tx.size() * 8;
    t.tx_buffer = tx.data();
    t.rx_buffer = nullptr;
    return spi_device_transmit(dev_handle_, &t) == ESP_OK;
}

bool SpiDevice::ReadRegBytes(uint8_t reg_addr, std::vector<uint8_t>& data, size_t len)
{
    std::vector<uint8_t> tx(1 + len, 0x00);
    tx[0] = reg_addr;
    std::vector<uint8_t> rx(1 + len, 0x00);

    spi_transaction_t t = {};
    t.length = (1 + len) * 8;
    t.tx_buffer = tx.data();
    t.rx_buffer = rx.data();

    if (spi_device_transmit(dev_handle_, &t) != ESP_OK)
    {
        return false;
    }
    data.assign(rx.begin() + 1, rx.end());
    return true;
}

bool SpiDevice::WriteReg8(uint8_t reg_addr, uint8_t data)
{
    return WriteRegBytes(reg_addr, {data});
}

bool SpiDevice::ReadReg8(uint8_t reg_addr, uint8_t& data)
{
    std::vector<uint8_t> buf;
    if (!ReadRegBytes(reg_addr, buf, 1))
        return false;
    data = buf[0];
    return true;
}

bool SpiDevice::WriteReg16(uint8_t reg_addr, uint16_t data)
{
    return WriteRegBytes(reg_addr,
                         {static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data & 0xFF)});
}

bool SpiDevice::ReadReg16(uint8_t reg_addr, uint16_t& data)
{
    std::vector<uint8_t> buf;
    if (!ReadRegBytes(reg_addr, buf, 2))
        return false;
    data = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
    return true;
}

bool SpiDevice::WriteReg32(uint8_t reg_addr, uint32_t data)
{
    return WriteRegBytes(reg_addr,
                         {static_cast<uint8_t>(data >> 24), static_cast<uint8_t>(data >> 16),
                          static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data & 0xFF)});
}

bool SpiDevice::ReadReg32(uint8_t reg_addr, uint32_t& data)
{
    std::vector<uint8_t> buf;
    if (!ReadRegBytes(reg_addr, buf, 4))
        return false;
    data = (static_cast<uint32_t>(buf[0]) << 24) | (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8) | static_cast<uint32_t>(buf[3]);
    return true;
}

bool SpiDevice::WriteRegBit(uint8_t reg_addr, uint8_t bit, bool value)
{
    uint8_t reg = 0;
    if (!ReadReg8(reg_addr, reg))
        return false;
    if (value)
        reg |= (1u << bit);
    else
        reg &= ~(1u << bit);
    return WriteReg8(reg_addr, reg);
}

bool SpiDevice::ReadRegBit(uint8_t reg_addr, uint8_t bit, bool& value)
{
    uint8_t reg = 0;
    if (!ReadReg8(reg_addr, reg))
        return false;
    value = (reg >> bit) & 0x01;
    return true;
}

bool SpiDevice::WriteRegBits(uint8_t reg_addr, uint8_t mask, uint8_t value)
{
    uint8_t reg = 0;
    if (!ReadReg8(reg_addr, reg))
        return false;
    reg = (reg & ~mask) | (value & mask);
    return WriteReg8(reg_addr, reg);
}

bool SpiDevice::ReadRegBits(uint8_t reg_addr, uint8_t mask, uint8_t& value)
{
    uint8_t reg = 0;
    if (!ReadReg8(reg_addr, reg))
        return false;
    value = reg & mask;
    return true;
}
