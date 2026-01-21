#include "engine/renderer.h"
#include "engine/game.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>

static double g_last_time = 0.0;

static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_contexts[4];

static void game_loop() {
    double current_time = emscripten_get_now() / 1000.0;
    float delta_time = (float)(current_time - g_last_time);
    g_last_time = current_time;

    if (delta_time > 0.1f) delta_time = 0.1f; // Cap delta time

    update_game(delta_time);

    // Collect all AI positions
    float ai_positions[4][2];
    int ai_teams[4];

    for (int i = 0; i < 4; i++) {
        ai_positions[i][0] = get_ai_x(i); // x
        ai_positions[i][1] = get_ai_y(i); // y
        ai_teams[i] = get_ai_team(i);
    }

    // Render each AI entity's perspective to the appropriate canvas
    const char* canvas_ids[4] = {"#canvas-red", "#canvas-blue", "#canvas-purple", "#canvas-brown"};

    for (int i = 0; i < 4; i++) {
        // Switch to the appropriate WebGL context
        emscripten_webgl_make_context_current(g_contexts[i]);

        render_frame_for_viewport(ai_positions[i][0], ai_positions[i][1], 50.0f, i, ai_positions, ai_teams);
    }
}

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void init() {
        g_last_time = emscripten_get_now() / 1000.0;
        init_game();
        
        // Ensure WebGL context is created before initializing renderer
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        attrs.majorVersion = 2;
        attrs.minorVersion = 0;
        attrs.alpha = false;
        attrs.depth = false;
        attrs.stencil = false;
        attrs.antialias = false;
        
        // Create WebGL contexts for all 4 canvases
        const char* canvas_ids[4] = {"#canvas-red", "#canvas-blue", "#canvas-purple", "#canvas-brown"};

        // Get canvas dimensions from first canvas (all should be same size)
        int canvas_width = EM_ASM_INT({
            const canvas = document.querySelector("#canvas-red");
            return canvas ? canvas.width : 400;
        });
        int canvas_height = EM_ASM_INT({
            const canvas = document.querySelector("#canvas-red");
            return canvas ? canvas.height : 400;
        });

        for (int i = 0; i < 4; i++) {
            g_contexts[i] = emscripten_webgl_create_context(canvas_ids[i], &attrs);

            if (g_contexts[i] > 0) {
                // Make context current and initialize renderer for this context
                emscripten_webgl_make_context_current(g_contexts[i]);
                init_renderer(canvas_width, canvas_height, i);
            }
        }

        // Set the first context as current for initial setup
        if (g_contexts[0] > 0) {
            emscripten_webgl_make_context_current(g_contexts[0]);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void start_game_loop() {
        emscripten_set_main_loop(game_loop, 0, 1);
    }
}
