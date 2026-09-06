#include <math.h>
#include "raycaster.h"

/*
** No angle, no cosf/sinf here at all -- ray_dir is built from two
** vectors already sitting in t_camera: forward (the centre of the
** view) and right, scaled by PLANE_SCALE to set the field of view.
** column_x walks from -1 (left edge of screen) to +1 (right edge),
** so ray_dir sweeps the whole FOV as one column-to-column blend of
** two vectors -- the same forward/right pair r01/r02's own camera
** already builds, no separate per-ray trig call.
*/
static t_vec2   ray_direction(t_camera const *cam, int column)
{
    float   column_x;
    t_vec2  plane;
    t_vec2  dir;

    column_x = 2.0f * (float)column / (float)WINDOW_W - 1.0f;
    plane = vec2_scale(cam->right, PLANE_SCALE);
    dir = vec2_add(cam->forward, vec2_scale(plane, column_x));
    return (dir);
}

static float    safe_inv(float v)
{
    if (v == 0.0f)
        return (1e30f);
    return (fabsf(1.0f / v));
}

/*
** DDA: instead of stepping the ray forward by tiny constant
** distances (slow, and can tunnel through a thin wall), step
** directly from one grid line to the next -- deltaDist is exactly
** how far along the ray one full grid cell costs in X or Y.
** Whichever axis is closer gets stepped; side records which face
** (a vertical X-side or a horizontal Y-side) the wall was actually
** hit on, which both fixes the fisheye distortion (perp_dist below
** is a straight subtraction, no cosine anywhere) and picks the
** texture.
*/
t_hit   raycaster_cast_dir(t_camera const *cam, t_map const *map, t_vec2 dir)
{
    t_vec2  delta;
    t_vec2  side_dist;
    int     map_x;
    int     map_y;
    int     step_x;
    int     step_y;
    t_hit   hit;

    delta.x = safe_inv(dir.x);
    delta.y = safe_inv(dir.y);
    map_x = (int)cam->pos.x;
    map_y = (int)cam->pos.y;
    if (dir.x < 0.0f) {
        step_x = -1;
        side_dist.x = (cam->pos.x - (float)map_x) * delta.x;
    } else {
        step_x = 1;
        side_dist.x = ((float)map_x + 1.0f - cam->pos.x) * delta.x;
    }
    if (dir.y < 0.0f) {
        step_y = -1;
        side_dist.y = (cam->pos.y - (float)map_y) * delta.y;
    } else {
        step_y = 1;
        side_dist.y = ((float)map_y + 1.0f - cam->pos.y) * delta.y;
    }
    hit.side = 0;
    while (1) {
        if (side_dist.x < side_dist.y) {
            side_dist.x += delta.x;
            map_x += step_x;
            hit.side = 0;
        } else {
            side_dist.y += delta.y;
            map_y += step_y;
            hit.side = 1;
        }
        if (map_is_wall(map, map_x, map_y))
            break ;
    }
    if (hit.side == 0)
        hit.perp_dist = side_dist.x - delta.x;
    else
        hit.perp_dist = side_dist.y - delta.y;
    if (hit.side == 0)
        hit.wall_x = cam->pos.y + hit.perp_dist * dir.y;
    else
        hit.wall_x = cam->pos.x + hit.perp_dist * dir.x;
    hit.wall_x -= floorf(hit.wall_x);
    if (hit.side == 0 && dir.x > 0.0f)
        hit.tex_id = 0;
    else if (hit.side == 0)
        hit.tex_id = 1;
    else if (dir.y > 0.0f)
        hit.tex_id = 2;
    else
        hit.tex_id = 3;
    return (hit);
}

/*
** The screen's version: one column of the view, aimed by
** ray_direction(). Everything below the aiming is shared with the
** weapon and with the enemies' line of sight, because "what does this
** ray hit first" is one question however it is asked.
*/
t_hit   raycaster_cast(t_camera const *cam, t_map const *map, int column)
{
    return (raycaster_cast_dir(cam, map, ray_direction(cam, column)));
}
