#pragma once


#define ASSET_HEADER_CHECKSUM 123456

void
PackModel(Render::model* Model)
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
UnpackModel(Render::model* Model)
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
PackAsset(Asset::asset_file_header* Header, int32_t TotalAssetSize)
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
UnpackAsset(Asset::asset_file_header* Header)
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
