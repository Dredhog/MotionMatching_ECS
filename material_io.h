#pragma once

#include "resource_manager.h"
#include "render_data.h"

material ImportMaterial(Resource::resource_manager* Resources, const char* Path);
void     ExportMaterial(const material* Material, const char* Directory, const char* Name);
