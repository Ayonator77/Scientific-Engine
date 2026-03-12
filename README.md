# Scientific Engine (C++ / OpenGL 4.6)

> A real-time procedural planet generator and 3D rendering sandbox, built from scratch.

I'm Ayodeji. I built this engine because I'm fascinated by the intersection of mathematics, graphics programming, and low-level systems work. Rather than using Unity or Unreal, I wanted to build the rendering pipeline from the ground up — from GLSL shaders and GPU buffer management to procedural geometry and a custom editor.

---

## What It Does Today

The engine generates procedural planets in real-time with an interactive editor for tweaking parameters.

**Procedural Mesh Generation**
Planets start as an icosahedron, subdivided with an edge midpoint cache to produce uniformly distributed vertices. Vertices are normalized onto a unit sphere and displaced by layered noise to create terrain. Normals are recalculated per-face after displacement.

**Terrain Noise**
Topography is sculpted using Ridged Multifractal noise, Domain Warping, and 3D Simplex noise. All generation is seeded, so a given seed reproduces the same planet deterministically.

**Elevation-Based Fragment Shading**
A custom fragment shader assigns biome colors (sand, grass, rock, snow) based on each vertex's distance from the planet core.

**Lighting**
Lambertian diffuse shading with point lights that can be repositioned in 3D space via mouse ray-casting.

**Editor**
An integrated Dear ImGui panel provides real-time telemetry (FPS, frame times), mesh stats (poly count), and controls for planet regeneration with different seeds and noise parameters.

---

## Tech Stack

| Component     | Choice                           |
|---------------|----------------------------------|
| Language      | C++20                            |
| Graphics API  | OpenGL 4.6 (Core Profile)        |
| Windowing     | SDL2                             |
| Math          | GLM                              |
| UI            | Dear ImGui (Docking Branch)      |
| Build         | CMake + vcpkg                    |

---

## Building

**Prerequisites:** Visual Studio 2022 (MSVC), CMake, Git

```
git clone https://github.com/Ayonator77/Scientific-Engine.git
cd Scientific-Engine
.\run.bat
```

The build system uses vcpkg for dependency management and fetches ImGui via CMake's FetchContent. No prebuilt binaries are stored in the repo.

---

## Project Status

This is an active prototype. The planetary generation pipeline and editor are functional. The architecture is being refactored toward a more modular design to support future simulation work.

**What's working:**
- Procedural planet generation with seeded, deterministic output
- Icosahedral subdivision with edge caching
- Real-time noise-based terrain displacement
- Elevation-mapped biome shading
- Interactive point lights with ray-cast picking
- ImGui editor with regeneration controls and telemetry

**What's in progress:**
- Extracting rendering, input, and planet construction into dedicated subsystems (currently consolidated in `Application.cpp`)
- Real unit tests for geometry math, subdivision, and deterministic seed output
- Shader loader hardening (fail-fast on missing files)

**What's planned (not yet implemented):**
- Smoothed Particle Hydrodynamics (SPH) fluid simulation
- N-Body orbital mechanics (Verlet/RK4 integration)
- General relativity visualization experiments

---

## Architecture Notes

The current entry point is `Application`, which owns the SDL/OpenGL context, the render loop, camera, lighting, planet construction, and editor orchestration. This works for the current scope but is being broken apart as new systems are added. See the project issues for refactoring plans.

---

## License

[Add your license here]