ngpe - None General Purpose Engine
======
A game engine developed bottom up with the goal of learning as much as possible about game engine architecture and the intricacies large real-time sofrware systems. The title serves as a reminder to not overgeneralize when programming (To help remember to not solve nonexistent problems and avoid "futureproofing"). Developed by Lukas Taparauskas (Dredhog) and Rytis Pranckūnas (QuickShift). Repo began as a university project at KTU and has been continued afterwards.

Features
--------
- Rigid body dynamics system (Gauss Seidel constraint solver with convex polyhedral collision detection)
- Custom IMGUI inspired UI
- Renderer (Supports multiple shaders and multiple materials per entity)
- Skeletal animation system with muli animation playback and blending per entity
- Multiple bones per vertex
- Linear and aditive animation blenging 
- In engine skeletal animation creation and editing (Currently UI for this is missing due to UI system revamp)
- In engine material creation/preview/editing
- Asset build system (Models and actors are built into engine format offline from DCC formats importable by Assimp)
- Resource manager (Runtime resource manager tracks references via resource ids for automatic memory managemen of asset memory)
- Scene import/export
- Vector/Matrix/Quaternion math library
- File IO/File Tree search libraries
- Custom heap and stack memory allocators
- Linux/Windows support
- All higher level subsystems reside in platform independent layer
- Debug drawing facilities

Tools
-----
* Language
	c++ - technically, but the style is very reminescent of c, only utilising few c++ features, such as namespaces, operator overloading and anything that is worth the overhead and friction. Avoiding unnecessary abstraction and premature code compression into functions (only if used in multiple places).
* Libraries
  * SDL - Resides in platform dependent layer, easily replacable by fully custom platform library
  * OpenGL
  * Assimp - used only in offline build system for .obj and .dae file parsing

### GUI ###

![Graphical User Interface](docs/ui.png "Graphical User Interface")

### In Engine Material Editing ###

![Material Editor](docs/materials.png "In Engine Material Editing With Preview")

### Rigid Body Dynamics and Debug Drawing ###

![Rigid Body Dynamics](docs/dynamics.png "Stacking Boxes")

