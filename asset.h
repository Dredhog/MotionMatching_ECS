#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "model.h"
#include "anim.h"
#include "edit_animation.h"
#include "stack_allocator.h"
#include "render_data.h"

#define ASSET_HEADER_CHECKSUM 123456

namespace Asset
{
  enum asset_type
  {
    ASSET_Model,
    ASSET_Actor,
    ASSET_AnimationGroup,
    ASSET_Scene,
  };

  struct asset_file_header
  {
    uint32_t Checksum;
    uint32_t AssetType;
    uint32_t TotalSize;
    uint64_t Model;
    uint64_t AnimationGroup;
    uint64_t Scene;
  };

  void PackModel(Render::model* Model);
  void UnpackModel(Render::model* Model);

  void PackAnimation(Anim::animation* Animation);
  void UnpackAnimation(Anim::animation* Animation);

  void PackAnimationGroup(Anim::animation_group* AnimationGroup);
  void UnpackAnimationGroup(Anim::animation_group* AnimationGroup);

  void PackAsset(Asset::asset_file_header* Header, int32_t TotalAssetSize);
  void UnpackAsset(Asset::asset_file_header* Header);

  void ExportAnimationGroup(Memory::stack_allocator*               Alloc,
                            const EditAnimation::animation_editor* AnimEditor, char* FileName);
  void ImportAnimationGroup(Memory::stack_allocator* Alloc, Anim::animation_group** OutputAnimGroup,
                            char* FileName);
}
