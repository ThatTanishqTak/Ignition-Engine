# Ignition — System Design

Ignition is a Vulkan-based C++ engine built as a shared library (`Ignition`) that hosts two
applications: the **Editor** (scene authoring) and the **Sandbox** (demo/feature gallery). Both
executables derive from `Ignition::Application` and are driven by the engine's `EntryPoint`.

## System Overview

```mermaid
flowchart TB
    subgraph Apps["Applications (executables)"]
        Editor["Ignition-Editor<br/>EditorApplication · EditorLayer<br/>EditorCameraController · EditorContext"]
        Sandbox["Ignition-Sandbox<br/>SandboxApplication · GalleryLayer<br/>CameraController"]
    end

    subgraph Engine["Ignition-Engine (shared library, IGNITION_API)"]
        Core["Core<br/>Application · EntryPoint<br/>Layer / LayerStack · Log · Time · Profiler"]
        EngineInt["Engine (internal)<br/>owns Window, Input,<br/>Renderer, EventQueue"]

        Window["Window<br/>SDL3 window, clipboard,<br/>cursor, text input"]
        Events["Events<br/>EventQueue · Key / Mouse /<br/>Gamepad / Window events"]
        Input["Input<br/>Input · ActionMap"]

        Renderer["Renderer (public API)<br/>Camera · Mesh · Material<br/>Texture · DebugDraw"]
        Vulkan["Vulkan backend (internal)<br/>Instance · Device · Swapchain<br/>Pipelines · Compute · VMA allocator<br/>LineRenderer · GPU timers"]

        Scene["Scene (ECS)<br/>Scene · Entity · Components<br/>SceneSerializer · ModelImporter"]
        Physics["Physics<br/>PhysicsWorld · PhysicsComponents"]
        Fluid["Fluid<br/>FluidSolver3D<br/>(D3Q19 lattice-Boltzmann)"]
        UI["UI (retained mode)<br/>UIContext · Element · Style<br/>UILayer"]
        Assets["Assets<br/>AssetRegistry"]
    end

    subgraph Vendor["Vendor (third-party)"]
        SDL["SDL3"]
        VK["Vulkan + VMA"]
        Jolt["Jolt Physics"]
        EnTT["entt"]
        Assimp["assimp + stb"]
        Yaml["yaml-cpp"]
        GLM["glm"]
        Spdlog["spdlog"]
        Tracy["tracy"]
        Slang["Slang shaders<br/>Mesh · Fluid3D · FluidVolume · DebugLine"]
    end

    Editor --> Core
    Sandbox --> Core
    Core --> EngineInt

    EngineInt --> Window
    EngineInt --> Events
    EngineInt --> Input
    EngineInt --> Renderer

    Window --> SDL
    Window --> Events
    Events --> Input

    Renderer --> Vulkan
    Vulkan --> VK
    Vulkan --> Slang

    Scene --> EnTT
    Scene --> Physics
    Scene --> Renderer
    Scene --> Assimp
    Scene --> Yaml

    Physics --> Jolt
    Fluid --> Renderer
    UI --> Renderer
    Assets --> Scene

    Core --> Spdlog
    Core --> Tracy
    Engine --> GLM
```

## Runtime Composition

`Application` (public) owns an `ApplicationImplementation` (pimpl) that holds the internal
`Engine` plus the `LayerStack`. The `Engine` owns the platform and rendering objects and pumps
the frame:

```mermaid
sequenceDiagram
    participant App as Application (Editor / Sandbox)
    participant Eng as Engine (internal)
    participant Win as Window (SDL3)
    participant EQ as EventQueue
    participant Layers as LayerStack (UILayer overlay + app layers)
    participant Rend as Renderer → VulkanRenderer

    App->>Eng: Initialize(title, w, h)
    Eng->>Win: create SDL window + surface
    Eng->>Rend: create Vulkan backend (instance, device, swapchain)

    loop Frame
        Eng->>Win: PollEvents()
        Win->>EQ: push Key / Mouse / Gamepad / Window events
        EQ->>Layers: dispatch (overlays first, may consume)
        App->>Layers: OnUpdate(dt) / OnFixedUpdate(step)
        Note over App: fixed step drives PhysicsWorld (Jolt)<br/>and queues FluidSolver3D steps
        Eng->>Rend: BeginFrame()
        Note over Rend: queued fluid compute recorded first,<br/>then scene pass, debug lines, UI
        App->>Layers: OnRender()
        Eng->>Rend: EndFrame() → submit + present
    end

    App->>Eng: Shutdown() (WaitIdle, teardown)
```

## Subsystems

| Subsystem | Public API (`Include/Ignition`) | Internal (`Internal/Ignition`) | Backed by |
|---|---|---|---|
| Core | `Application`, `EntryPoint`, `Layer`/`LayerStack`, `Log`, `Time`, `Profiler` | `Engine`, `ApplicationImplementation`, Tracy/Vulkan profiler glue | spdlog, tracy |
| Window | `Window` (clipboard, cursor shapes, IME text input) | `WindowImplementation` | SDL3 |
| Events | `Event`, `EventQueue`, key/mouse/gamepad/window event types | — | — |
| Input | `Input`, `ActionMap`, cursor modes | `InputImplementation` | SDL3 (via Window events) |
| Renderer | `Renderer`, `Camera`, `Mesh`, `Material`, `Texture`, `DebugDraw` | `VulkanRenderer` + instance/device/swapchain/pipeline/compute/buffer/image/descriptor wrappers, `VulkanLineRenderer`, GPU timers | Vulkan 1.4, VMA, Slang |
| Scene | `Scene`, `Entity`, `Components` (incl. transform hierarchy, aero fields), `SceneSerializer`, `ModelImporter` | `SceneRegistry`, `ModelLoader` | entt, yaml-cpp, assimp, stb |
| Physics | `PhysicsWorld` (sim + query-only mode, raycasts), `PhysicsComponents` | `PhysicsWorldImplementation` | Jolt Physics |
| Fluid | `FluidSolver3D` (D3Q19 LBM wind tunnel, mesh voxelization, ray-marched volume view) | `VulkanFluidSolver3D` (compute passes) | Vulkan compute |
| UI | `UIContext`, `Element`, `Style` (retained-mode tree) | `UILayer` (overlay bridging events + rendering) | Renderer |
| Assets | `AssetRegistry` | — | — |

## Design Notes

- **Public / Internal split.** `Include/` is the exported surface (`IGNITION_API`); `Internal/`
  headers (pimpl implementations, the Vulkan backend, `Engine`) never leak to applications. All
  vendor libraries except glm are linked `PRIVATE` to the engine, so apps only see the Ignition
  API plus glm math types.
- **Layer model.** Applications compose behavior by pushing `Layer`s; overlays (the retained
  `UILayer`) sit above app layers and get first chance at events.
- **GPU-first simulation.** The fluid solver runs entirely in Vulkan compute; steps queued
  during update are recorded by the renderer at the top of the next frame, before the scene
  pass. Its output is visualized by ray-marching the whole lattice.
- **Shaders** are authored in Slang (`Ignition-Engine/Shaders`) and compiled at build time by
  the `IgnitionShaders` target, which Editor and Sandbox depend on.
