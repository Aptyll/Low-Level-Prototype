#ifndef RENDERER_H
#define RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

void init_renderer(int canvas_width, int canvas_height, int context_index);
void resize_renderer(int canvas_width, int canvas_height);
void render_frame(float player_x, float player_y, float grid_size);
void render_frame_for_viewport(float center_ai_x, float center_ai_y, float grid_size, int viewport_index,
                              float ai_positions[4][2], int ai_teams[4]);

#ifdef __cplusplus
}
#endif

#endif
