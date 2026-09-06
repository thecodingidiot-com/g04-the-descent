#include "map.h"
#include "scaler.h"

# define NEAR_PLANE 0.4f

/*
** Identical in shape to r01's scaler_project(): project into camera
** space with two dot products, then one divide turns depth into size
** and reuses itself for the horizontal position.
*/
t_projection    scaler_project(t_camera const *cam, t_vec2 world_pos)
{
    t_projection    proj;
    t_vec2          rel;

    rel = vec2_sub(world_pos, cam->pos);
    proj.depth = vec2_dot(rel, cam->forward);
    proj.side = vec2_dot(rel, cam->right);
    if (proj.depth < NEAR_PLANE) {
        proj.visible = 0;
        return (proj);
    }
    proj.visible = 1;
    proj.size = (int)((float)WINDOW_H * WALL_HEIGHT / proj.depth / 2.0f);
    proj.screen_x = WINDOW_W / 2
        + (int)((float)WINDOW_H * proj.side / proj.depth) - proj.size / 2;
    proj.screen_y = WINDOW_H / 2 - proj.size / 2;
    return (proj);
}

int sprite_column_visible(float const *depth, int x, float sprite_depth)
{
    if (x < 0 || x >= WINDOW_W)
        return (0);
    return (sprite_depth < depth[x]);
}
