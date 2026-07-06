#pragma once

#include "esp_lvgl_port.h"
#include "wrapper/display.hpp"
#include "registry/touch.hpp"
#include "wrapper/logger.hpp"
#include "wrapper/encoder.hpp"

namespace wrapper
{

struct LvglPortConfig : public lvgl_port_cfg_t
{
    LvglPortConfig(int task_prio,
                   int stack_sz,
                   int affinity,
                   int max_sleep_ms,
                   uint32_t stack_caps,
                   uint32_t timer_ms)
        : lvgl_port_cfg_t{}
    {
        task_priority = task_prio;
        task_stack = stack_sz;
        task_affinity = affinity;
        task_max_sleep_ms = max_sleep_ms;
        task_stack_caps = stack_caps;
        timer_period_ms = timer_ms;
    }
};

struct LvglDisplayConfig : public lvgl_port_display_cfg_t
{
    LvglDisplayConfig(
        // esp_lcd_panel_io_handle_t io_handle,
        // esp_lcd_panel_handle_t panel_handle,
        // esp_lcd_panel_handle_t control_handle,
        uint32_t buf_sz,
        bool double_buf,
        uint32_t trans_sz,
        uint32_t hor_res,
        uint32_t ver_res,
        bool mono,
        bool swap_xy,
        bool mirror_x,
        bool mirror_y,
        lv_color_format_t format,
        bool buff_dma,
        bool buff_spiram,
        bool sw_rotate,
        bool swap_bytes,
        bool full_refresh,
        bool direct_mode)
        : lvgl_port_display_cfg_t{}
    {
        // io_handle = io_handle;
        // panel_handle = panel_handle;
        // control_handle = control_handle;
        io_handle = nullptr;
        panel_handle = nullptr;
        control_handle = nullptr;
        buffer_size = buf_sz;
        double_buffer = double_buf;
        trans_size = trans_sz;
        hres = hor_res;
        vres = ver_res;
        monochrome = mono;
        rotation.swap_xy = swap_xy;
        rotation.mirror_x = mirror_x;
        rotation.mirror_y = mirror_y;
        color_format = format;
        flags.buff_dma = buff_dma;
        flags.buff_spiram = buff_spiram;
        flags.sw_rotate = sw_rotate;
        flags.swap_bytes = swap_bytes;
        flags.full_refresh = full_refresh;
        flags.direct_mode = direct_mode;
    }
};

struct LvglDisplayDsiConfig : public lvgl_port_display_dsi_cfg_t
{
    LvglDisplayDsiConfig(bool avoid_tearing = false) : lvgl_port_display_dsi_cfg_t{}
    {
        flags.avoid_tearing = avoid_tearing;
    }
};

struct LvglTouchConfig : public lvgl_port_touch_cfg_t
{
    LvglTouchConfig(float scale_x = 0.0f, float scale_y = 0.0f) : lvgl_port_touch_cfg_t{}
    {
        disp = nullptr;
        handle = nullptr;
        scale.x = scale_x;
        scale.y = scale_y;
    }
};

class LvglPort
{
    Logger& logger_;
    lv_display_t* lvgl_display_;
    lv_indev_t* lvgl_touch_;
    lv_indev_t* lvgl_encoder_;  ///< 编码器输入设备（可为 nullptr）
    lv_group_t* lvgl_group_;    ///< 编码器使用的 LVGL 焦点组
    bool initialized_;

   public:
    LvglPort(Logger& logger);
    ~LvglPort();

    bool IsInitialized() const { return initialized_; }
    lv_display_t* GetDisplay() const { return lvgl_display_; }
    lv_indev_t* GetTouch() const { return lvgl_touch_; }
    lv_indev_t* GetEncoder() const { return lvgl_encoder_; }
    lv_group_t* GetGroup() const { return lvgl_group_; }

    // operations
    bool Init(const LvglPortConfig& config);
    bool Deinit();
    bool AddDisplay(const DisplayBase& display, LvglDisplayConfig& config);
    bool AddDisplayDsi(const DisplayBase& display,
                       LvglDisplayConfig& config,
                       const LvglDisplayDsiConfig& dsi_config);
    bool AddTouch(const I2cTouch& touch, LvglTouchConfig& config);
    /**
     * @brief 将旋转编码器注册为 LVGL 编码器输入设备。
     *
     * 同时创建 LVGL 焦点组并设置为全局默认组，
     * 使所有新建的可聚焦控件自动加入该组。
     *
     * @param enc  已初始化的 Encoder 实例引用
     * @return true 成功；false 失败
     */
    bool AddEncoder(Encoder& enc);
    bool Lock(uint32_t timeout_ms);
    void Unlock();
    void Stop();
    void Resume();
    void Wake(lvgl_port_event_type_t event, void* pram);
    bool SetRotation(lv_display_rotation_t rotation);
    void Test(bool is_monochrome = false);
};
}  // namespace wrapper
