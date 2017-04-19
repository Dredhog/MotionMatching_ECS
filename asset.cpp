#include "asset.h"

#define ASSET_HEADER_CHECKSUM 123456

#include "file_io.h"

void
Asset::PackModel(Render::model* Model)
{
  uint64_t Base = (uint64_t)Model;
  for(int i = 0; i < Model->MeshCount; i++)
  {
    Render::mesh* Mesh = Model->Meshes[i];

    Mesh->Vertices = (Render::vertex*)((uint64_t)Mesh->Vertices - Base);
    Mesh->Indices  = (uint32_t*)((uint64_t)Mesh->Indices - Base);

    Model->Meshes[i] = (Render::mesh*)((uint64_t)Model->Meshes[i] - Base);
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
}

void
Asset::PackAnimation(Anim::animation* Animation)
{
  uint64_t Base          = (uint64_t)Animation;
  Animation->Transforms  = (Anim::transform*)((uint64_t)Animation->Transforms - Base);
  Animation->SampleTimes = (float*)((uint64_t)Animation->SampleTimes - Base);
}

void
Asset::PackAnimationGroup(Anim::animation_group* AnimationGroup)
{
  for(int a = 0; a < AnimationGroup->AnimationCount; a++)
  {
    PackAnimation(AnimationGroup->Animations[a]);
  }
}

void
Asset::PackAsset(Asset::asset_file_header* Header, int32_t TotalAssetSize)
{
  uint64_t HeaderBase = (uint64_t)Header;

  PackModel((Render::model*)Header->Model);
  Header->Model = Header->Model - HeaderBase;

  if(Header->Skeleton && Header->AssetType == Asset::ASSET_Actor)
  {
    Header->Skeleton = Header->Skeleton - HeaderBase;
  }

  Header->TotalSize = TotalAssetSize;
  Header->Checksum  = ASSET_HEADER_CHECKSUM;
}

void
Asset::UnpackAsset(Asset::asset_file_header* Header)
{
  assert(Header->Checksum == ASSET_HEADER_CHECKSUM);
  uint64_t HeaderBase = (uint64_t)Header;

  Header->Model = Header->Model + HeaderBase;

  if(Header->AssetType == Asset::ASSET_Actor)
  {
    Header->Skeleton = Header->Skeleton + HeaderBase;
  }

  UnpackModel((Render::model*)Header->Model);
}

void
Asset::ExportAnimationGroup(Memory::stack_allocator*         Alloc,
                            EditAnimation::animation_editor* AnimEditor, char* FileName)
{
  Memory::marker Marker = Alloc->GetMarker();

  Asset::asset_file_header* Header = PushStruct(Alloc, Asset::asset_file_header);
  *Header                          = {};
  Header->AssetType                = Asset::ASSET_AnimationGroup;

  Anim::animation_group* AnimGroup = PushStruct(Alloc, Anim::animation_group);
  Header->AnimationGroup           = (uint64_t)AnimGroup;

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

    AnimGroup->Animations[a]->Transforms =
      PushArray(Alloc, AnimationTransformCount, Anim::transform);
    for(int k = 0; k < KeyframeCount; k++)
    {
      memcpy(&AnimGroup->Animations[a]->Transforms[k * ChannelCount], &AnimEditor->Keyframes[k],
             ChannelCount * sizeof(Anim::transform));
    }
  }
  Asset::PackAnimationGroup(AnimGroup);
  uint64_t HeaderBase    = (uint64_t)Header;
  Header->AnimationGroup = ((uint64_t)Header->AnimationGroup - HeaderBase);

  Header->TotalSize =
    Memory::SafeTruncate_size_t_To_uint32_t(Alloc->GetByteCountAboveMarker(Marker));
  WriteEntireFile(FileName, Header->TotalSize, Header);
}
