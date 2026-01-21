#ifndef GAME_H
#define GAME_H

#define NUM_AI_ENTITIES 4

enum TeamColor {
    TEAM_RED = 0,
    TEAM_BLUE = 1,
    TEAM_PURPLE = 2,
    TEAM_BROWN = 3
};

struct AIEntity {
    float x, y;
    float vx, vy; // velocity components
    TeamColor team;
    float move_timer; // timer for random movement changes
};

#ifdef __cplusplus
extern "C" {
#endif

void init_game();
void update_game(float delta_time);
float get_ai_x(int ai_index);
float get_ai_y(int ai_index);
int get_ai_team(int ai_index);

#ifdef __cplusplus
}
#endif

#endif
