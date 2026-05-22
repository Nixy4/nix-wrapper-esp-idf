#pragma once
#include <vector>
#include <cstdint>

namespace wrapper
{
struct KeyChart
{
    uint8_t value;
    uint8_t x_1;
    uint8_t x_2;
};

struct Point2D
{
    int8_t x;
    int8_t y;

    bool operator==(const Point2D& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point2D& other) const { return !(*this == other); }
};

struct KeyValue
{
    const char* value_first;
    const int value_num_first;
    const char* value_second;
    const int value_num_second;
};

struct KeyState
{
    bool tab = false;
    bool fn = false;
    bool shift = false;
    bool ctrl = false;
    bool opt = false;
    bool alt = false;
    bool del = false;
    bool enter = false;
    bool space = false;

    std::vector<char> values;
    std::vector<int> hid_key;

    void Reset()
    {
        tab = false;
        fn = false;
        shift = false;
        ctrl = false;
        opt = false;
        alt = false;
        del = false;
        enter = false;
        space = false;

        values.clear();
        hid_key.clear();
    }
};

struct KeyboardConfig
{
    std::vector<int> input_pins;
    std::vector<int> output_pins;
};

class Keyboard
{
   public:
    enum KeyCode
    {
        KEY_NONE = 0x00,
        KEY_ERR_OVF = 0x01,
        KEY_A = 0x04,
        KEY_B = 0x05,
        KEY_C = 0x06,
        KEY_D = 0x07,
        KEY_E = 0x08,
        KEY_F = 0x09,
        KEY_G = 0x0a,
        KEY_H = 0x0b,
        KEY_I = 0x0c,
        KEY_J = 0x0d,
        KEY_K = 0x0e,
        KEY_L = 0x0f,
        KEY_M = 0x10,
        KEY_N = 0x11,
        KEY_O = 0x12,
        KEY_P = 0x13,
        KEY_Q = 0x14,
        KEY_R = 0x15,
        KEY_S = 0x16,
        KEY_T = 0x17,
        KEY_U = 0x18,
        KEY_V = 0x19,
        KEY_W = 0x1a,
        KEY_X = 0x1b,
        KEY_Y = 0x1c,
        KEY_Z = 0x1d,

        KEY_1 = 0x1e,
        KEY_2 = 0x1f,
        KEY_3 = 0x20,
        KEY_4 = 0x21,
        KEY_5 = 0x22,
        KEY_6 = 0x23,
        KEY_7 = 0x24,
        KEY_8 = 0x25,
        KEY_9 = 0x26,
        KEY_0 = 0x27,

        KEY_ENTER = 0x28,
        KEY_ESC = 0x29,
        KEY_BACKSPACE = 0x2a,
        KEY_TAB = 0x2b,
        KEY_SPACE = 0x2c,
        KEY_MINUS = 0x2d,
        KEY_EQUAL = 0x2e,
        KEY_LEFTBRACE = 0x2f,
        KEY_RIGHTBRACE = 0x30,
        KEY_BACKSLASH = 0x31,
        KEY_HASHTILDE = 0x32,
        KEY_SEMICOLON = 0x33,
        KEY_APOSTROPHE = 0x34,
        KEY_GRAVE = 0x35,
        KEY_COMMA = 0x36,
        KEY_DOT = 0x37,
        KEY_SLASH = 0x38,
        KEY_CAPSLOCK = 0x39,

        KEY_F1 = 0x3a,
        KEY_F2 = 0x3b,
        KEY_F3 = 0x3c,
        KEY_F4 = 0x3d,
        KEY_F5 = 0x3e,
        KEY_F6 = 0x3f,
        KEY_F7 = 0x40,
        KEY_F8 = 0x41,
        KEY_F9 = 0x42,
        KEY_F10 = 0x43,
        KEY_F11 = 0x44,
        KEY_F12 = 0x45,

        KEY_PRTSC = 0x46,
        KEY_SCROLLLOCK = 0x47,
        KEY_PAUSE = 0x48,
        KEY_INSERT = 0x49,
        KEY_HOME = 0x4a,
        KEY_PAGEUP = 0x4b,
        KEY_DELETE = 0xD4,
        KEY_END = 0x4d,
        KEY_PAGEDOWN = 0x4e,
        KEY_RIGHT = 0x4f,
        KEY_LEFT = 0x50,
        KEY_DOWN = 0x51,
        KEY_UP = 0x52,

