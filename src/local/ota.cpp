#include "local/ota.hpp"

#include <cinttypes>
#include <cstring>

#include "esp_app_desc.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"

using namespace wrapper;

// ─── helpers ─────────────────────────────────────────────────────────────────

namespace
{

esp_https_ota_config_t BuildHttpsConfig(const HttpsOtaConfig& cfg)
{
    esp_http_client_config_t http_cfg = {};
    http_cfg.url = cfg.url;
    http_cfg.cert_pem = cfg.ca_cert;
    http_cfg.client_cert_pem = cfg.client_cert;
    http_cfg.client_key_pem = cfg.client_key;
    if (!cfg.ca_cert)
    {
        http_cfg.skip_cert_common_name_check = true;
    }

    esp_https_ota_config_t ota_cfg = {};
    ota_cfg.http_config = new esp_http_client_config_t(http_cfg);
    ota_cfg.bulk_flash_erase = cfg.bulk_flash_erase;
    return ota_cfg;
}

void FreeHttpsConfig(esp_https_ota_config_t& ota_cfg)
{
    delete ota_cfg.http_config;
    ota_cfg.http_config = nullptr;
}

}  // namespace

// ─── OtaManager ──────────────────────────────────────────────────────────────

OtaManager::OtaManager(Logger& logger) : logger_(logger) {}

OtaManager::~OtaManager()
{
    if (session_active_)
    {
        logger_.Warning("OTA session still active during destruction; aborting");
        Abort();
    }
    if (https_session_active_)
    {
        logger_.Warning("HTTPS OTA session still active during destruction; aborting");
        HttpsAbort();
    }
}

// ─── Raw OTA session ─────────────────────────────────────────────────────────

bool OtaManager::Begin(size_t image_size)
{
    if (session_active_)
    {
        logger_.Error("OTA session already active; call End() or Abort() first");
        return false;
    }

    update_partition_ = esp_ota_get_next_update_partition(nullptr);
    if (!update_partition_)
    {
        logger_.Error("No OTA update partition available");
        return false;
    }

    logger_.Info("Beginning OTA to partition '%s' (offset=0x%" PRIx32 " size=0x%" PRIx32 ")",
                 update_partition_->label,
                 update_partition_->address,
                 update_partition_->size);

    const esp_err_t err = esp_ota_begin(update_partition_, image_size, &ota_handle_);
    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_begin failed (err=0x%" PRIx32 ")", static_cast<uint32_t>(err));
        update_partition_ = nullptr;
        return false;
    }

    session_active_ = true;
    return true;
}

bool OtaManager::Write(const void* data, size_t size)
{
    if (!session_active_)
    {
        logger_.Error("No active OTA session; call Begin() first");
        return false;
    }

    const esp_err_t err = esp_ota_write(ota_handle_, data, size);
    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_write failed (err=0x%" PRIx32 ")", static_cast<uint32_t>(err));
        return false;
    }

    return true;
}

bool OtaManager::End()
{
    if (!session_active_)
    {
        logger_.Error("No active OTA session; call Begin() first");
        return false;
    }

    const esp_err_t err = esp_ota_end(ota_handle_);
    session_active_ = false;
    ota_handle_ = 0;

    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_end failed (err=0x%" PRIx32 ")", static_cast<uint32_t>(err));
        update_partition_ = nullptr;
        return false;
    }

    logger_.Info("OTA image written and validated on partition '%s'",
                 update_partition_->label);
    return true;
}

bool OtaManager::Abort()
{
    if (!session_active_)
    {
        return true;
    }

    const esp_err_t err = esp_ota_abort(ota_handle_);
    session_active_ = false;
    ota_handle_ = 0;
    update_partition_ = nullptr;

    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_abort failed (err=0x%" PRIx32 ")", static_cast<uint32_t>(err));
        return false;
    }

    logger_.Info("OTA session aborted");
    return true;
}

bool OtaManager::IsSessionActive() const { return session_active_; }

