#include <math.h>
#include "raycaster.h"
#include "weapon.h"

/*
** How far along the ray does this enemy sit, and how far off it? The
** dot product with the ray direction gives the distance along; the
** perpendicular offset is what decides whether the shot passes close
** enough to count. A billboard has width, so a shot that misses the
** centre by a little still hits.
*/
static int  ray_hits_enemy(t_vec2 origin, t_vec2 dir, t_vec2 target,
        float *along)
{
    t_vec2  rel;
    float   perp;

    rel = vec2_sub(target, origin);
    *along = vec2_dot(rel, dir);
    if (*along <= 0.0f || *along > WEAPON_RANGE)
        return (0);
    perp = fabsf(rel.x * dir.y - rel.y * dir.x);
    return (perp <= WEAPON_SPREAD);
}

/*
** Returns the index of the enemy hit, or -1.
**
** The wall is checked first and once: the same cast the renderer runs
** for the centre column, giving the distance to whatever is directly
** ahead. Anything further along the ray than that wall is behind it,
** so an enemy through a doorway two rooms away cannot be shot through
** the doorframe.
*/
int weapon_fire(t_camera const *cam, t_map const *map,
        t_enemy list[MAX_ENEMIES])
{
    t_hit   wall;
    float   along;
    float   best;
    int     best_i;
    int     i;

    wall = raycaster_cast_dir(cam, map, cam->forward);
    best = WEAPON_RANGE;
    best_i = -1;
    i = 0;
    while (i < MAX_ENEMIES) {
        if (list[i].alive
            && ray_hits_enemy(cam->pos, cam->forward, list[i].pos, &along)
            && along < wall.perp_dist && along < best) {
            best = along;
            best_i = i;
        }
        i++;
    }
    if (best_i >= 0) {
        list[best_i].health--;
        if (list[best_i].health <= 0)
            list[best_i].alive = 0;
    }
    return (best_i);
}
