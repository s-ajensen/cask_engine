#include <cask/abi.h>
#include <cask/world.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static constexpr size_t CUBE_COUNT = 1000000;
static constexpr float TICK_RATE = 60.0f;

struct CubeState {
    float* pos_x;
    float* pos_y;
    float* pos_z;
    float* prev_y;

    float* base_y;
    float* amplitude;
    float* frequency;
    float* phase;

    float* color_r;
    float* color_g;
    float* color_b;

    size_t count;
    int tick;
};

struct RenderState {
    GLFWwindow* window;
    GLuint shader_program;
    GLuint vao;
    GLuint cube_vbo;
    GLuint instance_vbo;
    GLint mvp_loc;
};

static CubeState cubes;
static RenderState render;
static uint32_t cubes_id = 0;
static uint32_t render_id = 0;

static float randf() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

static float randf_range(float min_val, float max_val) {
    return min_val + randf() * (max_val - min_val);
}

static const char* vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_instance_pos;
layout (location = 2) in vec3 a_instance_color;

uniform mat4 u_mvp;

out vec3 v_color;
out vec3 v_normal;

void main() {
    vec3 world_pos = a_pos * 0.3 + a_instance_pos;
    gl_Position = u_mvp * vec4(world_pos, 1.0);
    v_color = a_instance_color;
    v_normal = a_pos;
}
)";

static const char* fragment_shader_src = R"(
#version 330 core
in vec3 v_color;
in vec3 v_normal;

out vec4 frag_color;

void main() {
    vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0));
    float diffuse = max(dot(normalize(v_normal), light_dir), 0.0);
    float ambient = 0.3;
    vec3 lit_color = v_color * (ambient + diffuse * 0.7);
    frag_color = vec4(lit_color, 1.0);
}
)";

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::printf("Shader compile error: %s\n", log);
    }
    return shader;
}

