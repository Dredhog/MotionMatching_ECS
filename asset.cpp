#include "asset.h"

#include "file_io.h"

void Asset::PackModel(Render::model* Model)
{
  uint64_t Base = (uint64_t)Model;
  for(int i = 0; i < Model->MeshCount; i++)
  {
    Render::mesh* Mesh = Model->Meshes[i];

    Mesh->Vertices = (Render::vertex*)((uint64_t)Mesh->Vertices - Base);
    Mesh->Indices  = (uint32_t*)((uint64_t)Mesh->Indices - Base);

    Model->Meshes[i] = (Render::mesh*)((uint64_t)Model->Meshes[i] - Base);
  }
  if(Model->Skeleton)
  {
    Model->Skeleton = (Anim::skeleton*)((uint64_t)Model->Skeleton - Base);
  }
  Model->Meshes = (Render::mesh**)((uint64_t)Model->Meshes - Base);
}

void
Asset::UnpackModel(Render::model* Model)
{
  uint64_t Base = (uint64_t)Model;

  Model->Meshes = (Render::mesh**)((uint64_t)Model->Meshes + Base);

  for(int i = 0; i < Model->MeshCount; i++)
  {
    Model->Meshes[i] = (Render::mesh*)((uint64_t)Model->Meshes[i] + Base);

    Render::mesh* Mesh = Model->Meshes[i];

    Mesh->Vertices = (Render::vertex*)((uint64_t)Mesh->Vertices + Base);
    Mesh->Indices  = (uint32_t*)((uint64_t)Mesh->Indices + Base);
  }
  if(Model->Skeleton)
  {
    Model->Skeleton = (Anim::skeleton*)((uint64_t)Model->Skeleton + Base);
  }
}

void
Asset::PackAnimation(Anim::animation* Animation)
{
  uint64_t Base          = (uint64_t)Animation;
  Animation->Transforms  = (transform*)((uint64_t)Animation->Transforms - Base);
  Animation->SampleTimes = (float*)((uint64_t)Animation->SampleTimes - Base);
}

void
Asset::UnpackAnimation(Anim::animation* Animation)
{
  uint64_t Base          = (uint64_t)Animation;
  Animation->Transforms  = (transform*)((uint64_t)Animation->Transforms + Base);
  Animation->SampleTimes = (float*)((uint64_t)Animation->SampleTimes + Base);
}

void
Asset::PackAnimationGroup(Anim::animation_group* AnimationGroup)
{
  uint64_t Base = (uint64_t)AnimationGroup;
  for(int a = 0; a < AnimationGroup->AnimationCount; a++)
  {
    PackAnimation(AnimationGroup->Animations[a]);
    AnimationGroup->Animations[a] =
      (Anim::animation*)((uint64_t)AnimationGroup->Animations[a] - Base);
  }
  AnimationGroup->Animations = (Anim::animation**)((uint64_t)AnimationGroup->Animations - Base);
}

void
Asset::UnpackAnimationGroup(Anim::animation_group* AnimationGroup)
{
  uint64_t Base              = (uint64_t)AnimationGroup;
  AnimationGroup->Animations = (Anim::animation**)((uint64_t)AnimationGroup->Animations + Base);
  for(int a = 0; a < AnimationGroup->AnimationCount; a++)
  {
    AnimationGroup->Animations[a] =
      (Anim::animation*)((uint64_t)AnimationGroup->Animations[a] + Base);
    UnpackAnimation(AnimationGroup->Animations[a]);
  }
}

void
Asset::ExportAnimationGroup(Memory::stack_allocator*               Alloc,
                            const EditAnimation::animation_editor* AnimEditor, const char* FileName)
{
  Memory::marker Marker = Alloc->GetMarker();

  Anim::animation_group* AnimGroup = PushStruct(Alloc, Anim::animation_group);

  AnimGroup->AnimationCount = 1;
  AnimGroup->Animations     = PushArray(Alloc, AnimGroup->AnimationCount, Anim::animation*);

  for(int a = 0; a < AnimGroup->AnimationCount; a++)
  {
    AnimGroup->Animations[a]                = PushStruct(Alloc, Anim::animation);
    AnimGroup->Animations[a]->ChannelCount  = AnimEditor->Skeleton->BoneCount;
    AnimGroup->Animations[a]->KeyframeCount = AnimEditor->KeyframeCount;

    AnimGroup->Animations[a]->SampleTimes =
      PushArray(Alloc, AnimGroup->Animations[a]->KeyframeCount, float);

    int32_t ChannelCount            = AnimGroup->Animations[a]->ChannelCount;
    int32_t KeyframeCount           = AnimGroup->Animations[a]->KeyframeCount;
    int32_t AnimationTransformCount = ChannelCount * KeyframeCount;

    memcpy(AnimGroup->Animations[a]->SampleTimes, AnimEditor->SampleTimes,
           KeyframeCount * sizeof(float));
    for(int k = 0; k < KeyframeCount; k++)
    {
      AnimGroup->Animations[a]->SampleTimes[k] -= AnimEditor->SampleTimes[0];
    }

    AnimGroup->Animations[a]->Transforms = PushArray(Alloc, AnimationTransformCount, transform);
    for(int k = 0; k < KeyframeCount; k++)
    {
      memcpy(&AnimGroup->Animations[a]->Transforms[k * ChannelCount],
             AnimEditor->Keyframes[k].Transforms, ChannelCount * sizeof(transform));
    }
  }

  int32_t TotalSize =
    Memory::SafeTruncate_size_t_To_uint32_t(Alloc->GetByteCountAboveMarker(Marker));
  PackAnimationGroup(AnimGroup);

	Platform::WriteEntireFile(FileName, TotalSize, AnimGroup);
}

void
Asset::ExportMMParams(const mm_params* Params, const char* FileName)
{
	Platform::WriteEntireFile(FileName, sizeof(mm_params), Params);
}

void
Asset::ImportMMParams(Memory::stack_allocator* Alloc, mm_params* OutParams, const char* FileName)
{
  Memory::marker MemoryStart = Alloc->GetMarker();

  debug_read_file_result ReadFile = Platform::ReadEntireFile(Alloc, FileName);
  assert(ReadFile.Contents && ReadFile.ContentsSize == sizeof(mm_params));
  memcpy(OutParams, ReadFile.Contents, sizeof(mm_params));

	Alloc->FreeToMarker(MemoryStart);

}

#if 0
void
Asset::ImportAnimationGroup(Memory::stack_allocator* Alloc, Anim::animation_group** OutputAnimGroup,
                            char* FileName)
{
  assert(OutputAnimGroup);
  debug_read_file_result AssetReadResult = ReadEntireFile(Alloc, FileName);

  assert(AssetReadResult.Contents);
  Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

  UnpackAsset(AssetHeader);

  *OutputAnimGroup = (Anim::animation_group*)AssetHeader->AnimationGroup;
  assert(*OutputAnimGroup);
}
#endif

/*void
Asset::ImportAnimationGroup(Memory::stack_allocator* Alloc, Anim::animation_group** OutputAnimGroup,
                            const char* FileName)
{
  assert(OutputAnimGroup);
  debug_read_file_result AssetReadResult = Platform::ReadEntireFile(Alloc, FileName);

  assert(AssetReadResult.Contents);
  
  *OutputAnimGroup = (Anim::animation_group*)AssetReadResult.Contents;;
	UnpackAnimationGroup(*OutputAnimGroup);

  assert(*OutputAnimGroup);
}*/
