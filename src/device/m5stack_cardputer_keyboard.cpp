#include "m5stack_cardputer_keyboard.hpp"
#include <driver/gpio.h>
#include <cstring>

namespace wrapper
{

const KeyChart Keyboard::kXMapChart[7] = {{1, 0, 1},  {2, 2, 3},    {4, 4, 5},   {8, 6, 7},
                                          {16, 8, 9}, {32, 10, 11}, {64, 12, 13}};

using KeyCode = Keyboard::KeyCode;

const KeyValue Keyboard::kKeyValueMap[4][14] = {
    {{"`", KeyCode::KEY_GRAVE, "~", KeyCode::KEY_GRAVE},
     {"1", KeyCode::KEY_1, "!", KeyCode::KEY_1},
     {"2", KeyCode::KEY_2, "@", KeyCode::KEY_2},
     {"3", KeyCode::KEY_3, "#", KeyCode::KEY_3},
     {"4", KeyCode::KEY_4, "$", KeyCode::KEY_4},
     {"5", KeyCode::KEY_5, "%", KeyCode::KEY_5},
     {"6", KeyCode::KEY_6, "^", KeyCode::KEY_6},
     {"7", KeyCode::KEY_7, "&", KeyCode::KEY_7},
     {"8", KeyCode::KEY_8, "*", KeyCode::KEY_KPASTERISK},
     {"9", KeyCode::KEY_9, "(", KeyCode::KEY_KPLEFTPAREN},
     {"0", KeyCode::KEY_0, ")", KeyCode::KEY_KPRIGHTPAREN},
     {"-", KeyCode::KEY_MINUS, "_", KeyCode::KEY_KPMINUS},
     {"=", KeyCode::KEY_EQUAL, "+", KeyCode::KEY_KPPLUS},
     {"del", KeyCode::KEY_BACKSPACE, "del", KeyCode::KEY_BACKSPACE}},
    {{"tab", KeyCode::KEY_TAB, "tab", KeyCode::KEY_TAB},
     {"q", KeyCode::KEY_Q, "Q", KeyCode::KEY_Q},
     {"w", KeyCode::KEY_W, "W", KeyCode::KEY_W},
     {"e", KeyCode::KEY_E, "E", KeyCode::KEY_E},
     {"r", KeyCode::KEY_R, "R", KeyCode::KEY_R},
     {"t", KeyCode::KEY_T, "T", KeyCode::KEY_T},
     {"y", KeyCode::KEY_Y, "Y", KeyCode::KEY_Y},
     {"u", KeyCode::KEY_U, "U", KeyCode::KEY_U},
     {"i", KeyCode::KEY_I, "I", KeyCode::KEY_I},
     {"o", KeyCode::KEY_O, "O", KeyCode::KEY_O},
     {"p", KeyCode::KEY_P, "P", KeyCode::KEY_P},
     {"[", KeyCode::KEY_LEFTBRACE, "{", KeyCode::KEY_LEFTBRACE},
     {"]", KeyCode::KEY_RIGHTBRACE, "}", KeyCode::KEY_RIGHTBRACE},
     {"\\", KeyCode::KEY_BACKSLASH, "|", KeyCode::KEY_BACKSLASH}},
    {{"fn", KeyCode::KEY_NONE, "fn", KeyCode::KEY_NONE},
     {"shift", KeyCode::KEY_NONE, "shift", KeyCode::KEY_NONE},
     {"a", KeyCode::KEY_A, "A", KeyCode::KEY_A},
     {"s", KeyCode::KEY_S, "S", KeyCode::KEY_S},
     {"d", KeyCode::KEY_D, "D", KeyCode::KEY_D},
     {"f", KeyCode::KEY_F, "F", KeyCode::KEY_F},
     {"g", KeyCode::KEY_G, "G", KeyCode::KEY_G},
     {"h", KeyCode::KEY_H, "H", KeyCode::KEY_H},
     {"j", KeyCode::KEY_J, "J", KeyCode::KEY_J},
     {"k", KeyCode::KEY_K, "K", KeyCode::KEY_K},
     {"l", KeyCode::KEY_L, "L", KeyCode::KEY_L},
     {";", KeyCode::KEY_SEMICOLON, ":", KeyCode::KEY_SEMICOLON},
     {"'", KeyCode::KEY_APOSTROPHE, "\"", KeyCode::KEY_APOSTROPHE},
     {"enter", KeyCode::KEY_ENTER, "enter", KeyCode::KEY_ENTER}},
    {{"ctrl", KeyCode::KEY_LEFTCTRL, "ctrl", KeyCode::KEY_LEFTCTRL},
     {"opt", KeyCode::KEY_NONE, "opt", KeyCode::KEY_NONE},
     {"alt", KeyCode::KEY_LEFTALT, "alt", KeyCode::KEY_LEFTALT},
     {"z", KeyCode::KEY_Z, "Z", KeyCode::KEY_Z},
     {"x", KeyCode::KEY_X, "X", KeyCode::KEY_X},
     {"c", KeyCode::KEY_C, "C", KeyCode::KEY_C},
     {"v", KeyCode::KEY_V, "V", KeyCode::KEY_V},
     {"b", KeyCode::KEY_B, "B", KeyCode::KEY_B},
     {"n", KeyCode::KEY_N, "N", KeyCode::KEY_N},
     {"m", KeyCode::KEY_M, "M", KeyCode::KEY_M},
     {",", KeyCode::KEY_COMMA, "<", KeyCode::KEY_COMMA},
     {".", KeyCode::KEY_DOT, ">", KeyCode::KEY_DOT},
     {"/", KeyCode::KEY_KPSLASH, "?", KeyCode::KEY_KPSLASH},
     {"space", KeyCode::KEY_SPACE, "space", KeyCode::KEY_SPACE}}};

Keyboard::Keyboard() : is_caps_locked_(false) {}

KeyValue Keyboard::GetKeyValue(const Point2D &key_coor) const
{
    return kKeyValueMap[key_coor.y][key_coor.x];
}

void Keyboard::SetOutput(uint8_t output)
{
    if (output_list_.size() < 3)
        return;

    output = output & 0B00000111;

    gpio_set_level((gpio_num_t)output_list_[0], (output & 0B00000001));
    gpio_set_level((gpio_num_t)output_list_[1], (output & 0B00000010));
    gpio_set_level((gpio_num_t)output_list_[2], (output & 0B00000100));
}

uint8_t Keyboard::GetInput()
{
    uint8_t buffer = 0x00;
    uint8_t pin_value = 0x00;

    for (size_t i = 0; i < input_list_.size(); i++)
    {
        pin_value = (gpio_get_level((gpio_num_t)input_list_[i]) == 1) ? 0x00 : 0x01;
        pin_value = pin_value << i;
        buffer = buffer | pin_value;
    }

    return buffer;
}

void Keyboard::Init(const KeyboardConfig &config)
{
    input_list_ = config.input_pins;
    output_list_ = config.output_pins;

    for (auto i : output_list_)
    {
        gpio_reset_pin((gpio_num_t)i);
        gpio_set_direction((gpio_num_t)i, GPIO_MODE_OUTPUT);
        gpio_set_pull_mode((gpio_num_t)i, GPIO_PULLUP_PULLDOWN);
        gpio_set_level((gpio_num_t)i, 0);
    }

    for (auto i : input_list_)
    {
        gpio_reset_pin((gpio_num_t)i);
        gpio_set_direction((gpio_num_t)i, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)i, GPIO_PULLUP_ONLY);
    }

