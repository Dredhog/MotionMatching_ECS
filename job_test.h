//motion_matching_system.cpp
#include "ecs.h"

class MotionMatchingSystem
{
  struct motion_matching_job
  {
    struct component_data
    {
      const motion_matching* MotionMatching;
      const anim_goal*       AnimGoal;
      animation_controller*  Controller*;
    };

    ECS_JOB_FUNCTION(MotionMatching)
    {
      comopnent_data* ComponentData = (comopnent_data*)Components;
      for(int i = 0; i < Count; i++)
      {
        ComponentData->Controller[i];
        ComponentData->MotionMatching[i];
        ComponentData->AnimGoal[i];
      }
    }
  };
};

//animation_system.cpp
class AnimationSystem
{
  struct component_data
  {
    const anim_goal*      AnimGoal;
    animation_controller* Controller*;
  };

  Memory::tagged_heap_allocator* MatrixPalleteMemory;

  archetype_requirement
  InitJobRequirements()
  {
    archetype_requirement Requirements = {};
    Requirements.Push({ GetComponentID(MMData), REQUIREMENT_Permission_R });
    Requirements.Push({ GetComponentID(COMPONENT_MMData), REQUIREMENT_Permission_R });
    Requirements.Push({ GetComponentID(COMPONENT_MMData) });
  }

  struct matrix_pallete_gen_job
  {
    float dt;

    void PARALLEL_FOR_JOB(BlenAnimJob, anim_controller)
    {
      BEGIN_TIMED_BLOCK(BlendAnimJob);
      for(int i = FirstIndex; i < LastIndex; i++)
      {
        Anim::animation_controller* AnimController = anim_controllers[i];
        MatrixPalleteMemory.Alloc(AnimController->ChannelCount * mat4);
        AnimController->GlobalTimeSec += dt;
      }
    }
  };

  PARALLEL_FOR_JOB(RootMotionJob, Permission_R anim_controller, Permission_RW root_motion_update)
  {
    for(int i = FirstIndex; i < LastIndex; i++)
    {
      const Anim::animation_controller* AnimController   = &anim_controllers[i];
      Anim::translation_rotation*       RootMotionUpdate = &root_motion_updates[i];
      RootMotionUpdate = GetRootMotion(AnimController, AnimController->GlobalTimeSec,
                                       AnimController->GlobalTimeSec + dt);
    }
  }
}

//render_system.cpp
{
  PARALLEL_FOR_JOB(MeshInstanceGeneration, MaterialArray, Model)
  {
    for(int i = FirstIndex; i < LastIndex; i++)
    {
      const material**            Materials        = &MaterialArrays[i];
      Anim::translation_rotation* RootMotionUpdate = &Models[i];
      AnimController->GlobalTimeSec += dt;
    }
  }

  PARALLEL_FOR_JOB(TrajectoryAnimGoalGeneration, AnimController, TrajectoryController, Transform,
                   AnimGoal)
  {
    for(int i = FirstIndex; i < LastIndex; i++)
    {
      const material**            Materials        = &MaterialArrays[i];
      Anim::translation_rotation* RootMotionUpdate = &Models[i];
      GenerateAnimGoal();
    }
  }

  PARALLEL_FOR_JOB(ControllerAnimGoalGeneration, InputController, AnimGoal)
  {
    for(int i = FirstIndex; i < LastIndex; i++)
    {
      const material**            Materials        = &MaterialArrays[i];
      Anim::translation_rotation* RootMotionUpdate = &Models[i];
      AnimController->GlobalTimeSec += dt;
    }
  }
};
