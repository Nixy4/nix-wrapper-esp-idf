#include "axp2101.hpp"

namespace wrapper
{

Axp2101::Axp2101(Logger& logger) : I2cDevice(logger) {}

Axp2101::~Axp2101() {}

}  // namespace wrapper
