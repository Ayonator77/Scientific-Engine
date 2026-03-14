# Scientific Engine (C++ / OpenGL 4.6)

> A real-time procedural planet generator and GPU-accelerated fluid simulator, built from scratch.

I'm Ayodeji. I built this engine because I'm fascinated by the intersection of mathematics, graphics programming, and low-level systems work. Rather than using Unity or Unreal, I wanted to build the rendering pipeline from the ground up — from GLSL shaders and GPU buffer management to procedural geometry, compute-based physics, and a custom editor.

---

## What It Does

The engine generates procedural planets and runs a Smoothed Particle Hydrodynamics (SPH) fluid simulation on the GPU, all in real-time with an interactive editor.

### Procedural Planet Generation

Planets start as an icosahedron defined by the golden ratio φ = (1+√5)/2. The 12 base vertices are subdivided using edge-midpoint caching — each pass splits every triangle into four by inserting normalized midpoints on each edge, projecting them onto the unit sphere. At subdivision level 6, this produces 81,920 triangles and 40,962 vertices.

Terrain is sculpted using **Ridged Multifractal Noise**: 3D Simplex noise is passed through a ridge transform `(1 - |noise|)²` and layered across multiple octaves with frequency doubling and amplitude halving. A weight-feedback loop makes higher elevations accumulate more detail, producing realistic mountain-on-mountain terrain. All generation is seeded for deterministic reproduction. A sea-level floor flattens low elevations into oceans.

### GPU Fluid Simulation (SPH)

A Smoothed Particle Hydrodynamics solver runs entirely on the GPU via OpenGL 4.6 compute shaders. Up to 8,000+ particles simulate incompressible fluid dynamics in a two-pass pipeline:

1. **Density & Pressure Pass** (`sph_density.comp`) — computes per-particle density using the Poly6 smoothing kernel, then derives pressure from the Tait equation of state.
2. **Forces & Integration Pass** (`sph_forces.comp`) — computes pressure gradient forces (Spiky kernel), viscosity forces (viscosity kernel Laplacian), and gravity. Integrates using semi-implicit Euler with velocity damping and box-boundary collision.

The particle buffer lives in a single SSBO that is dual-bound as both a compute storage buffer and a vertex attribute buffer — the vertex shader reads directly from the same VRAM the compute shaders write to, with zero CPU readback.

### Lighting & Shading

Lambertian diffuse shading with inverse-square attenuation supports up to 8 dynamic point lights. Lights are rendered as GPU point sprites with circular billboard fragment shading and selection-ring overlays. Lights are pickable via mouse ray-casting (ray-sphere intersection in NDC → world space) and draggable along the camera's view-plane axes.

### Particle Visualization

Particles are rendered as point sprites with density-mapped coloring — low-pressure regions render deep blue, high-pressure regions render white-cyan. Point size scales inversely with camera distance for depth perception.

### Editor

An integrated Dear ImGui (docking branch) editor provides:
- Planet controls: seed, subdivision level, noise amplitude/frequency/octaves, sea level
- Fluid controls: particle count, smoothing radius, target density, pressure, viscosity, gravity, wall damping
- Light management: add/remove/select lights, drag position, color picker, intensity
- Stats bar: FPS, particle count, OpenGL version
- GPU diagnostic: dumps full VRAM particle state (position, velocity, density, pressure) to file for debugging

---

## Architecture

```
Application (main loop owner)
├── GraphicsContext     — SDL2 window + OpenGL 4.6 core context
├── Input              — SDL keyboard/mouse polling
├── Renderer           — stateless draw calls (planet, lights, particles)
├── Editor             — ImGui lifecycle + all UI panels
└── Scene              — owns all world state
    ├── Camera         — orbital (spherical coordinates)
    ├── Icosahedron    — procedural planet mesh
    ├── PointLight[]   — dynamic lights with ray-pick selection
    └── SphSolver      — GPU fluid simulation
        ├── sph_density.comp   — Poly6 density + equation of state
        └── sph_forces.comp    — Spiky pressure + viscosity + Euler integration
```

The frame loop executes four phases: **Input → Physics → Render → UI**. The `Scene` owns all simulation state; the `Renderer` is stateless and receives everything it needs per call. GPU resources (VAOs, SSBOs, shader programs) are protected by deleted copy constructors — only move semantics are permitted.

