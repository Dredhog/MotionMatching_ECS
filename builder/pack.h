#pragma once

void
PackModel(Render::model* Model)
{
	uint64_t Base = (uint64_t)Model;
  for(int i = 0; i < Model->MeshCount; i++)
  {
		Render::mesh *Mesh = Model->Meshes[i];

		Mesh->Vertices = (Render::vertex*)((uint64_t)Mesh->Vertices - Base);
		Mesh->Indices = (uint32_t*)((uint64_t)Mesh->Indices - Base);
		
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

		Render::mesh *Mesh = Model->Meshes[i];

		Mesh->Vertices = (Render::vertex*)((uint64_t)Mesh->Vertices + Base);
		Mesh->Indices = (uint32_t*)((uint64_t)Mesh->Indices + Base);
  }
}