const esp_partition_t* OtaManager::GetUpdatePartition() const { return update_partition_; }

// ─── HTTPS OTA ───────────────────────────────────────────────────────────────

bool OtaManager::HttpsUpdate(const HttpsOtaConfig& config)
{
    if (!config.url)
    {
        logger_.Error("HttpsUpdate: url is null");
        return false;
    }
    if (https_session_active_)
    {
        logger_.Error("HTTPS OTA session already active");
        return false;
    }

    logger_.Info("Starting HTTPS OTA from: %s", config.url);

    esp_https_ota_config_t ota_cfg = BuildHttpsConfig(config);
    const esp_err_t err = esp_https_ota(&ota_cfg);
    FreeHttpsConfig(ota_cfg);

    if (err != ESP_OK)
    {
        logger_.Error("esp_https_ota failed (err=0x%" PRIx32 ")", static_cast<uint32_t>(err));
        return false;
    }

    logger_.Info("HTTPS OTA complete; call esp_restart() to boot new firmware");
    return true;
}

bool OtaManager::HttpsBegin(const HttpsOtaConfig& config)
{
    if (!config.url)
    {
        logger_.Error("HttpsBegin: url is null");
        return false;
    }
    if (https_session_active_)
    {
        logger_.Error("HTTPS OTA session already active");
        return false;
    }

    logger_.Info("Opening HTTPS OTA session: %s", config.url);

    esp_https_ota_config_t ota_cfg = BuildHttpsConfig(config);
    esp_https_ota_handle_t handle = nullptr;
    const esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    FreeHttpsConfig(ota_cfg);

    if (err != ESP_OK)
    {
        logger_.Error("esp_https_ota_begin failed (err=0x%" PRIx32 ")", static_cast<uint32_t>(err));
        return false;
    }

    https_handle_ = handle;
    https_session_active_ = true;
    return true;
}

bool OtaManager::HttpsPerform(bool& done)
{
    done = false;

    if (!https_session_active_ || !https_handle_)
    {
        logger_.Error("No active HTTPS OTA session; call HttpsBegin() first");
        return false;
    }

    const esp_err_t err =
        esp_https_ota_perform(static_cast<esp_https_ota_handle_t>(https_handle_));

    if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS)
    {
        return true;  // still in progress, not an error
    }

    if (err == ESP_OK)
    {
        done = true;
        return true;
    }

    logger_.Error("esp_https_ota_perform failed (err=0x%" PRIx32 ")", static_cast<uint32_t>(err));
    return false;
}

bool OtaManager::HttpsFinish()
{
    if (!https_session_active_ || !https_handle_)
    {
        logger_.Error("No active HTTPS OTA session; call HttpsBegin() first");
        return false;
    }

    const esp_err_t err =
        esp_https_ota_finish(static_cast<esp_https_ota_handle_t>(https_handle_));
    https_handle_ = nullptr;
    https_session_active_ = false;

    if (err != ESP_OK)
    {
        logger_.Error("esp_https_ota_finish failed (err=0x%" PRIx32 ")", static_cast<uint32_t>(err));
        return false;
    }

    logger_.Info("HTTPS OTA finished; call esp_restart() to boot new firmware");
    return true;
}

bool OtaManager::HttpsAbort()
{
    if (!https_session_active_ || !https_handle_)
    {
        return true;
    }

    const esp_err_t err =
        esp_https_ota_abort(static_cast<esp_https_ota_handle_t>(https_handle_));
    https_handle_ = nullptr;
    https_session_active_ = false;

    if (err != ESP_OK)
    {
        logger_.Error("esp_https_ota_abort failed (err=0x%" PRIx32 ")", static_cast<uint32_t>(err));
        return false;
    }

    logger_.Info("HTTPS OTA session aborted");
    return true;
}

bool OtaManager::IsHttpsSessionActive() const { return https_session_active_; }

