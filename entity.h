#pragma once

#include "anim.h"
#include "model.h"

struct rid
{
  int32_t Value;
};

struct entity
{
  Anim::transform Transform;
	rid             ModelID;
  int32_t*        MaterialIndices;

  Anim::animation_controller* AnimController;
};
