#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "model.h"
#include "anim.h"
#include "edit_animation.h"
#include "stack_alloc.h"
#include "render_data.h"

#define ASSET_HEADER_CHECKSUM 123456

namespace Asset
{
  void PackModel(Render::model* Model);
  void UnpackModel(Render::model* Model);

  void PackAnimation(Anim::animation* Animation);
  void UnpackAnimation(Anim::animation* Animation);

  void PackAnimationGroup(Anim::animation_group* AnimationGroup);
  void UnpackAnimationGroup(Anim::animation_group* AnimationGroup);

  void ExportAnimationGroup(Memory::stack_allocator*               Alloc,
                            const EditAnimation::animation_editor* AnimEditor, char* FileName);
  void ImportAnimationGroup(Memory::stack_allocator* Alloc, Anim::animation_group** OutputAnimGroup,
                            char* FileName);
}
