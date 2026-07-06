#pragma once
#include "local/i2c.hpp"
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace wrapper
{

class PowerHub
{
    PowerHub() = default;
    ~PowerHub() = default;

    PowerHub(const PowerHub&) = delete;
    PowerHub& operator=(const PowerHub&) = delete;
    PowerHub(PowerHub&&) = delete;
    PowerHub& operator=(PowerHub&&) = delete;

   public:
    PowerHub& GetInstance()
    {
        static PowerHub instance;
        return instance;
    }
};

}  // namespace wrapper
