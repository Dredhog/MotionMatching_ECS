#pragma once

#include <stdint.h>

namespace Asset
{
  struct asset_file_header
  {
    uint32_t Checksum;
    uint32_t TotalSize;

    uint64_t HeaderOffset;
    uint64_t FixUpTableOffset;
  };
}
