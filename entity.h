#pragma once

#include "anim.h"
#include "model.h"
#include "rid.h"
#include "rigid_body.h"

struct entity
{
  Anim::transform Transform;
  rigid_body      RigidBody;

  rid  ModelID;
  rid* MaterialIDs;

  Anim::animation_controller* AnimController;
};
