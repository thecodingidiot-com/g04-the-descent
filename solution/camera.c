#include <math.h>
#include "camera.h"

/*
** The map's y axis points DOWN -- row 0 is north -- and that decides
** the sign here. Standing on such a map facing east, (1, 0), your
** right hand points south, (0, 1). So right is forward rotated by
** (-fy, fx), not (fy, -fx): the latter yields north, which is the
** player's LEFT.
**
** raycaster.c builds the camera plane out of this vector, so getting
** it backwards does not tilt the view or skew it -- it mirrors the
** whole screen, left for right, in a way that looks completely
** plausible until you compare what you see against the minimap in the
** corner, which draws the same camera's heading correctly.
*/
static void camera_rebuild_axes(t_camera *cam)
{
    cam->forward.x = cosf(cam->angle);
    cam->forward.y = sinf(cam->angle);
    cam->right.x = -cam->forward.y;
    cam->right.y = cam->forward.x;
}

void    camera_init(t_camera *cam, float x, float y, float angle)
{
    cam->pos.x = x;
    cam->pos.y = y;
    cam->angle = angle;
    camera_rebuild_axes(cam);
}

void    camera_turn(t_camera *cam, float delta_angle)
{
    cam->angle += delta_angle;
    camera_rebuild_axes(cam);
}

void    camera_move(t_camera *cam, float delta_forward)
{
    cam->pos = vec2_add(cam->pos, vec2_scale(cam->forward, delta_forward));
}
