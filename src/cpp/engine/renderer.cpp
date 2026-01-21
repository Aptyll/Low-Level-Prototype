#include "renderer.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>

static int g_canvas_width = 400; // Split screen, so half width
static int g_canvas_height = 400; // Split screen, so half height
static float g_grid_size = 50.0f;

// Team colors (RGB)
static float g_team_colors[4][3] = {
    {1.0f, 0.2f, 0.2f}, // Red
    {0.2f, 0.2f, 1.0f}, // Blue
    {0.8f, 0.2f, 0.8f}, // Purple
    {0.6f, 0.4f, 0.2f}  // Brown
};

// Simple shader sources
static const char* vertex_shader_source = R"(#version 300 es
precision mediump float;
in vec2 a_position;
uniform vec2 u_resolution;
uniform vec2 u_offset;

void main() {
    vec2 position = (a_position + u_offset) / u_resolution * 2.0 - 1.0;
    position.y = -position.y;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

static const char* fragment_shader_source = R"(#version 300 es
precision mediump float;
uniform vec3 u_color;
out vec4 fragColor;

void main() {
    fragColor = vec4(u_color, 1.0);
}
)";

// Per-context renderer state (one per WebGL context)
struct RendererState {
    GLuint shader_program;
    GLuint grid_vao;
    GLuint player_vao;
    bool initialized;
};

static RendererState g_renderer_states[4] = {
    {0, 0, 0, false},
    {0, 0, 0, false},
    {0, 0, 0, false},
    {0, 0, 0, false}
};

static void draw_directional_arrows(RendererState& state, float center_ai_x, float center_ai_y,
                                    float ai_positions[4][2], int ai_teams[4], int viewport_index,
                                    float offset_x, float offset_y, GLint color_loc, GLint offset_loc);

static GLuint compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        return 0;
    }
    return shader;
}

