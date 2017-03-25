#pragma once

#include <stdint.h>

namespace Asset
{
  enum asset_type
  {
    ASSET_Model,
    ASSET_Actor,
  };

  struct asset_file_header
  {
    uint32_t Checksum;
    uint32_t AssetType;
    uint32_t TotalSize;
    uint64_t Model;
    uint64_t Skeleton;
  };
}