bool OtaManager::HttpsGetImageDesc(esp_app_desc_t& desc)
{
    if (!https_session_active_ || !https_handle_)
    {
        logger_.Error("No active HTTPS OTA session; call HttpsBegin() first");
        return false;
    }

    const esp_err_t err = esp_https_ota_get_img_desc(
        static_cast<esp_https_ota_handle_t>(https_handle_), &desc);
    if (err != ESP_OK)
    {
        logger_.Error("esp_https_ota_get_img_desc failed (err=0x%" PRIx32 ")",
                      static_cast<uint32_t>(err));
        return false;
    }

    return true;
}

int OtaManager::HttpsGetImageLenRead() const
{
    if (!https_session_active_ || !https_handle_)
    {
        return -1;
    }
    return esp_https_ota_get_image_len_read(
        static_cast<esp_https_ota_handle_t>(https_handle_));
}

// ─── Partition queries ────────────────────────────────────────────────────────

const esp_partition_t* OtaManager::GetRunningPartition() const
{
    return esp_ota_get_running_partition();
}

const esp_partition_t* OtaManager::GetBootPartition() const
{
    return esp_ota_get_boot_partition();
}

const esp_partition_t* OtaManager::GetNextUpdatePartition() const
{
    return esp_ota_get_next_update_partition(nullptr);
}

bool OtaManager::SetBootPartition(const esp_partition_t* partition)
{
    if (!partition)
    {
        logger_.Error("SetBootPartition: partition is null");
        return false;
    }

    const esp_err_t err = esp_ota_set_boot_partition(partition);
    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_set_boot_partition failed for '%s' (err=0x%" PRIx32 ")",
                      partition->label,
                      static_cast<uint32_t>(err));
        return false;
    }

    logger_.Info("Boot partition set to '%s'", partition->label);
    return true;
}

bool OtaManager::GetPartitionAppDesc(const esp_partition_t* partition, esp_app_desc_t& desc) const
{
    if (!partition)
    {
        logger_.Error("GetPartitionAppDesc: partition is null");
        return false;
    }

    const esp_err_t err = esp_ota_get_partition_description(partition, &desc);
    if (err != ESP_OK)
    {
        logger_.Error(
            "esp_ota_get_partition_description failed for '%s' (err=0x%" PRIx32 ")",
            partition->label,
            static_cast<uint32_t>(err));
        return false;
    }

    return true;
}

bool OtaManager::GetRunningAppDesc(esp_app_desc_t& desc) const
{
    return GetPartitionAppDesc(esp_ota_get_running_partition(), desc);
}

uint8_t OtaManager::GetAppPartitionCount() const
{
    return esp_ota_get_app_partition_count();
}

// ─── Rollback / validity ──────────────────────────────────────────────────────

bool OtaManager::MarkAppValid()
{
    const esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_mark_app_valid_cancel_rollback failed (err=0x%" PRIx32 ")",
                      static_cast<uint32_t>(err));
        return false;
    }

    logger_.Info("Running app marked as valid; rollback cancelled");
    return true;
}

bool OtaManager::MarkAppInvalid()
{
    const esp_err_t err = esp_ota_mark_app_invalid_rollback();
    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_mark_app_invalid_rollback failed (err=0x%" PRIx32 ")",
                      static_cast<uint32_t>(err));
        return false;
    }

    logger_.Info("Running app marked as invalid; rollback will occur on next restart");
    return true;
}

bool OtaManager::GetPartitionState(const esp_partition_t* partition,
                                   esp_ota_img_states_t& state) const
{
    if (!partition)
    {
        logger_.Error("GetPartitionState: partition is null");
        return false;
    }

    const esp_err_t err = esp_ota_get_state_partition(partition, &state);
    if (err != ESP_OK)
    {
        logger_.Error("esp_ota_get_state_partition failed for '%s' (err=0x%" PRIx32 ")",
                      partition->label,
                      static_cast<uint32_t>(err));
        return false;
    }

    return true;
}

bool OtaManager::IsRollbackPossible() const
{
    return esp_ota_check_rollback_is_possible();
}
