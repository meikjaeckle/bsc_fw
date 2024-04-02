// Copyright (c) 2024 Meik Jaeckle
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#ifndef HEADER_70BD43F6E23D41239F3E530DA2C0BDA1
#define HEADER_70BD43F6E23D41239F3E530DA2C0BDA1

#include <cstdint>
#include <inverter/DataAdapter.hpp>

namespace inverter
{

/**
 * @brief Interface for the inverter control module.
 */
struct Inverter
{
  virtual ~Inverter() = default;

  virtual const DataAdapter& getDataAdapter() const = 0;

  virtual void setChargeCurrentToZero(bool val) = 0;
  virtual void setDischargeCurrentToZero(bool val) = 0;
  virtual void setSocToFull(bool val) = 0;
};

} // namespace inverter

#endif // HEADER_70BD43F6E23D41239F3E530DA2C0BDA1