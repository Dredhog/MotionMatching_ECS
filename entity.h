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
  rid*            MaterialIDs;

  Anim::animation_controller* AnimController;
};
