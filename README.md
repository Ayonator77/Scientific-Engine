# Scientific Engine (C++ / OpenGL 4.6)

> A real-time procedural planet generator and GPU-accelerated fluid simulator, built from scratch.

I'm Ayodeji. I built this engine because I'm fascinated by the intersection of mathematics, graphics programming, and low-level systems work. Rather than using Unity or Unreal, I wanted to build the rendering pipeline from the ground up — from GLSL shaders and GPU buffer management to procedural geometry, compute-shader physics, and a custom editor.

---

## What It Does

The engine generates procedural planets in real-time and simulates fluid dynamics using Smoothed Particle Hydrodynamics (SPH) entirely on the GPU, with an interactive editor for tweaking all parameters live.

### Procedural Planet Generation

Planets start as an icosahedron (12 vertices, 20 faces) whose vertices are derived from three mutually perpendicular golden rectangles using the golden ratio φ = (1 + √5) / 2. The mesh is subdivided iteratively — each triangle is split into four by inserting edge midpoints — with an edge cache to prevent duplicate vertices. All midpoints are re-projected onto the unit sphere. After `n` subdivisions the mesh contains 20 × 4ⁿ triangles.

### Terrain Noise

Topography is sculpted using **Ridged Multifractal noise** built on 3D Simplex noise. Each octave applies a ridge transform (`(1 − |noise|)²`) with weight feedback from the previous octave, producing sharp mountain ridges that suppress detail in lowlands. Frequency doubles and amplitude halves per octave (lacunarity = 2, gain = 0.5). A sea-level clamp flattens ocean floors. All generation is seeded via a deterministic offset in the noise sample space, so a given seed reproduces the same planet.

### GPU Fluid Simulation (SPH)

A Smoothed Particle Hydrodynamics solver runs entirely in two OpenGL 4.6 compute shader passes per sub-step:

1. **Density & Pressure** — computes per-particle density using the Poly6 kernel (W = 315/64πh⁹ · (h² − r²)³), then derives pressure from a linearized equation of state (p = k(ρ − ρ₀)).
2. **Forces & Integration** — accumulates pressure gradient forces (Spiky kernel gradient), viscosity forces (viscosity kernel Laplacian), and gravity, then integrates via semi-implicit Euler with sub-stepping and artificial damping. Boundary collisions use axis-aligned box clamping with configurable restitution.

Particles are stored in a single SSBO that is simultaneously bound as a compute storage buffer (binding 0) and a vertex attribute array — zero-copy rendering with no CPU–GPU transfer.

### Rendering

- **Planet**: Lambertian diffuse shading with multiple point lights using inverse-square attenuation. Normals are transformed via the inverse-transpose of the model matrix.
- **Lights**: Rendered as point-sprite billboards with soft glow falloff (smoothstep) and a selection ring.
- **Particles**: Point sprites with density-mapped coloring (deep blue → cyan) and perspective-correct size scaling (pointSize ∝ 1/viewZ).

### Interaction

- **Orbit camera** using spherical coordinates (radius, pitch, yaw) with scroll-to-zoom and pitch clamping.
- **3D light picking** via analytic ray-sphere intersection (ray constructed by unprojecting NDC through inverse projection and view matrices).
- **Light dragging** in the camera's view plane using extracted right/up vectors from the view matrix.

### Editor

An integrated Dear ImGui (docking branch) panel provides real-time telemetry (FPS, frame times), mesh stats, fluid simulation parameter sliders (smoothing radius, target density, pressure, viscosity, gravity, wall damping), planet regeneration controls, light management (add, delete, color, intensity, position), and a GPU state diagnostic dump.

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

## Architecture

```
Application          — owns SDL/GL context, main loop, frame timing
├── GraphicsContext   — SDL window + OpenGL 4.6 core profile init
├── Input            — per-frame keyboard/mouse state polling
├── Scene            — camera, lights, planet, SPH solver
│   ├── Camera       — spherical orbit camera
│   ├── Icosahedron  — procedural mesh (subdivide, noise, normals, GL buffers)
│   ├── SphSolver    — GPU compute dispatch, SSBO management, sub-stepping
│   └── PointLight[] — position, color, intensity, selection state
├── Renderer         — shader programs, draw calls (planet, lights, particles)
└── Editor           — ImGui lifecycle, dockspace, panels, output flags
```

The editor communicates with the application through a plain `EditorOutput` struct (flags for planet regen, sim reset, debug log), keeping UI completely decoupled from engine objects.

---

## Directory Structure

```
├── assets/shaders/
│   ├── planet.vert / planet.frag    — icosahedron rendering (Lambertian diffuse)
│   ├── light.vert / light.frag      — point-sprite light billboards
│   ├── particle.vert / particle.frag — SPH particle rendering
│   ├── sph_density.comp             — compute: Poly6 density + equation of state
│   └── sph_forces.comp              — compute: Spiky pressure + viscosity + Euler integration
├── include/
│   ├── core/       — Application, Camera, Shader, ComputeShader, Renderer, Scene, Input, GraphicsContext
│   ├── geometry/   — Icosahedron
│   ├── physics/    — Particle, SphParams, SphSolver
│   └── ui/         — Editor
├── src/            — implementations mirroring include/ layout
└── tests/          — Google Test suite
```

---

## Building

**Prerequisites:** Visual Studio 2022+ (MSVC), CMake ≥ 3.20, Git, vcpkg

```
git clone https://github.com/Ayonator77/Scientific-Engine.git
cd Scientific-Engine
.\run.bat
```

The build system uses vcpkg for dependency management (SDL2, GLAD, GLM, spdlog, GTest) and fetches ImGui's docking branch via CMake FetchContent. No prebuilt binaries are stored in the repo. `run.bat` auto-configures on first run and builds/executes in Debug mode.

---

## Project Status

### Working

- Procedural planet generation with seeded, deterministic output
- Icosahedral subdivision with edge midpoint caching
- Ridged Multifractal noise terrain displacement with domain-varied seed offsets
- Smooth vertex normals (area-weighted accumulation)
- Multi-light Lambertian diffuse shading with inverse-square attenuation
- Point-sprite light billboards with selection ring and soft glow
- 3D ray-cast light picking and view-plane dragging
- GPU SPH fluid simulation (Poly6 density, Spiky pressure gradient, viscosity Laplacian)
- Semi-implicit Euler integration with sub-stepping and artificial damping
- Zero-copy SSBO → vertex attribute particle rendering
- Density-mapped particle coloring with perspective-correct point sizes
- ImGui docking editor with planet, lights, simulation, and stats panels
- GPU VRAM diagnostic dump to file

### In Progress

- Extracting rendering, input, and planet construction into dedicated subsystems
- Unit tests for geometry math, subdivision, and deterministic seed output
- Shader loader hardening (fail-fast on missing files)

### Planned

- Spatial hashing for O(n) neighbor search (replacing O(n²) brute force)
- N-Body orbital mechanics (Verlet/RK4 integration)
- General relativity visualization experiments

---

## License