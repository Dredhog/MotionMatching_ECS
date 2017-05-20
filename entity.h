#pragma once

#include "anim.h"
#include "model.h"
#include "rid.h"

struct entity
{
  Anim::transform Transform;
  rid             ModelID;
  rid*            MaterialIDs;

  Anim::animation_controller* AnimController;
};
