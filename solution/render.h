#ifndef RENDER_H
# define RENDER_H

# include <SDL2/SDL.h>
# include "camera.h"
# include "enemy.h"
# include "map.h"

typedef struct s_render
{
    SDL_Window      *win;
    SDL_Renderer    *ren;
    /*
    ** One perpendicular distance per screen column, written while the
    ** walls are drawn and read while the enemies are.
    **
    ** r03 and r04 never keep this. They compute a column's distance,
    ** turn it into a height, draw the strip and forget it -- which is
    ** enough when walls are the only thing on screen, because nothing
    ** ever has to ask whether something else is nearer. The moment a
    ** sprite exists, that question has to be answerable per column, and
    ** the cheapest place to answer it is the pass that already knew.
    */
    float           depth[WINDOW_W];
    int             health;
    int             escaped;
}   t_render;

int     render_init(t_render *rd);
void    render_free(t_render *rd);
void    render_frame(t_render *rd, t_camera const *cam, t_map const *map,
            t_enemy const *enemies);

#endif
