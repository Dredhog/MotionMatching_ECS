#pragma once

#include "anim.h"
#include "model.h"
#include "rid.h"
#include "rigid_body.h"

struct entity
{
  transform  Transform;
  rigid_body RigidBody; //Mainly needed for serialization (currently is copied in and out before before and after every physics update

  rid  ModelID;
  rid* MaterialIDs;

  Anim::animation_player* AnimPlayer;
};
