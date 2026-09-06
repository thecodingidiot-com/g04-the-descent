#include <math.h>
#include <stdlib.h>
#include "raycaster.h"
#include "render.h"
#include "scaler.h"

int render_init(t_render *rd)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return (0);
    rd->win = SDL_CreateWindow("The Descent", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, 0);
    if (!rd->win)
        return (0);
    /* Acceleration where it exists, software where it does not -- a
    ** headless box has no accelerated renderer, and a NULL one turns
    ** every draw call into a silent no-op. */
    rd->ren = SDL_CreateRenderer(rd->win, -1, SDL_RENDERER_ACCELERATED);
    if (!rd->ren)
        rd->ren = SDL_CreateRenderer(rd->win, -1, SDL_RENDERER_SOFTWARE);
    if (!rd->ren)
        return (0);
    rd->health = 100;
    rd->escaped = 0;
    return (1);
}

void    render_free(t_render *rd)
{
    if (rd->ren)
        SDL_DestroyRenderer(rd->ren);
    if (rd->win)
        SDL_DestroyWindow(rd->win);
    SDL_Quit();
}

static void draw_background(t_render *rd)
{
    SDL_Rect    r;

    SDL_SetRenderDrawColor(rd->ren, 24, 26, 34, 255);
    r.x = 0; r.y = 0; r.w = WINDOW_W; r.h = WINDOW_H / 2;
    SDL_RenderFillRect(rd->ren, &r);
    SDL_SetRenderDrawColor(rd->ren, 40, 36, 32, 255);
    r.y = WINDOW_H / 2; r.h = WINDOW_H - WINDOW_H / 2;
    SDL_RenderFillRect(rd->ren, &r);
}

/*
** The wall pass, and the only place rd->depth is written. Distance goes
** into the buffer for every column, whether or not anything will ever
** read it -- a buffer with holes in it is worse than none, because the
** holes are exactly the columns a sprite would be wrongly drawn over.
*/
static void draw_walls(t_render *rd, t_camera const *cam, t_map const *map)
{
    int     column;
    t_hit   hit;
    float   perp;
    int     line_h;
    int     shade;
    SDL_Rect dst;

    column = 0;
    while (column < WINDOW_W) {
        hit = raycaster_cast(cam, map, column);
        perp = hit.perp_dist;
        if (perp < 1.0f / 8.0f)
            perp = 1.0f / 8.0f;
        rd->depth[column] = perp;
        line_h = (int)((float)WINDOW_H * WALL_HEIGHT / perp);
        if (line_h > WINDOW_H * 4)
            line_h = WINDOW_H * 4;
        shade = (int)(210.0f / (1.0f + perp * 0.10f));
        if (hit.side == 1)
            shade = shade * 7 / 10;
        SDL_SetRenderDrawColor(rd->ren, (Uint8)shade, (Uint8)(shade * 9 / 10),
            (Uint8)(shade * 7 / 10), 255);
        dst.x = column;
        dst.y = WINDOW_H / 2 - line_h / 2;
        dst.w = 1;
        dst.h = line_h;
        SDL_RenderFillRect(rd->ren, &dst);
        column++;
    }
}

/*
** Sprites, far to near, each occluded per column against the buffer.
**
** Sorting matters because two sprites can overlap each other, and the
** depth buffer only arbitrates between a sprite and the WALLS -- it is
** written once by the wall pass and never updated by the sprites, so a
** nearer enemy drawn first would be painted over by a further one.
** Draw the far ones first and the near ones cover them, which is the
** painter's algorithm doing the part the buffer does not.
*/
/*
** The sprite and its state travel together through the sort. Keeping
** the projection in one array and the "is it firing" flag in a parallel
** one works right up until the sort reorders one and not the other,
** at which point a dead enemy's muzzle flash appears on a live one.
*/
typedef struct s_sprite
{
    t_projection    proj;
    int             firing;
}   t_sprite;

static int  cmp_far_first(void const *a, void const *b)
{
    t_sprite const *pa = a;
    t_sprite const *pb = b;

    if (pa->proj.depth < pb->proj.depth)
        return (1);
    if (pa->proj.depth > pb->proj.depth)
        return (-1);
    return (0);
}

static void draw_enemies(t_render *rd, t_camera const *cam,
        t_enemy const *enemies)
{
    t_sprite        sprite[MAX_ENEMIES];
    int             count;
    int             i;
    int             x;
    SDL_Rect        col;

    count = 0;
    i = 0;
    while (i < MAX_ENEMIES) {
        if (enemies[i].alive) {
            sprite[count].proj = scaler_project(cam, enemies[i].pos);
            if (sprite[count].proj.visible) {
                sprite[count].firing = enemies[i].firing;
                count++;
            }
        }
        i++;
    }
    qsort(sprite, count, sizeof(t_sprite), cmp_far_first);
    i = 0;
    while (i < count) {
        x = sprite[i].proj.screen_x;
        while (x < sprite[i].proj.screen_x + sprite[i].proj.size) {
            /* One column at a time, because occlusion is per column:
            ** half a sprite behind a corner is the normal case, not an
            ** edge case. */
            if (sprite_column_visible(rd->depth, x, sprite[i].proj.depth)) {
                if (sprite[i].firing)
                    SDL_SetRenderDrawColor(rd->ren, 240, 190, 90, 255);
                else
                    SDL_SetRenderDrawColor(rd->ren, 170, 60, 60, 255);
                col.x = x;
                col.y = sprite[i].proj.screen_y;
                col.w = 1;
                col.h = sprite[i].proj.size;
                SDL_RenderFillRect(rd->ren, &col);
            }
            x++;
        }
        i++;
    }
}

static void draw_hud(t_render *rd)
{
    SDL_Rect    bar;

    bar.x = 16;
    bar.y = WINDOW_H - 28;
    bar.w = rd->health * 2;
    bar.h = 12;
    if (bar.w < 0)
        bar.w = 0;
    SDL_SetRenderDrawColor(rd->ren, 200, 60, 60, 255);
    SDL_RenderFillRect(rd->ren, &bar);
    /* the crosshair: the exact column the weapon casts down */
    SDL_SetRenderDrawColor(rd->ren, 230, 230, 230, 255);
    bar.x = WINDOW_W / 2 - 1;
    bar.y = WINDOW_H / 2 - 6;
    bar.w = 2;
    bar.h = 12;
    SDL_RenderFillRect(rd->ren, &bar);
}

void    render_frame(t_render *rd, t_camera const *cam, t_map const *map,
        t_enemy const *enemies)
{
    draw_background(rd);
    draw_walls(rd, cam, map);
    draw_enemies(rd, cam, enemies);
    draw_hud(rd);
    SDL_RenderPresent(rd->ren);
}