static GLuint create_shader_program() {
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::printf("Shader link error: %s\n", log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

static void init_cubes() {
    cubes.count = CUBE_COUNT;
    cubes.tick = 0;

    cubes.pos_x = new float[CUBE_COUNT];
    cubes.pos_y = new float[CUBE_COUNT];
    cubes.pos_z = new float[CUBE_COUNT];
    cubes.prev_y = new float[CUBE_COUNT];
    cubes.base_y = new float[CUBE_COUNT];
    cubes.amplitude = new float[CUBE_COUNT];
    cubes.frequency = new float[CUBE_COUNT];
    cubes.phase = new float[CUBE_COUNT];
    cubes.color_r = new float[CUBE_COUNT];
    cubes.color_g = new float[CUBE_COUNT];
    cubes.color_b = new float[CUBE_COUNT];

    float grid_size = std::cbrt(static_cast<float>(CUBE_COUNT));
    float spacing = 1.5f;
    float offset = (grid_size * spacing) / 2.0f;

    for (size_t index = 0; index < CUBE_COUNT; index++) {
        size_t ix = index % static_cast<size_t>(grid_size);
        size_t iy = (index / static_cast<size_t>(grid_size)) % static_cast<size_t>(grid_size);
        size_t iz = index / static_cast<size_t>(grid_size * grid_size);

        cubes.pos_x[index] = ix * spacing - offset;
        cubes.base_y[index] = iy * spacing - offset;
        cubes.pos_z[index] = iz * spacing - offset;

        cubes.pos_y[index] = cubes.base_y[index];
        cubes.prev_y[index] = cubes.base_y[index];

        cubes.amplitude[index] = randf_range(0.2f, 1.0f);
        cubes.frequency[index] = randf_range(0.5f, 3.0f);
        cubes.phase[index] = randf_range(0.0f, 6.28318f);

        cubes.color_r[index] = randf_range(0.2f, 1.0f);
        cubes.color_g[index] = randf_range(0.2f, 1.0f);
        cubes.color_b[index] = randf_range(0.2f, 1.0f);
    }
}

static void init_render() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    render.window = glfwCreateWindow(1280, 720, "Cask Cubes", nullptr, nullptr);
    glfwMakeContextCurrent(render.window);
    glfwSwapInterval(1);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    render.shader_program = create_shader_program();
    render.mvp_loc = glGetUniformLocation(render.shader_program, "u_mvp");

    float cube_verts[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    };

    glGenVertexArrays(1, &render.vao);
    glBindVertexArray(render.vao);

    glGenBuffers(1, &render.cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, render.cube_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &render.instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, render.instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, CUBE_COUNT * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
}

static void make_perspective(float* matrix, float fov, float aspect, float near_plane, float far_plane) {
    float tan_half_fov = std::tan(fov / 2.0f);
    std::memset(matrix, 0, 16 * sizeof(float));
    matrix[0] = 1.0f / (aspect * tan_half_fov);
    matrix[5] = 1.0f / tan_half_fov;
    matrix[10] = -(far_plane + near_plane) / (far_plane - near_plane);
    matrix[11] = -1.0f;
    matrix[14] = -(2.0f * far_plane * near_plane) / (far_plane - near_plane);
}

static void make_look_at(float* matrix, float eye_x, float eye_y, float eye_z,
                         float center_x, float center_y, float center_z) {
    float fx = center_x - eye_x;
    float fy = center_y - eye_y;
    float fz = center_z - eye_z;
    float f_len = std::sqrt(fx*fx + fy*fy + fz*fz);
    fx /= f_len; fy /= f_len; fz /= f_len;

    float up_x = 0, up_y = 1, up_z = 0;
    float sx = fy * up_z - fz * up_y;
    float sy = fz * up_x - fx * up_z;
    float sz = fx * up_y - fy * up_x;
    float s_len = std::sqrt(sx*sx + sy*sy + sz*sz);
    sx /= s_len; sy /= s_len; sz /= s_len;

    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    matrix[0] = sx; matrix[4] = sy; matrix[8]  = sz; matrix[12] = -(sx*eye_x + sy*eye_y + sz*eye_z);
    matrix[1] = ux; matrix[5] = uy; matrix[9]  = uz; matrix[13] = -(ux*eye_x + uy*eye_y + uz*eye_z);
    matrix[2] = -fx; matrix[6] = -fy; matrix[10] = -fz; matrix[14] = (fx*eye_x + fy*eye_y + fz*eye_z);
    matrix[3] = 0; matrix[7] = 0; matrix[11] = 0; matrix[15] = 1;
}

static void mat4_multiply(float* result, const float* a, const float* b) {
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            result[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
}

void plugin_init(WorldHandle handle) {
    cask::WorldView world(handle);

    cubes_id = world.register_component("CubeState");
    render_id = world.register_component("RenderState");
    world.bind(cubes_id, &cubes);
    world.bind(render_id, &render);

    srand(42);
    init_cubes();
    init_render();

    std::printf("Cubes plugin initialized: %zu cubes\n", CUBE_COUNT);
}

void plugin_tick(WorldHandle handle) {
    cask::WorldView world(handle);
    CubeState* state = world.get<CubeState>(cubes_id);

    state->tick++;
    float time = static_cast<float>(state->tick) / TICK_RATE;

    for (size_t index = 0; index < state->count; index++) {
        state->prev_y[index] = state->pos_y[index];
        state->pos_y[index] = state->base_y[index] +
            state->amplitude[index] * std::sin(state->phase[index] + time * state->frequency[index]);
    }
}

void plugin_frame(WorldHandle handle, float alpha) {
    cask::WorldView world(handle);
    CubeState* state = world.get<CubeState>(cubes_id);
    RenderState* rs = world.get<RenderState>(render_id);

    if (glfwWindowShouldClose(rs->window)) {
        std::exit(0);
    }

    glfwPollEvents();

    int width, height;
    glfwGetFramebufferSize(rs->window, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float* instance_data = new float[state->count * 6];
    for (size_t index = 0; index < state->count; index++) {
        float interp_y = state->prev_y[index] + alpha * (state->pos_y[index] - state->prev_y[index]);
        instance_data[index * 6 + 0] = state->pos_x[index];
        instance_data[index * 6 + 1] = interp_y;
        instance_data[index * 6 + 2] = state->pos_z[index];
        instance_data[index * 6 + 3] = state->color_r[index];
        instance_data[index * 6 + 4] = state->color_g[index];
        instance_data[index * 6 + 5] = state->color_b[index];
    }

    glBindBuffer(GL_ARRAY_BUFFER, rs->instance_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, state->count * 6 * sizeof(float), instance_data);
    delete[] instance_data;

    float time = static_cast<float>(state->tick + alpha) / TICK_RATE;
    float cam_dist = 200.0f;
    float cam_x = std::sin(time * 0.3f) * cam_dist;
    float cam_z = std::cos(time * 0.3f) * cam_dist;
    float cam_y = 80.0f + std::sin(time * 0.2f) * 40.0f;

    float proj[16], view[16], mvp[16];
    make_perspective(proj, 0.8f, static_cast<float>(width) / height, 0.1f, 500.0f);
    make_look_at(view, cam_x, cam_y, cam_z, 0, 0, 0);
    mat4_multiply(mvp, proj, view);

    glUseProgram(rs->shader_program);
    glUniformMatrix4fv(rs->mvp_loc, 1, GL_FALSE, mvp);

    glBindVertexArray(rs->vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, static_cast<GLsizei>(state->count));

    glfwSwapBuffers(rs->window);
}

void plugin_shutdown(WorldHandle handle) {
    glDeleteVertexArrays(1, &render.vao);
    glDeleteBuffers(1, &render.cube_vbo);
    glDeleteBuffers(1, &render.instance_vbo);
    glDeleteProgram(render.shader_program);
    glfwDestroyWindow(render.window);
    glfwTerminate();

    delete[] cubes.pos_x;
    delete[] cubes.pos_y;
    delete[] cubes.pos_z;
    delete[] cubes.prev_y;
    delete[] cubes.base_y;
    delete[] cubes.amplitude;
    delete[] cubes.frequency;
    delete[] cubes.phase;
    delete[] cubes.color_r;
    delete[] cubes.color_g;
    delete[] cubes.color_b;

    std::printf("Cubes plugin shutdown\n");
}

static const char* defines[] = {"CubeState", "RenderState"};

static PluginInfo plugin_info = {
    .name = "CubesPlugin",
    .defines_components = defines,
    .requires_components = nullptr,
    .defines_count = 2,
    .requires_count = 0,
    .init_fn = plugin_init,
    .tick_fn = plugin_tick,
    .frame_fn = plugin_frame,
    .shutdown_fn = plugin_shutdown
};

extern "C" PluginInfo* get_plugin_info() {
    return &plugin_info;
}
