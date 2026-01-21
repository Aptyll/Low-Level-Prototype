#include "game.h"
#include <cstdlib> // for rand()
#include <cmath>  // for sin, cos

static AIEntity g_ai_entities[NUM_AI_ENTITIES];
static float g_ai_speed = 150.0f; // pixels per second
static float g_world_bounds = 1000.0f; // world size

// Simple random number generator (0.0 to 1.0)
float random_float() {
    return (float)rand() / (float)RAND_MAX;
}

extern "C" {
    void init_game() {
        // Initialize AI entities at different starting positions
        static float start_positions[4][2] = {
            {-200.0f, -200.0f}, // Red team
            {200.0f, -200.0f},  // Blue team
            {-200.0f, 200.0f},  // Purple team
            {200.0f, 200.0f}    // Brown team
        };

        for (int i = 0; i < NUM_AI_ENTITIES; i++) {
            g_ai_entities[i].x = start_positions[i][0];
            g_ai_entities[i].y = start_positions[i][1];
            g_ai_entities[i].vx = 0.0f;
            g_ai_entities[i].vy = 0.0f;
            g_ai_entities[i].team = (TeamColor)i;
            g_ai_entities[i].move_timer = 0.0f;
        }

        // Seed random number generator
        srand(42); // Fixed seed for reproducible behavior
    }

    void update_game(float delta_time) {
        for (int i = 0; i < NUM_AI_ENTITIES; i++) {
            AIEntity& ai = g_ai_entities[i];

            // Update movement timer
            ai.move_timer -= delta_time;

            // Change direction randomly every 1-3 seconds
            if (ai.move_timer <= 0.0f) {
                // Random direction (0-360 degrees)
                float angle = random_float() * 6.283185f; // 2 * PI
                float speed = g_ai_speed * (0.5f + random_float() * 0.5f); // 50-100% of base speed

                ai.vx = cos(angle) * speed;
                ai.vy = sin(angle) * speed;

                // Reset timer (1-3 seconds)
                ai.move_timer = 1.0f + random_float() * 2.0f;
            }

            // Update position
            ai.x += ai.vx * delta_time;
            ai.y += ai.vy * delta_time;

            // Keep entities within world bounds (bounce off edges)
            if (ai.x < -g_world_bounds || ai.x > g_world_bounds) {
                ai.vx = -ai.vx;
                ai.x = ai.x < -g_world_bounds ? -g_world_bounds : g_world_bounds;
            }
            if (ai.y < -g_world_bounds || ai.y > g_world_bounds) {
                ai.vy = -ai.vy;
                ai.y = ai.y < -g_world_bounds ? -g_world_bounds : g_world_bounds;
            }
        }
    }

    float get_ai_x(int ai_index) {
        if (ai_index >= 0 && ai_index < NUM_AI_ENTITIES) {
            return g_ai_entities[ai_index].x;
        }
        return 0.0f;
    }

    float get_ai_y(int ai_index) {
        if (ai_index >= 0 && ai_index < NUM_AI_ENTITIES) {
            return g_ai_entities[ai_index].y;
        }
        return 0.0f;
    }

    int get_ai_team(int ai_index) {
        if (ai_index >= 0 && ai_index < NUM_AI_ENTITIES) {
            return (int)g_ai_entities[ai_index].team;
        }
        return 0;
    }
}
