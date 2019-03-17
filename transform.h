#include "linear_math/vector.h"
#include "linear_math/quaternion.h"

struct transform
{
	quat Translation;
	vec3 Rotation;
  vec3 Scale;
};
