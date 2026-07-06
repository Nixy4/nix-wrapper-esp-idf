#pragma once

#include <cstdint>
#include <string>

#include "esp_app_desc.h"
#include "esp_flash_partitions.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

#include "local/logger.hpp"

namespace wrapper
{

// ─── HttpsOtaConfig ──────────────────────────────────────────────────────────
//
// Simplified configuration for HTTPS OTA updates.
// For advanced use-cases (client certificates, partial download, etc.)
// use HttpsBegin() / HttpsPerform() / HttpsFinish() directly.

struct HttpsOtaConfig
{
    const char* url = nullptr;
    const char* ca_cert = nullptr;      // PEM-encoded CA certificate (nullptr = skip verify)
    const char* client_cert = nullptr;  // PEM-encoded client certificate (mTLS)
    const char* client_key = nullptr;   // PEM-encoded client private key (mTLS)
    bool bulk_flash_erase = false;      // Erase entire partition before write

    HttpsOtaConfig() = default;
    explicit HttpsOtaConfig(const char* url, const char* ca_cert = nullptr)
        : url(url), ca_cert(ca_cert)
    {
    }
};

// ─── OtaManager ──────────────────────────────────────────────────────────────
//
// Wraps ESP-IDF OTA update operations in two groups:
//
//   1. Raw OTA session  – Begin() / Write() / End() / Abort()
//      Manages a single flash-write session to the next OTA partition.
//
//   2. HTTPS OTA        – HttpsUpdate() (one-shot) or
//                         HttpsBegin() / HttpsPerform() / HttpsFinish() (stepped)
//      Wraps esp_https_ota for network-based firmware updates.
//
//   3. Partition / rollback helpers – partition queries, app description reading,
//      rollback validity control.

class OtaManager
{
    Logger& logger_;

    // Raw OTA session state
    esp_ota_handle_t ota_handle_ = 0;
    const esp_partition_t* update_partition_ = nullptr;
    bool session_active_ = false;

    // HTTPS OTA session state
    void* https_handle_ = nullptr;  // esp_https_ota_handle_t (opaque to header)
    bool https_session_active_ = false;

   public:
    explicit OtaManager(Logger& logger);
    ~OtaManager();

    // ── Raw OTA session ──────────────────────────────────────────────────────

    // Begin a raw OTA session targeting the next available OTA partition.
    // image_size may be OTA_SIZE_UNKNOWN (0xFFFFFFFF) to erase on-the-fly.
    bool Begin(size_t image_size = OTA_SIZE_UNKNOWN);

    // Write a chunk of firmware image data to the active OTA partition.
    bool Write(const void* data, size_t size);

    // Finalise the OTA session and validate the written image.
    // On success the boot partition is NOT changed — call SetBootPartition() or
    // let the caller invoke esp_restart() after SetBootPartition().
    bool End();

    // Abort the active OTA session and free associated resources.
    bool Abort();

    bool IsSessionActive() const;

    // Return the partition being written in the current raw OTA session.
    // Returns nullptr if no session is active.
    const esp_partition_t* GetUpdatePartition() const;

    // ── HTTPS OTA ────────────────────────────────────────────────────────────

    // One-shot HTTPS OTA update.  Blocks until the update completes or fails.
    // On success call esp_restart() to boot the new firmware.
    bool HttpsUpdate(const HttpsOtaConfig& config);

    // Step 1: Initialise HTTPS OTA session.  Must be called before HttpsPerform.
    bool HttpsBegin(const HttpsOtaConfig& config);

    // Step 2: Read and write one chunk of image data.
    // Returns true when the image has been fully downloaded (done).
    // Returns false on error; check IsHttpsSessionActive() to distinguish
    // "still in progress" from error (ESP_ERR_HTTPS_OTA_IN_PROGRESS is normal).
    bool HttpsPerform(bool& done);

    // Step 3: Finalise and set boot partition to the new image.
    // On success call esp_restart() to boot the new firmware.
    bool HttpsFinish();

    // Abort an in-progress HTTPS OTA session.
    bool HttpsAbort();

    bool IsHttpsSessionActive() const;

    // Read the app description from the image being downloaded via HTTPS.
    // Must be called after HttpsBegin() and before the first HttpsPerform().
    bool HttpsGetImageDesc(esp_app_desc_t& desc);

    // Number of image bytes written so far during an active HTTPS session.
    int HttpsGetImageLenRead() const;

    // ── Partition queries ────────────────────────────────────────────────────

    const esp_partition_t* GetRunningPartition() const;
    const esp_partition_t* GetBootPartition() const;

    // Returns the next candidate OTA partition (the one Begin() would use).
    const esp_partition_t* GetNextUpdatePartition() const;

    // Change the boot partition for next restart.
    bool SetBootPartition(const esp_partition_t* partition);

    // Read the app description embedded in any app partition.
    bool GetPartitionAppDesc(const esp_partition_t* partition, esp_app_desc_t& desc) const;

    // Read the app description of the currently running firmware.
    bool GetRunningAppDesc(esp_app_desc_t& desc) const;

    // Total number of OTA app partitions defined in the partition table.
    uint8_t GetAppPartitionCount() const;

    // ── Rollback / validity ──────────────────────────────────────────────────

    // Mark the running application as valid (cancels a pending rollback).
    // Call this as early as possible after a successful first boot of new firmware.
    bool MarkAppValid();

    // Mark the running application as invalid and trigger a rollback on restart.
    bool MarkAppInvalid();

    // Query the OTA state of any app partition.
    bool GetPartitionState(const esp_partition_t* partition, esp_ota_img_states_t& state) const;

    // True if there is at least one valid OTA app partition to roll back to.
    bool IsRollbackPossible() const;
};

}  // namespace wrapper
