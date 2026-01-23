# Game Engine Architecture
Core Philosophy
> "The engine should know absolutely nothing about implementation details of any particular game. It's an orchestration layer strictly."
This engine is the fusion of three traditions:
- Data-Oriented Design: Cache-friendly layouts, separation of data and behavior
- Functional Programming: Pure transforms over data, immutability where practical
- Test-Driven Development: Systems are pure functions → trivially testable
The Insight
> "DOD eliminates the bad parts of OOP (deep hierarchies, hidden state, pointer chasing) which are exactly the parts that make FP and TDD hard."
Carmack brought Haskell ideas back to C++ at id Software. This architecture is that same fusion.
---
C++ Style: "Simple C++" / "C with Classes"
Use C++ for:
- Function overloading (actually useful)
- Namespaces (organization without the mess)
- References (cleaner than pointer soup)
- Simple constructors/destructors (RAII for resources, not philosophy)
- Simple templates for containers only
- Operator overloading for math types only (Vec3, Matrix4)
- struct with methods (data + functions that operate on it)
Avoid:
- Inheritance hierarchies (composition instead)
- Virtual functions (unless genuinely needed - rare)
- Template metaprogramming
- STL complexity
- Exception handling
- Smart pointers everywhere (use surgically)
- "Modern C++" features that add cognitive overhead
Key principle: Data is inert. Functions operate on data. Keep it visible, keep it simple.
---
ECS Mental Model
Entities
Just identifiers. An entity is an index, nothing more:
typedef uint32_t Entity;
Components
Components are opaque blobs. The engine stores bytes by ID - it doesn't know or care about internal structure.
A component might be:
- A packed array of Vec3 (common pattern)
- A single Box2D world handle (singleton)
- A spatial hash map (custom structure)  
- Just an int (game state)
The engine's view:
```cpp
uint32_t world_define_component(WorldHandle* handle, const char* name, size_t size);
void* world_get_component(WorldHandle* handle, uint32_t id);
void world_resize_component(WorldHandle* handle, uint32_t id, size_t new_size);
```
No sizeof(Vec3). Just "I need N bytes." The defining plugin knows what those bytes mean.
Helper libraries for common patterns (packed arrays, etc.) are optional conveniences, not engine-mandated structure.
Component Identification
- String names for human coordination ("Position", "Velocity")
- Integer IDs for runtime access (fast lookup)
- Convention: component "Position" exports symbol POSITION_ID
- Registration at plugin init, cached for hot path
Systems
Two layers with different purposes:
**Internal (Pure Functional):**
```cpp
// Takes World by value, returns transformed World
// Ownership flows through the pipeline
using TickFn = World (*)(World);
using FrameFn = World (*)(World, float alpha);
// Engine composes them
for (auto& system : systems_) {
    world_ = system.tick(std::move(world_));
}
```
**Plugin Boundary (C ABI):**
```cpp
// Mutation through opaque handle - C compatible
extern "C" void physics_tick(WorldHandle* handle);
extern "C" void render_frame(WorldHandle* handle, float alpha);
```
The engine wraps plugin functions to present them as pure transforms internally.
The type system enforces the tick/frame distinction:
- Tick functions can't see alpha (no temptation to use it incorrectly)
- Frame functions get alpha (clear this is for interpolation)
The State Transform
> "Systems are just functions in a pipeline. This is literally (reduce apply-system initial-state all-systems) but in C++."
```cpp
// Internal representation - pure functional
world = input_system(std::move(world));
world = physics_system(std::move(world));
world = collision_system(std::move(world));
```
---
The Minimal Engine Core
The engine has exactly three responsibilities:
1. Load systems from config - read load order, dlopen plugins
2. Execute the timing loop - fixed timestep with catch-up
3. Provide plugin infrastructure - component registration, entity management
That's it. No game-specific logic. No rendering knowledge. No physics knowledge.
Game Loop (Fixed Timestep with Interpolation)
void engine_run(Engine* engine, Clock* clock, float tick_rate) {
    float prev = clock->get_time(clock->context);
    float accumulator = 0.0f;
    const float tick_duration = 1.0f / tick_rate;
    
    while (engine->running) {
        float current = clock->get_time(clock->context);
        accumulator += (current - prev);
        prev = current;
        
        // Fixed updates - deterministic, run N times to catch up
        while (accumulator >= tick_duration) {
            for (int index = 0; index < engine->systems.count; index++) {
                if (engine->systems[index].tick_fn) {
                    engine->systems[index].tick_fn(&engine->world);
                }
            }
            accumulator -= tick_duration;
        }
        
        // Frame updates - once per frame with interpolation
        float alpha = accumulator / tick_duration;
        for (int index = 0; index < engine->systems.count; index++) {
            if (engine->systems[index].frame_fn) {
                engine->systems[index].frame_fn(&engine->world, alpha);
            }
        }
    }
}
Alpha for Interpolation
Alpha tells frame systems how far between the last tick and the next tick we are:
// Last tick: position = 10
// This tick: position = 20  
// Alpha = 0.3 (30% of the way to next tick)
// Render at: lerp(10, 20, 0.3) = 13
This gives buttery smooth output even when tick rate differs from frame rate.
Timing Behavior
When frame updates are slow (GPU-bound):
- Fixed updates continue at correct speed ✓ (deterministic simulation)
- Frame updates stutter ✗ (but simulation plays correctly)
When frame updates are fast (high refresh rate):
- Interpolation gives buttery smooth output ✓
- This is where the pattern shines
When fixed updates are slow (CPU-bound):
- Spiral of death - accumulator grows faster than you can drain it
- Fix: Limit catch-up iterations and reset accumulator
---
Dynamic Plugin Architecture
> "Systems and mods SHOULD be the same thing."
Why Plugins Over Scripting
Traditional game engine split:
- C++ core (fast, compiled, for engineers)
- Scripts (slow, interpreted, for designers/modders)
Rejected because:
1. Performance overhead
2. Artificial complexity (two languages, FFI boundaries)
3. "Why not expect the same excellence across the board?" (Jonathan Blow)
Instead: Everything is native C++. Extension = write system, compile to .so/.dll, drop in plugins folder.
Plugin Interface
Two-layer architecture for ABI safety:
**Layer 1: C ABI at .so boundary**
```cpp
// cask/abi.h - stable C interface
typedef void (*PluginTickFn)(WorldHandle* handle);
typedef void (*PluginFrameFn)(WorldHandle* handle, float alpha);
typedef void (*PluginInitFn)(WorldHandle* handle);
typedef struct {
    const char* name;
    const char** defines_components;   // Components this plugin creates
    const char** requires_components;  // Components this plugin needs
    size_t defines_count;
    size_t requires_count;
    PluginInitFn init_fn;
    PluginTickFn tick_fn;              // NULL if no tick behavior
    PluginFrameFn frame_fn;            // NULL if no frame behavior
} PluginInfo;
extern "C" PluginInfo* get_plugin_info();
// C functions for component access
extern "C" void* world_get_component(WorldHandle* handle, uint32_t id);
extern "C" uint32_t world_define_component(WorldHandle* handle, const char* name, size_t size);
```
**Layer 2: C++ convenience header (compiles INTO plugin)**
```cpp
// cask/plugin.hpp - nice DX, never crosses .so boundary
#include <cask/abi.h>
namespace cask {
class WorldView {
    WorldHandle* handle_;
public:
    explicit WorldView(WorldHandle* h) : handle_(h) {}
    
    template<typename T>
    T* get(uint32_t component_id) {
        return static_cast<T*>(world_get_component(handle_, component_id));
    }
};
}
```
Component Definition
Plugins declare what components they define and require:
```cpp
// transform_components.cpp - defines Position, Rotation
extern "C" {
    uint32_t POSITION_ID = 0;  // Convention: "Position" → POSITION_ID
    uint32_t ROTATION_ID = 0;
    
    void init(WorldHandle* handle) {
        POSITION_ID = world_define_component(handle, "Position", sizeof(PositionData));
        ROTATION_ID = world_define_component(handle, "Rotation", sizeof(RotationData));
    }
    
    static const char* defines[] = {"Position", "Rotation"};
    
    PluginInfo* get_plugin_info() {
        static PluginInfo info = {
            .name = "TransformComponents",
            .defines_components = defines,
            .defines_count = 2,
            .init_fn = init
        };
        return &info;
    }
}
```
Dependency Graph
At load time, before any init runs, the engine:
1. Loads all plugin manifests
2. Builds dependency graph from defines/requires declarations
3. Validates: no missing deps, no circular deps, no duplicate definitions
4. Topologically sorts for init order
5. Panics with clear error if invalid
```cpp
// Validation errors caught at startup, not mid-game
"Plugin 'Physics' requires 'Position', but nothing defines it"
"Both 'PluginA' and 'PluginB' define 'Velocity' - conflict"
"Circular dependency: Physics → Collision → Physics"
```
Configuration File
# systems.conf
[systems]
input.so
physics.so
collision.so
opengl_render.so
audio.so
Load order is explicit. Base game and mods use the same mechanism (like Skyrim).
---
Example Systems
Physics System (Tick Only)
```cpp
// physics_system.cpp
#include <cask/plugin.hpp>
#include <transform_components.hpp>  // Defines PositionData, VelocityData
extern "C" {
    uint32_t position_id;
    uint32_t velocity_id;
    uint32_t prev_position_id;
    
    void physics_init(WorldHandle* handle) {
        // IDs wired up by engine based on requires_components
        position_id = *(uint32_t*)plugin_get_symbol("TransformComponents", "POSITION_ID");
        velocity_id = *(uint32_t*)plugin_get_symbol("TransformComponents", "VELOCITY_ID");
        prev_position_id = *(uint32_t*)plugin_get_symbol("TransformComponents", "PREV_POSITION_ID");
    }
    
    void physics_tick(WorldHandle* handle) {
        cask::WorldView world(handle);
        
        auto* positions = world.get<PositionData>(position_id);
        auto* velocities = world.get<VelocityData>(velocity_id);
        auto* prev_positions = world.get<PositionData>(prev_position_id);
        
        for (size_t entity = 0; entity < positions->count; entity++) {
            prev_positions->data[entity] = positions->data[entity];
            positions->data[entity] += velocities->data[entity];
        }
    }
    
    static const char* requires[] = {"Position", "Velocity", "PrevPosition"};
    
    PluginInfo* get_plugin_info() {
        static PluginInfo info = {
            .name = "PhysicsSystem",
            .requires_components = requires,
            .requires_count = 3,
            .init_fn = physics_init,
            .tick_fn = physics_tick
        };
        return &info;
    }
}
```
Key insight: No dt parameter. Determinism comes from tick count, not time.
Renderer (Frame Only)
```cpp
// opengl_renderer.cpp
#include <cask/plugin.hpp>
#include <transform_components.hpp>
extern "C" {
    uint32_t position_id;
    uint32_t prev_position_id;
    
    void render_frame(WorldHandle* handle, float alpha) {
        cask::WorldView world(handle);
        
        auto* positions = world.get<PositionData>(position_id);
        auto* prev_positions = world.get<PositionData>(prev_position_id);
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        for (size_t entity = 0; entity < positions->count; entity++) {
            Vec3 render_pos = lerp(prev_positions->data[entity], 
                                   positions->data[entity], alpha);
            draw_entity(entity, render_pos);
        }
        
        swap_buffers();
    }
    
    static const char* requires[] = {"Position", "PrevPosition"};
    
    PluginInfo* get_plugin_info() {
        static PluginInfo info = {
            .name = "OpenGLRenderer",
            .requires_components = requires,
            .requires_count = 2,
            .frame_fn = render_frame
        };
        return &info;
    }
}
```
Renderers are swappable:
- terminal_render.so - ASCII output for debugging
- opengl_render.so - OpenGL graphics  
- directx_render.so - DirectX on Windows
- headless_render.so - No-op for testing
---
Testing
> "No mocking, no setup, no hidden state. Just data in, data out."
Unit Testing a System
TEST_CASE("physics moves entity by velocity each tick", "[physics]") {
    World world;
    init_components(&world);
    
    Entity player = create_entity(&world);
    Vec3* velocities = get_component_array(&world, VELOCITY_ID);
    Vec3* positions = get_component_array(&world, POSITION_ID);
    
    velocities[player] = {1, 0, 0};
    positions[player] = {0, 0, 0};
    
    for (int tick = 0; tick < 10; tick++) {
        physics_tick(&world);
    }
    
    REQUIRE(positions[player].x == 10.0f);  // Perfectly deterministic
}
Integration Testing
Load only systems under test:
TEST_CASE("player stops at wall", "[physics][collision]") {
    Engine engine;
    engine_init(&engine);
    
    load_system(&engine, "./physics.so");
    load_system(&engine, "./collision.so");
    // NO renderer, NO audio - just the systems under test
    
    Entity player = create_entity(&engine.world);
    set_component(player, VELOCITY_ID, Vec3{10, 0, 0});
    
    Entity wall = create_entity(&engine.world);
    set_component(wall, POSITION_ID, Vec3{5, 0, 0});
    set_component(wall, COLLIDER_ID, BoxCollider{1, 1, 1});
    
    for (int tick = 0; tick < 100; tick++) {
        run_tick_systems(&engine);
    }
    
    Vec3 pos = get_component(player, POSITION_ID);
    REQUIRE(pos.x < 6.0f);
}
Fake Implementations
// fake_renderer.cpp
extern "C" void fake_frame(World* world, float alpha) {
    // Do nothing - testing logic, not graphics
}
extern "C" SystemPlugin* get_system_plugin() {
    static SystemPlugin plugin = {
        .tick_fn = NULL,
        .frame_fn = fake_frame,
        .name = "Fake Renderer",
        .version = 1
    };
    return &plugin;
}
Injectable Clock
struct Clock {
    void* context;
    float (*get_time)(void* context);
};
// Production
float real_get_time(void* context) {
    return glfwGetTime();
}
// Testing
struct FakeClock {
    float current_time;
};
float fake_get_time(void* context) {
    return ((FakeClock*)context)->current_time;
}
// Tests control time explicitly
fake_clock.current_time = 0.0f;
engine_tick(&engine, &clock);
fake_clock.current_time = 0.016f;
engine_tick(&engine, &clock);
---
Non-Negotiable Constraints
From the C++ skill and discussions:
1. TDD is mandatory - write failing tests first, always
2. No comments - code is self-documenting
3. No single-character names - entity not e, index not i (exception: loop indices)
4. No nested conditionals - early returns, extract predicates
5. Screaming architecture - organize by domain, not technical layer
6. Fakes over mocks - real in-memory implementations
7. Catch2 for testing - BDD-style sections
---
Open Questions
These emerged from discussions but weren't fully resolved:
1. Cross-platform abstraction - thin layer for dlopen/LoadLibrary
2. Hot reload during development - watch directory, reload changed plugins?
3. Entity lifecycle - pooling, generation counters for validity?