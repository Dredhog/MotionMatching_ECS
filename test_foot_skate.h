#include "anim.h"
#include "blend_stack.h"
#include "debug_drawing.h"

struct foot_skate_data_row
{
	float t;
  float dt;

  float LeftFootHeight;
	float RightFootHeight;

	float LeftFootXVel;
	float LeftFootZVel;
	float RightFootXVel;
	float RightFootZVel;
};


struct foot_skate_test_instance
{
  path             TestAnimPath;
  Anim::animation* Animation;
};

foot_skate_data_row
MeasureFootSkate(const Anim::skeleton* Skeleton, mirror_info* MirrorInfo, blend_stack* BlendStack,
                 Anim::blend_func* BlendFunc, transform LocalDeltaRoot, float t, float dt)
{
	foot_skate_data_row Result = {};
  // Sample Pose
	
  // Sample Future Pose

  // Compute Local Velocities

  // Compute Global Velocity

  // Visualize Velocity Colored By Height
	
	//Debug visualize velocities

  return Result;
}

foot_skate_data_row
MeasureFootSkate(Memory::stack_allocator* TempAlloc, const Anim::skeleton* Skeleton,
                 Anim::animation* Anim, float t, float dt)
{
	foot_skate_data_row Result = {};
	
  // Sample Pose
	mm_frame_info Pose;
  mm_frame_info PoseMirror;
	vec3 OutPoseVelocity = {};
  GetPoseGoal(&Pose, NULL, &OutPoseVelocity, NULL, TempAlloc, Skeleton, Animation, t, dt);
	
  // Compute Global Velocitiies
	for(int i = 0; i < MM_POINT_COUNT; i++)
	{
    Pose->BoneVs[i] += OutPoseVelocity;
  }
	
	//Debug visualize velocities

  return Result;
}
