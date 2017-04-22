#pragma once

#include "anim.h"
#include "model.h"

struct entity
{
  Anim::transform Transform;
  Render::model*  Model;
  int32_t*        MaterialIndices;

  Anim::animation_controller* AnimController;
};

