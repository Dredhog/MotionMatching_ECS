#pragma once

#include "resource_manager.h"
#include "render_data.h"

material ImportMaterial(Memory::stack_allocator* Allocator, Resource::resource_manager* Resources, const char* Path);
material ImportMaterial(Memory::heap_allocator* Allocator, Resource::resource_manager* Resources, const char* Path);
void
ExportMaterial(Resource::resource_manager* ResourceManager, const material* Material, const char* Path);