---

## Tech Stack

| Component     | Choice                                |
|---------------|---------------------------------------|
| Language      | C++20                                 |
| Graphics API  | OpenGL 4.6 (Core Profile)             |
| GPU Compute   | OpenGL Compute Shaders (GLSL 460)     |
| Windowing     | SDL2                                  |
| Math          | GLM                                   |
| UI            | Dear ImGui (Docking Branch)           |
| Build         | CMake + vcpkg                         |
| Testing       | Google Test                           |
| Logging       | spdlog                                |

---

## Directory Structure

```
├── assets/shaders/         # GLSL shaders (vertex, fragment, compute)
│   ├── planet.vert/frag    # Planet mesh rendering + Lambertian lighting
│   ├── light.vert/frag     # Point sprite light billboards
│   ├── particle.vert/frag  # SPH particle rendering (density-mapped color)
│   ├── sph_density.comp    # Compute: Poly6 density + pressure EOS
│   └── sph_forces.comp     # Compute: Spiky/viscosity forces + Euler integration
├── include/
│   ├── core/               # Application, Camera, Shader, ComputeShader, Renderer, etc.
│   ├── geometry/           # Icosahedron (procedural mesh)
│   ├── physics/            # Particle, SphParams, SphSolver
│   └── ui/                 # Editor (ImGui panels)
├── src/                    # Implementation files (mirrors include/ layout)
└── tests/                  # Google Test suite
```

---

## Building

**Prerequisites:** Visual Studio 2022+ (MSVC), CMake ≥ 3.20, Git, vcpkg

```bash
git clone https://github.com/Ayonator77/Scientific-Engine.git
cd Scientific-Engine
.\run.bat
```

The build system uses vcpkg for dependency management (SDL2, GLAD, GLM, spdlog, Google Test) and fetches ImGui's docking branch via CMake's FetchContent. No prebuilt binaries are stored in the repo.

**Manual build:**
```bash
cmake --preset msvc
cmake --build build/vs2026 --config Debug --target run
```

---

## SPH Math Reference

The fluid simulation implements the Navier-Stokes equations discretized over particles:

**Density estimation** (Poly6 kernel):
```
ρᵢ = Σⱼ mⱼ · W_poly6(‖rᵢ - rⱼ‖, h)
W_poly6(r, h) = (315 / 64πh⁹) · (h² - r²)³    for r < h
```

**Pressure** (Tait equation of state):
```
pᵢ = k · (ρᵢ - ρ₀)
```

**Pressure force** (Spiky kernel gradient):
```
Fᵢ_pressure = -Σⱼ mⱼ · (pᵢ + pⱼ)/(2ρⱼ) · ∇W_spiky
∇W_spiky(r, h) = -(45/πh⁶) · (h - r)² · (r̂/r)
```

**Viscosity force** (Laplacian kernel):
```
Fᵢ_visc = μ · Σⱼ mⱼ · (vⱼ - vᵢ)/ρⱼ · ∇²W_visc
∇²W_visc(r, h) = (45/πh⁶) · (h - r)
```

**Integration** (semi-implicit Euler with damping):
```
v += a · dt
v *= 0.995          (energy bleed for stability)
x += v · dt
```

---

## Project Status

This is an active prototype. The planetary generation pipeline, SPH fluid simulation, and editor are functional.

**What's working:**
- Procedural planet generation with seeded, deterministic output
- Icosahedral subdivision with edge caching (up to 327k triangles)
- Ridged multifractal noise terrain with weight-feedback octaves
- GPU-accelerated SPH fluid simulation (two-pass compute pipeline)
- Dual SSBO/VBO binding for zero-copy particle rendering
- Density-mapped particle coloring with distance-scaled point sprites
- Lambertian diffuse shading with up to 8 dynamic point lights
- Interactive point lights with ray-cast picking and drag movement
- ImGui docking editor with planet, fluid, and light controls
- GPU VRAM diagnostic dump for debugging simulation instability

**What's in progress:**
- Spatial hashing for O(N) neighbor search (currently O(N²) brute force)
- Elevation-based biome fragment shading
- Shader loader hardening (fail-fast on missing files)

**What's planned:**
- N-Body orbital mechanics (Verlet/RK4 integration)
- General relativity visualization experiments
- Particle LOD and adaptive timestep

---

## License