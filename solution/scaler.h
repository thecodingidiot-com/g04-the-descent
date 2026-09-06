#ifndef SCALER_H
# define SCALER_H

# include "camera.h"
# include "vec2.h"

/*
** r01's billboard projection, returning to solve a raycaster's problem.
**
** The scaler era faked depth by dividing: a thing twice as far away is
** drawn half as tall. The raycaster era does the same division per wall
** column. An enemy in a raycast world is neither -- it is a flat sprite
** that has to agree with walls about how far away it is -- and the way
** to make it agree is to project it exactly the way r01 projected its
** billboards, then let the depth buffer arbitrate.
**
** Two differences from r01. The horizon is WINDOW_H / 2 rather than a
** tuned constant, because a raycaster's centre line is where its walls
** are centred. And `side` is kept, because deciding which screen
** columns a sprite covers needs it.
*/
typedef struct s_projection
{
    float   depth;
    float   side;
    int     visible;
    int     size;
    int     screen_x;
    int     screen_y;
}   t_projection;

t_projection    scaler_project(t_camera const *cam, t_vec2 world_pos);

/*
** Is a sprite column visible, given the wall distance recorded for that
** screen column? This is the entire occlusion rule, and it is a pure
** function of two numbers so that it can be asserted without a window.
**
** Off-screen counts as not visible, which keeps the caller's loop free
** of bounds checks -- a sprite half off the edge is the normal case.
*/
int             sprite_column_visible(float const *depth, int x,
                    float sprite_depth);

#endif
