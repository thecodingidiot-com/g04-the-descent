#include <SDL2/SDL.h>
#include "libtci.h"
#include "camera.h"
#include "enemy.h"
#include "map.h"
#include "raycaster.h"
#include "render.h"
#include "weapon.h"

# define TURN_SPEED  0.045f
# define MOVE_SPEED  0.055f

/*
** Move with collision, one axis at a time, so the player slides along a
** wall instead of stopping against it. r03 and r04 both walked straight
** through walls -- by design, since neither chapter was a game -- and
** this is where that debt is paid.
**
** The radius keeps the camera off the wall surface: standing exactly on
** it makes perp_dist zero for the columns in front, and a divide by
** nearly nothing is how a raycaster produces a wall that fills the
** screen with a single colour.
*/
# define PLAYER_RADIUS 0.2f

static void try_move(t_camera *cam, t_map const *map, float dx, float dy)
{
    float   nx;
    float   ny;

    nx = cam->pos.x + dx;
    ny = cam->pos.y + dy;
    if (!map_is_wall(map, (int)(nx + (dx > 0 ? PLAYER_RADIUS : -PLAYER_RADIUS)),
            (int)cam->pos.y))
        cam->pos.x = nx;
    if (!map_is_wall(map, (int)cam->pos.x,
            (int)(ny + (dy > 0 ? PLAYER_RADIUS : -PLAYER_RADIUS))))
        cam->pos.y = ny;
}

static void handle_input(t_camera *cam, t_map const *map, Uint8 const *keys)
{
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_H])
        camera_turn(cam, -TURN_SPEED);
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_L])
        camera_turn(cam, TURN_SPEED);
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_K])
        try_move(cam, map, cam->forward.x * MOVE_SPEED, cam->forward.y * MOVE_SPEED);
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_J])
        try_move(cam, map, -cam->forward.x * MOVE_SPEED, -cam->forward.y * MOVE_SPEED);
}

int main(int argc, char **argv)
{
    t_render    rd;
    t_map       map;
    t_camera    cam;
    t_enemy     enemies[MAX_ENEMIES];
    SDL_Event   ev;
    Uint8 const *keys;
    int         running;
    int         cooldown;

    if (argc < 2) {
        tci_printf("usage: %s <maze.txt>\n", argv[0]);
        return (1);
    }
    if (!map_load(&map, argv[1]))
        return (1);
    if (!render_init(&rd)) {
        tci_printf("error: %s\n", SDL_GetError());
        map_free(&map);
        return (1);
    }
    camera_init(&cam, map.start_pos.x, map.start_pos.y, map.start_angle);
    enemy_reset(enemies, &map);
    cooldown = 0;
    running = 1;
    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)
                running = 0;
            if (ev.type == SDL_KEYDOWN && (ev.key.keysym.sym == SDLK_ESCAPE
                    || ev.key.keysym.sym == SDLK_q))
                running = 0;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_SPACE
                    && cooldown == 0) {
                weapon_fire(&cam, &map, enemies);
                cooldown = WEAPON_COOLDOWN;
            }
        }
        if (cooldown > 0)
            cooldown--;
        keys = SDL_GetKeyboardState(NULL);
        handle_input(&cam, &map, keys);
        rd.health -= enemy_update(enemies, &cam, &map);
        if (rd.health <= 0) {
            tci_printf("You did not find the way out.\n");
            running = 0;
        }
        if (map_is_exit(&map, (int)cam.pos.x, (int)cam.pos.y)) {
            tci_printf("You found the way out.\n");
            rd.escaped = 1;
            running = 0;
        }
        render_frame(&rd, &cam, &map, enemies);
        SDL_Delay(16);
    }
    render_free(&rd);
    map_free(&map);
    return (0);
}