static GLuint create_shader_program() {
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    
    if (!vertex_shader || !fragment_shader) {
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success) {
        return 0;
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    return program;
}

static void create_grid_vao(RendererState& state) {
    // Create VAO for grid lines
    glGenVertexArrays(1, &state.grid_vao);
    glBindVertexArray(state.grid_vao);
    
    // We'll draw grid lines dynamically, so just set up the VAO structure
    glBindVertexArray(0);
}

static void create_player_vao(RendererState& state) {
    // Create VAO for player square
    glGenVertexArrays(1, &state.player_vao);
    glBindVertexArray(state.player_vao);
    
    // Player square vertices (centered at origin, 20x20)
    float size = 20.0f;
    float vertices[] = {
        -size/2, -size/2,
         size/2, -size/2,
         size/2,  size/2,
        -size/2,  size/2
    };
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    GLint pos_attr = glGetAttribLocation(state.shader_program, "a_position");
    glEnableVertexAttribArray(pos_attr);
    glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glBindVertexArray(0);
}

extern "C" {
    void init_renderer(int canvas_width, int canvas_height, int context_index) {
        if (context_index < 0 || context_index >= 4) return;
        
        RendererState& state = g_renderer_states[context_index];
        
        g_canvas_width = canvas_width;
        g_canvas_height = canvas_height;
        
        // Create shader program for this context
        state.shader_program = create_shader_program();
        if (!state.shader_program) {
            return;
        }
        
        // Set up viewport
        glViewport(0, 0, canvas_width, canvas_height);
        
        // Create VAOs for this context
        create_grid_vao(state);
        create_player_vao(state);
        
        state.initialized = true;
    }

    EMSCRIPTEN_KEEPALIVE
    void resize_renderer(int canvas_width, int canvas_height) {
        g_canvas_width = canvas_width;
        g_canvas_height = canvas_height;
        // Viewport will be updated on next render when context is made current
    }
    
    void render_frame(float player_x, float player_y, float grid_size) {
        // Use context 0's state for backward compatibility
        RendererState& state = g_renderer_states[0];
        if (!state.initialized || !state.shader_program) return;
        
        g_grid_size = grid_size;
        
        // Clear with soft dark background
        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(state.shader_program);
        
        // Set resolution uniform
        GLint resolution_loc = glGetUniformLocation(state.shader_program, "u_resolution");
        glUniform2f(resolution_loc, (float)g_canvas_width, (float)g_canvas_height);
        
        // Calculate camera offset (center on player)
        float offset_x = -player_x + g_canvas_width / 2.0f;
        float offset_y = -player_y + g_canvas_height / 2.0f;
        
        // Draw subtle grid lines
        glBindVertexArray(state.grid_vao);
        
        GLint color_loc = glGetUniformLocation(state.shader_program, "u_color");
        GLint offset_loc = glGetUniformLocation(state.shader_program, "u_offset");
        
        // Draw subtle grid lines
        int grid_start_x = (int)((player_x - g_canvas_width / 2.0f) / grid_size) - 1;
        int grid_end_x = (int)((player_x + g_canvas_width / 2.0f) / grid_size) + 1;
        int grid_start_y = (int)((player_y - g_canvas_height / 2.0f) / grid_size) - 1;
        int grid_end_y = (int)((player_y + g_canvas_height / 2.0f) / grid_size) + 1;
        
        // Set subtle dark gray color for grid lines
        glUniform3f(color_loc, 0.12f, 0.12f, 0.12f); // Soft dark gray
        
        // Draw vertical grid lines
        for (int x = grid_start_x; x <= grid_end_x; x++) {
            float line_x = x * grid_size;
            float line_start_y = grid_start_y * grid_size;
            float line_end_y = (grid_end_y + 1) * grid_size;
            
            float vertices[] = {
                line_x, line_start_y,
                line_x, line_end_y
            };
            
            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            
            GLint pos_attr = glGetAttribLocation(state.shader_program, "a_position");
            glEnableVertexAttribArray(pos_attr);
            glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
            
            glUniform2f(offset_loc, offset_x, offset_y);
            glDrawArrays(GL_LINES, 0, 2);
            
            glDeleteBuffers(1, &vbo);
        }
        
        // Draw horizontal grid lines
        for (int y = grid_start_y; y <= grid_end_y; y++) {
            float line_y = y * grid_size;
            float line_start_x = grid_start_x * grid_size;
            float line_end_x = (grid_end_x + 1) * grid_size;
            
            float vertices[] = {
                line_start_x, line_y,
                line_end_x, line_y
            };
            
            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            
            GLint pos_attr = glGetAttribLocation(state.shader_program, "a_position");
            glEnableVertexAttribArray(pos_attr);
            glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
            
            glUniform2f(offset_loc, offset_x, offset_y);
            glDrawArrays(GL_LINES, 0, 2);
            
            glDeleteBuffers(1, &vbo);
        }
        
        // Draw player square at player's world position
        // Player square vertices are centered at origin, so we need to offset by player position
        float player_size = 20.0f;
        float player_vertices[] = {
            player_x - player_size/2, player_y - player_size/2,
            player_x + player_size/2, player_y - player_size/2,
            player_x + player_size/2, player_y + player_size/2,
            player_x - player_size/2, player_y + player_size/2
        };
        
        GLuint player_vbo;
        glGenBuffers(1, &player_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, player_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(player_vertices), player_vertices, GL_STATIC_DRAW);
        
        GLint pos_attr = glGetAttribLocation(state.shader_program, "a_position");
        glEnableVertexAttribArray(pos_attr);
        glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
        
        glUniform3f(color_loc, 0.8f, 0.8f, 0.9f); // Light gray/white for player
        glUniform2f(offset_loc, offset_x, offset_y);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        glDeleteBuffers(1, &player_vbo);
        glBindVertexArray(0);
    }

    void render_frame_for_viewport(float center_ai_x, float center_ai_y, float grid_size, int viewport_index,
                                  float ai_positions[4][2], int ai_teams[4]) {
        if (viewport_index < 0 || viewport_index >= 4) return;
        
        RendererState& state = g_renderer_states[viewport_index];
        if (!state.initialized || !state.shader_program) return;
        
        g_grid_size = grid_size;

        // Update viewport to match current canvas size
        glViewport(0, 0, g_canvas_width, g_canvas_height);

        // Clear with soft dark background
        glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(state.shader_program);

        // Set resolution uniform
        GLint resolution_loc = glGetUniformLocation(state.shader_program, "u_resolution");
        glUniform2f(resolution_loc, (float)g_canvas_width, (float)g_canvas_height);

        // Calculate camera offset (center on the specified AI)
        float offset_x = -center_ai_x + g_canvas_width / 2.0f;
        float offset_y = -center_ai_y + g_canvas_height / 2.0f;

        // Draw subtle grid lines
        glBindVertexArray(state.grid_vao);

        GLint color_loc = glGetUniformLocation(state.shader_program, "u_color");
        GLint offset_loc = glGetUniformLocation(state.shader_program, "u_offset");

        // Draw subtle grid lines
        int grid_start_x = (int)((center_ai_x - g_canvas_width / 2.0f) / grid_size) - 1;
        int grid_end_x = (int)((center_ai_x + g_canvas_width / 2.0f) / grid_size) + 1;
        int grid_start_y = (int)((center_ai_y - g_canvas_height / 2.0f) / grid_size) - 1;
        int grid_end_y = (int)((center_ai_y + g_canvas_height / 2.0f) / grid_size) + 1;

        // Set subtle dark gray color for grid lines
        glUniform3f(color_loc, 0.12f, 0.12f, 0.12f); // Soft dark gray

        // Draw vertical grid lines
        for (int x = grid_start_x; x <= grid_end_x; x++) {
            float line_x = x * grid_size;
            float line_start_y = grid_start_y * grid_size;
            float line_end_y = (grid_end_y + 1) * grid_size;

            float vertices[] = {
                line_x, line_start_y,
                line_x, line_end_y
            };

            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            GLint pos_attr = glGetAttribLocation(state.shader_program, "a_position");
            glEnableVertexAttribArray(pos_attr);
            glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);

            glUniform2f(offset_loc, offset_x, offset_y);
            glDrawArrays(GL_LINES, 0, 2);

            glDeleteBuffers(1, &vbo);
        }

        // Draw horizontal grid lines
        for (int y = grid_start_y; y <= grid_end_y; y++) {
            float line_y = y * grid_size;
            float line_start_x = grid_start_x * grid_size;
            float line_end_x = (grid_end_x + 1) * grid_size;

            float vertices[] = {
                line_start_x, line_y,
                line_end_x, line_y
            };

            GLuint vbo;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            GLint pos_attr = glGetAttribLocation(state.shader_program, "a_position");
            glEnableVertexAttribArray(pos_attr);
            glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);

            glUniform2f(offset_loc, offset_x, offset_y);
            glDrawArrays(GL_LINES, 0, 2);

            glDeleteBuffers(1, &vbo);
        }

        // Draw all AI entities (including the centered one)
        for (int i = 0; i < 4; i++) {
            float ai_x = ai_positions[i][0];
            float ai_y = ai_positions[i][1];
            int team = ai_teams[i];

            // Draw AI square
            float ai_size = 20.0f;
            float ai_vertices[] = {
                ai_x - ai_size/2, ai_y - ai_size/2,
                ai_x + ai_size/2, ai_y - ai_size/2,
                ai_x + ai_size/2, ai_y + ai_size/2,
                ai_x - ai_size/2, ai_y + ai_size/2
            };

            GLuint ai_vbo;
            glGenBuffers(1, &ai_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, ai_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(ai_vertices), ai_vertices, GL_STATIC_DRAW);

            GLint pos_attr = glGetAttribLocation(state.shader_program, "a_position");
            glEnableVertexAttribArray(pos_attr);
            glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);

            // Use team color
            if (team >= 0 && team < 4) {
                glUniform3f(color_loc, g_team_colors[team][0], g_team_colors[team][1], g_team_colors[team][2]);
            } else {
                glUniform3f(color_loc, 0.8f, 0.8f, 0.9f); // Default light gray
            }

            glUniform2f(offset_loc, offset_x, offset_y);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

            glDeleteBuffers(1, &ai_vbo);
        }

        // Draw directional arrows for AI entities outside the viewport
        draw_directional_arrows(state, center_ai_x, center_ai_y, ai_positions, ai_teams, viewport_index, offset_x, offset_y, color_loc, offset_loc);

        glBindVertexArray(0);
    }

    static void draw_directional_arrows(RendererState& state, float center_ai_x, float center_ai_y,
                                        float ai_positions[4][2], int ai_teams[4], int viewport_index,
                                        float offset_x, float offset_y, GLint color_loc, GLint offset_loc) {
        const float viewport_half_width = g_canvas_width / 2.0f;
        const float viewport_half_height = g_canvas_height / 2.0f;
        const float arrow_size = 15.0f;
        const float edge_margin = 20.0f; // Distance from edge

        for (int i = 0; i < 4; i++) {
            // Skip the centered AI (don't show arrow for self)
            if (i == viewport_index) continue;

            float ai_x = ai_positions[i][0];
            float ai_y = ai_positions[i][1];
            int team = ai_teams[i];

            // Calculate direction vector from center AI to this AI
            float dx = ai_x - center_ai_x;
            float dy = ai_y - center_ai_y;

            // Check if AI is outside viewport bounds
            float viewport_left = center_ai_x - viewport_half_width;
            float viewport_right = center_ai_x + viewport_half_width;
            float viewport_top = center_ai_y - viewport_half_height;
            float viewport_bottom = center_ai_y + viewport_half_height;

            bool outside_viewport = (ai_x < viewport_left || ai_x > viewport_right ||
                                    ai_y < viewport_top || ai_y > viewport_bottom);

            if (!outside_viewport) continue;

            // Calculate angle
            float angle = atan2(dy, dx);

            // Calculate intersection point with viewport edge
            float edge_x, edge_y;

            // Determine which edge to place arrow on
            // Calculate intersections with all edges
            float t_left = (viewport_left - center_ai_x) / (dx != 0 ? dx : 0.0001f);
            float t_right = (viewport_right - center_ai_x) / (dx != 0 ? dx : 0.0001f);
            float t_top = (viewport_top - center_ai_y) / (dy != 0 ? dy : 0.0001f);
            float t_bottom = (viewport_bottom - center_ai_y) / (dy != 0 ? dy : 0.0001f);

            // Find the valid intersection (t between 0 and 1, and within edge bounds)
            float t = 1.0f;
            if (t_left > 0 && t_left < t) {
                float y_intersect = center_ai_y + dy * t_left;
                if (y_intersect >= viewport_top && y_intersect <= viewport_bottom) {
                    t = t_left;
                    edge_x = viewport_left;
                    edge_y = y_intersect;
                }
            }
            if (t_right > 0 && t_right < t) {
                float y_intersect = center_ai_y + dy * t_right;
                if (y_intersect >= viewport_top && y_intersect <= viewport_bottom) {
                    t = t_right;
                    edge_x = viewport_right;
                    edge_y = y_intersect;
                }
            }
            if (t_top > 0 && t_top < t) {
                float x_intersect = center_ai_x + dx * t_top;
                if (x_intersect >= viewport_left && x_intersect <= viewport_right) {
                    t = t_top;
                    edge_x = x_intersect;
                    edge_y = viewport_top;
                }
            }
            if (t_bottom > 0 && t_bottom < t) {
                float x_intersect = center_ai_x + dx * t_bottom;
                if (x_intersect >= viewport_left && x_intersect <= viewport_right) {
                    t = t_bottom;
                    edge_x = x_intersect;
                    edge_y = viewport_bottom;
                }
            }

            // Clamp arrow position to viewport edges with margin
            edge_x = std::max(viewport_left + edge_margin, std::min(viewport_right - edge_margin, edge_x));
            edge_y = std::max(viewport_top + edge_margin, std::min(viewport_bottom - edge_margin, edge_y));

            // Draw arrow pointing in the direction
            // Arrow is a triangle pointing outward
            float cos_a = cos(angle);
            float sin_a = sin(angle);

            // Arrow tip (pointing outward)
            float tip_x = edge_x + cos_a * arrow_size;
            float tip_y = edge_y + sin_a * arrow_size;

            // Arrow base (two points forming the base)
            float perp_x = -sin_a;
            float perp_y = cos_a;
            float base_half_width = arrow_size * 0.5f;

            float base1_x = edge_x + perp_x * base_half_width;
            float base1_y = edge_y + perp_y * base_half_width;
            float base2_x = edge_x - perp_x * base_half_width;
            float base2_y = edge_y - perp_y * base_half_width;

            // Draw arrow triangle
            float arrow_vertices[] = {
                tip_x, tip_y,
                base1_x, base1_y,
                base2_x, base2_y
            };

            GLuint arrow_vbo;
            glGenBuffers(1, &arrow_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, arrow_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(arrow_vertices), arrow_vertices, GL_STATIC_DRAW);

            GLint pos_attr = glGetAttribLocation(state.shader_program, "a_position");
            glEnableVertexAttribArray(pos_attr);
            glVertexAttribPointer(pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);

            // Use team color for arrow
            if (team >= 0 && team < 4) {
                glUniform3f(color_loc, g_team_colors[team][0], g_team_colors[team][1], g_team_colors[team][2]);
            } else {
                glUniform3f(color_loc, 0.8f, 0.8f, 0.9f); // Default light gray
            }

            glUniform2f(offset_loc, offset_x, offset_y);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDeleteBuffers(1, &arrow_vbo);
        }
    }
}