        KEY_NUMLOCK = 0x53,
        KEY_KPSLASH = 0x54,
        KEY_KPASTERISK = 0x55,
        KEY_KPMINUS = 0x56,
        KEY_KPPLUS = 0x57,
        KEY_KPENTER = 0x58,
        KEY_KP1 = 0x59,
        KEY_KP2 = 0x5a,
        KEY_KP3 = 0x5b,
        KEY_KP4 = 0x5c,
        KEY_KP5 = 0x5d,
        KEY_KP6 = 0x5e,
        KEY_KP7 = 0x5f,
        KEY_KP8 = 0x60,
        KEY_KP9 = 0x61,
        KEY_KP0 = 0x62,
        KEY_KPDOT = 0x63,
        KEY_KPLEFTPAREN = 0xb6,
        KEY_KPRIGHTPAREN = 0xb7,

        KEY_LEFTCTRL = 0xe0,
        KEY_LEFTSHIFT = 0xe1,
        KEY_LEFTALT = 0xe2,
        KEY_LEFTMETA = 0xe3,
        KEY_RIGHTCTRL = 0xe4,
        KEY_RIGHTSHIFT = 0xe5,
        KEY_RIGHTALT = 0xe6,
        KEY_RIGHTMETA = 0xe7,

        KEY_MEDIA_PLAYPAUSE = 0xe8,
        KEY_MEDIA_STOPCD = 0xe9,
        KEY_MEDIA_PREVIOUSSONG = 0xea,
        KEY_MEDIA_NEXTSONG = 0xeb,
        KEY_MEDIA_EJECTCD = 0xec,
        KEY_MEDIA_VOLUMEUP = 0xed,
        KEY_MEDIA_VOLUMEDOWN = 0xee,
        KEY_MEDIA_MUTE = 0xef,
        KEY_MEDIA_WWW = 0xf0,
        KEY_MEDIA_BACK = 0xf1,
        KEY_MEDIA_FORWARD = 0xf2,
        KEY_MEDIA_STOP = 0xf3,
        KEY_MEDIA_FIND = 0xf4,
        KEY_MEDIA_SCROLLUP = 0xf5,
        KEY_MEDIA_SCROLLDOWN = 0xf6,
        KEY_MEDIA_EDIT = 0xf7,
        KEY_MEDIA_SLEEP = 0xf8,
        KEY_MEDIA_COFFEE = 0xf9,
        KEY_MEDIA_REFRESH = 0xfa,
        KEY_MEDIA_CALC = 0xfb
    };

   private:
    std::vector<Point2D> key_list_buffer_;
    std::vector<Point2D> last_key_list_buffer_;  // To track changes properly
    std::vector<Point2D> key_values_without_special_keys_;
    KeyState keys_state_buffer_;

    bool is_caps_locked_;

    std::vector<int> output_list_;
    std::vector<int> input_list_;

    static const KeyChart kXMapChart[7];
    static const KeyValue kKeyValueMap[4][14];

    void SetOutput(uint8_t output);
    uint8_t GetInput();

    // Internal update methods
    void UpdateKeyList();
    void UpdateKeysState();

   public:
    Keyboard();
    ~Keyboard() = default;

    void Init(const KeyboardConfig& config);

    // Main update loop
    void Update();

    Point2D GetKey();  // Kept for backward compatibility, but consider deprecating
    uint8_t GetKeyNum(Point2D key_coor) const;

    inline const std::vector<Point2D>& GetKeyList() const { return key_list_buffer_; }
    KeyValue GetKeyValue(const Point2D& key_coor) const;
    bool IsKeyPressing(int key_num) const;

    inline const KeyState& GetKeysState() const { return keys_state_buffer_; }

    inline bool IsCapsLocked(void) const { return is_caps_locked_; }
    inline void SetCapsLocked(bool is_locked) { is_caps_locked_ = is_locked; }

    bool IsChanged();
    inline uint8_t GetPressedCount() const { return key_list_buffer_.size(); }
    inline uint8_t IsPressed() const
    {
        return GetPressedCount();
    }  // Alias for backward compatibility
};
}  // namespace wrapper
