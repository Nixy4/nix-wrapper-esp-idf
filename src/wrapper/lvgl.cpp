#include "wrapper/lvgl.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace wrapper;

LvglPort::LvglPort(Logger& logger)
    : logger_(logger), lvgl_display_(NULL), lvgl_touch_(NULL), initialized_(false)
{
}

LvglPort::~LvglPort() { Deinit(); }

bool LvglPort::Init(const LvglPortConfig& config)
{
    if (initialized_)
    {
        logger_.Warning("LVGL port already initialized");
        return true;
    }

    esp_err_t ret = lvgl_port_init(&config);
    if (ret != ESP_OK)
    {
        logger_.Error("Failed to initialize LVGL port: %s", esp_err_to_name(ret));
        return false;
    }

    initialized_ = true;
    logger_.Info("LVGL port initialized");
    return true;
}

bool LvglPort::Deinit()
{
    if (lvgl_touch_ != NULL)
    {
        lvgl_port_remove_touch(lvgl_touch_);
        lvgl_touch_ = NULL;
    }

    if (lvgl_display_ != NULL)
    {
        lvgl_port_remove_disp(lvgl_display_);
        lvgl_display_ = NULL;
    }

    if (initialized_)
    {
        esp_err_t ret = lvgl_port_deinit();
        if (ret != ESP_OK)
        {
            logger_.Error("Failed to deinitialize LVGL port: %s", esp_err_to_name(ret));
            return false;
        }
        initialized_ = false;
        logger_.Info("LVGL port deinitialized");

        // Give LVGL worker task a moment to exit cleanly
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return true;
}

bool LvglPort::AddDisplay(const DisplayBase& display, LvglDisplayConfig& config)
{
    if (!initialized_)
    {
        logger_.Error("LVGL port not initialized");
        return false;
    }

    if (lvgl_display_ != NULL)
    {
        logger_.Warning("Display already added. Removing existing display first.");
        lvgl_port_remove_disp(lvgl_display_);
        lvgl_display_ = NULL;
    }

    // LvglDisplayConfig final_config = config;
    config.io_handle = display.GetIoHandle();
    config.panel_handle = display.GetPanelHandle();

    lvgl_display_ = lvgl_port_add_disp(&config);
    if (lvgl_display_ == NULL)
    {
        logger_.Error("Failed to add LVGL display");
        return false;
    }

    logger_.Info("LVGL display added");

    return true;
}

bool LvglPort::AddDisplayDsi(const DisplayBase& display,
                             LvglDisplayConfig& config,
                             const LvglDisplayDsiConfig& dsi_config)
{
    if (!initialized_)
    {
        logger_.Error("LVGL port not initialized");
        return false;
    }

    if (lvgl_display_ != NULL)
    {
        logger_.Warning("Display already added. Removing existing display first.");
        lvgl_port_remove_disp(lvgl_display_);
        lvgl_display_ = NULL;
    }

    config.io_handle = display.GetIoHandle();
    config.panel_handle = display.GetPanelHandle();

    lvgl_display_ = lvgl_port_add_disp_dsi(&config, &dsi_config);
    if (lvgl_display_ == NULL)
    {
        logger_.Error("Failed to add LVGL DSI display");
        return false;
    }

    logger_.Info("LVGL DSI display added");
    return true;
}

bool LvglPort::AddTouch(const I2cTouch& touch, LvglTouchConfig& config)
{
    if (!initialized_)
    {
        logger_.Error("LVGL port not initialized");
        return false;
    }

    if (lvgl_display_ == NULL)
    {
        logger_.Error("Display must be added before touch");
        return false;
    }

    if (lvgl_touch_ != NULL)
    {
        logger_.Warning("Touch already added. Removing existing touch first.");
        lvgl_port_remove_touch(lvgl_touch_);
        lvgl_touch_ = NULL;
    }

    config.disp = lvgl_display_;
    config.handle = touch.GetHandle();

    lvgl_touch_ = lvgl_port_add_touch(&config);
    if (lvgl_touch_ == NULL)
    {
        logger_.Error("Failed to add LVGL touch");
        return false;
    }

    logger_.Info("LVGL touch added");
    return true;
}

bool LvglPort::Lock(uint32_t timeout_ms) { return lvgl_port_lock(timeout_ms); }

void LvglPort::Unlock() { lvgl_port_unlock(); }

void LvglPort::Stop() { lvgl_port_stop(); }

void LvglPort::Resume() { lvgl_port_resume(); }

void LvglPort::Wake(lvgl_port_event_type_t event, void* param)
{
    lvgl_port_task_wake(event, param);
}

void LvglPort::Test(bool is_monochrome)
{
    logger_.Info("LVGL Functional Test Start (%s mode)", is_monochrome ? "Monochrome" : "Color");

    if (!Lock(0))
    {
        logger_.Error("Failed to acquire LVGL lock");
        return;
    }

    lv_obj_t* scr = lv_scr_act();
    if (!scr)
    {
        logger_.Error("No active screen!");
        Unlock();
        return;
    }

    // Get display resolution
    int32_t hor_res = lv_display_get_horizontal_resolution(lvgl_display_);
    int32_t ver_res = lv_display_get_vertical_resolution(lvgl_display_);
    logger_.Info("Display resolution: %dx%d", hor_res, ver_res);

    // Calculate scale factor based on minimum dimension (relative to 128px baseline)
    int32_t min_dim = (hor_res < ver_res) ? hor_res : ver_res;

    // 1. Clear screen and remove default padding/border
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_border_width(scr, 0, 0);

    if (is_monochrome)
    {
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    else
    {
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x1F1F1F), LV_PART_MAIN);  // Dark gray
    }

    // 2. Create Title Label (top 10% of screen)
    lv_obj_t* label = lv_label_create(scr);
    if (label)
    {
        lv_label_set_text(label, is_monochrome ? "MONO TEST" : "COLOR TEST");
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);

        // Font selection based on screen size
        if (min_dim >= 128)
        {
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        }
        // else use default font

        int32_t label_y = (ver_res * 5) / 100;  // 5% from top
        if (label_y < 2)
            label_y = 2;
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, label_y);
        logger_.Info("Test label created");
    }

    if (!is_monochrome)
    {
        // 3. Color Specific: RGB Bars (centered, 60% width, 15% height)
        lv_obj_t* container = lv_obj_create(scr);
        int32_t cont_w = (hor_res * 60) / 100;
        int32_t cont_h = (ver_res * 15) / 100;
        if (cont_h < 24)
            cont_h = 24;  // Minimum height for visibility
        if (cont_w < 60)
            cont_w = 60;  // Minimum width

        lv_obj_set_size(container, cont_w, cont_h);
        lv_obj_align(container, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(container, 0, 0);
        lv_obj_set_style_border_opa(container, 0, 0);
        lv_obj_set_style_pad_all(container, 0, 0);
        lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);

        static lv_color_t colors[] = {lv_palette_main(LV_PALETTE_RED),
                                      lv_palette_main(LV_PALETTE_GREEN),
                                      lv_palette_main(LV_PALETTE_BLUE)};
        int32_t rect_size = (cont_h * 80) / 100;  // 80% of container height
        if (rect_size < 16)
            rect_size = 16;
        int32_t spacing = (cont_w - 3 * rect_size) / 4;
        if (spacing < 4)
            spacing = 4;
        int32_t radius = (rect_size * 15) / 100;  // 15% radius
        if (radius < 2)
            radius = 2;

        for (int i = 0; i < 3; i++)
        {
            lv_obj_t* rect = lv_obj_create(container);
            lv_obj_set_size(rect, rect_size, rect_size);
            lv_obj_set_style_bg_color(rect, colors[i], 0);
            lv_obj_set_style_border_width(rect, 0, 0);
            lv_obj_set_style_radius(rect, radius, 0);
            lv_obj_align(rect, LV_ALIGN_LEFT_MID, i * (rect_size + spacing) + spacing, 0);
        }
        logger_.Info("RGB bars created (size: %d, spacing: %d)", rect_size, spacing);
    }
    else
    {
        // 3. Mono Specific: White Frame (centered, 50% width, 25% height)
        lv_obj_t* frame = lv_obj_create(scr);
        int32_t frame_w = (hor_res * 50) / 100;
        int32_t frame_h = (ver_res * 25) / 100;
        if (frame_w < 40)
            frame_w = 40;
        if (frame_h < 20)
            frame_h = 20;

        lv_obj_set_size(frame, frame_w, frame_h);
        lv_obj_align(frame, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(frame, 0, 0);
        lv_obj_set_style_border_color(frame, lv_color_hex(0xFFFFFF), 0);

        int32_t border_w = (min_dim * 2) / 128;
        if (border_w < 1)
            border_w = 1;
        if (border_w > 4)
            border_w = 4;
        lv_obj_set_style_border_width(frame, border_w, 0);
        lv_obj_set_style_pad_all(frame, 0, 0);
        logger_.Info("White frame created (%dx%d, border: %d)", frame_w, frame_h, border_w);
    }

    // 4. Create Spinner (bottom 20% of screen)
    lv_obj_t* spinner = lv_spinner_create(scr);
    if (spinner)
    {
        lv_spinner_set_anim_params(spinner, 1000, 60);

        // Spinner size: 15% of minimum dimension
        int32_t spin_size = (min_dim * 15) / 100;
        if (spin_size < 20)
            spin_size = 20;  // Minimum size for visibility
        if (spin_size > 100)
            spin_size = 100;  // Maximum size

        lv_obj_set_size(spinner, spin_size, spin_size);

        int32_t spin_y = -(ver_res * 5) / 100;  // 5% from bottom
        if (spin_y > -2)
            spin_y = -2;
        lv_obj_align(spinner, LV_ALIGN_BOTTOM_MID, 0, spin_y);

        // Arc width: proportional to spinner size
        int32_t arc_w = (spin_size * 12) / 100;  // 12% of spinner size
        if (arc_w < 2)
            arc_w = 2;
        if (arc_w > 10)
            arc_w = 10;

        if (is_monochrome)
        {
            lv_obj_set_style_arc_color(spinner, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(spinner, arc_w, LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(spinner, arc_w, LV_PART_MAIN);
        }
        else
        {
            lv_obj_set_style_arc_color(spinner, lv_palette_main(LV_PALETTE_ORANGE),
                                       LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(spinner, arc_w, LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(spinner, arc_w, LV_PART_MAIN);
            lv_obj_set_style_arc_color(spinner, lv_palette_lighten(LV_PALETTE_GREY, 1),
                                       LV_PART_MAIN);
        }
        logger_.Info("Test spinner created (size: %d, arc_width: %d)", spin_size, arc_w);
    }

    Unlock();
    logger_.Info("LVGL Functional Test Complete");
}

bool LvglPort::SetRotation(lv_display_rotation_t rotation)
{
    if (!Lock(0))
    {
        logger_.Error("Failed to acquire LVGL lock");
        return false;
    }

    if (lvgl_display_ == NULL)
    {
        logger_.Error("Display must be added before setting rotation");
        Unlock();
        return false;
    }

    // if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
    // {
    //     logger_.Error("Invalid rotation value. Must be 0, 90, 180, or 270 degrees");
    //     Unlock();
    //     return false;
    // }

    lv_disp_set_rotation(lvgl_display_, rotation);
    Unlock();
    logger_.Info("Display rotation set to %d degrees", rotation);
    return true;
}
