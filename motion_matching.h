#pragma once 

#define MM_POINT_COUNT 10
#define MM_COMPARISON_BONE_COUNT 3
#define MM_MAX_ANIM_COUNT 20

struct animation_set
{
	fixed_stack<rid, MM_MAX_ANIM_COUNT> AnimRIDs;
  Anim::animation* Anims[MM_MAX_ANIM_COUNT];
};

struct animation_goal
{
	vec3 Ps[MM_POINT_COUNT];
	vec3 Dirs[MM_POINT_COUNT];
	vec3 BonePs[MM_COMPARISON_BONE_COUNT];
	vec3 BoneVs[MM_COMPARISON_BONE_COUNT];
	uint64_t Flags;
};
#undef MM_POINT_COUNT
#undef MM_COMPARISON_BONE_COUNT
#undef MM_MAX_ANIM_COUNT