    SetOutput(0);
}

void Keyboard::Update()
{
    // Save previous state for IsChanged()
    last_key_list_buffer_ = key_list_buffer_;

    UpdateKeyList();
    UpdateKeysState();
}

Point2D Keyboard::GetKey()
{
    Point2D coor;
    coor.x = -1;
    coor.y = -1;

    uint8_t input_value = 0;

    for (int i = 0; i < 8; i++)
    {
        SetOutput(i);
        input_value = GetInput();

        /* If key pressed */
        if (input_value)
        {
            /* Get X */
            for (int j = 0; j < 7; j++)
            {
                if (input_value == kXMapChart[j].value)
                {
                    coor.x = (i > 3) ? kXMapChart[j].x_1 : kXMapChart[j].x_2;
                    break;
                }
            }

            /* Get Y */
            coor.y = (i > 3) ? (i - 4) : i;

            /* Keep the same as picture */
            coor.y = -coor.y;
            coor.y = coor.y + 3;

            break;
        }
    }
    return coor;
}

uint8_t Keyboard::GetKeyNum(Point2D key_coor) const
{
    uint8_t ret = 0;

    if ((key_coor.x < 0) || (key_coor.y < 0))
    {
        return 0;
    }

    ret = (key_coor.y * 14) + (key_coor.x + 1);

    return ret;
}

void Keyboard::UpdateKeyList()
{
    key_list_buffer_.clear();

    Point2D coor;
    uint8_t input_value = 0;

    for (int i = 0; i < 8; i++)
    {
        SetOutput(i);
        input_value = GetInput();

        /* If key pressed */
        if (input_value)
        {
            /* Get X */
            for (int j = 0; j < 7; j++)
            {
                if (input_value & (0x01 << j))
                {
                    coor.x = (i > 3) ? kXMapChart[j].x_1 : kXMapChart[j].x_2;

                    /* Get Y */
                    coor.y = (i > 3) ? (i - 4) : i;

                    /* Keep the same as picture */
                    coor.y = -coor.y;
                    coor.y = coor.y + 3;

                    key_list_buffer_.push_back(coor);
                }
            }
        }
    }
}

bool Keyboard::IsKeyPressing(int key_num) const
{
    if (key_list_buffer_.size())
    {
        for (const auto &i : key_list_buffer_)
        {
            if (GetKeyNum(i) == key_num)
                return true;
        }
    }
    return false;
}

void Keyboard::UpdateKeysState()
{
    keys_state_buffer_.Reset();
    key_values_without_special_keys_.clear();

    // Get special keys
    for (auto &i : key_list_buffer_)
    {
        int key_code = GetKeyValue(i).value_num_first;
        const char *key_str =
            GetKeyValue(i).value_first;  // Fallback for fn/opt which have 0 key_code

        if (key_code == KeyCode::KEY_TAB)
        {
            keys_state_buffer_.tab = true;
            continue;
        }
        if (key_code == KeyCode::KEY_NONE && strcmp(key_str, "fn") == 0)
        {
            keys_state_buffer_.fn = true;
            continue;
        }
        if (key_code == KeyCode::KEY_NONE && strcmp(key_str, "shift") == 0)
        {
            keys_state_buffer_.shift = true;
            continue;
        }
        if (key_code == KeyCode::KEY_LEFTCTRL)
        {
            keys_state_buffer_.ctrl = true;
            continue;
        }
        if (key_code == KeyCode::KEY_NONE && strcmp(key_str, "opt") == 0)
        {
            keys_state_buffer_.opt = true;
            continue;
        }
        if (key_code == KeyCode::KEY_LEFTALT)
        {
            keys_state_buffer_.alt = true;
            continue;
        }
        if (key_code == KeyCode::KEY_BACKSPACE)
        {
            keys_state_buffer_.del = true;
            keys_state_buffer_.hid_key.push_back(KeyCode::KEY_BACKSPACE);
            continue;
        }
        if (key_code == KeyCode::KEY_ENTER)
        {
            keys_state_buffer_.enter = true;
            keys_state_buffer_.hid_key.push_back(KeyCode::KEY_ENTER);
            continue;
        }
        if (key_code == KeyCode::KEY_SPACE)
        {
            keys_state_buffer_.space = true;
            keys_state_buffer_.hid_key.push_back(KeyCode::KEY_SPACE);
            continue;
        }

        key_values_without_special_keys_.push_back(i);
    }

    // Deal what left
    for (auto &i : key_values_without_special_keys_)
    {
        if (keys_state_buffer_.ctrl || keys_state_buffer_.shift || is_caps_locked_)
        {
            keys_state_buffer_.values.push_back(*GetKeyValue(i).value_second);
            keys_state_buffer_.hid_key.push_back(GetKeyValue(i).value_num_second);
        }
        else
        {
            keys_state_buffer_.values.push_back(*GetKeyValue(i).value_first);
            keys_state_buffer_.hid_key.push_back(GetKeyValue(i).value_num_first);
        }
    }
}

bool Keyboard::IsChanged()
{
    if (last_key_list_buffer_.size() != key_list_buffer_.size())
    {
        return true;
    }

    // Sizes are equal, check content
    // Assuming order might differ? UpdateKeyList always scans 0..7 so order should be deterministic
    // based on hardware scan. If I hold A then B, vs B then A, the order depends on their position
    // in matrix. So simple vector comparison should work if we assume matrix scan order is fixed.

    for (size_t i = 0; i < key_list_buffer_.size(); ++i)
    {
        if (last_key_list_buffer_[i] != key_list_buffer_[i])
        {
            return true;
        }
    }

    return false;
}

}  // namespace wrapper
