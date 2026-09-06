#include <math.h>
#include "enemy.h"
#include "raycaster.h"

/*
** Enemies are placed at fixed cells rather than scattered at random:
** a maze you can learn is the whole premise, and an encounter that
** moves between runs would make learning it pointless.
*/
static const float  g_spawn[MAX_ENEMIES][2] = {
    {37.5f, 9.5f}, {62.5f, 9.5f}, {12.5f, 28.5f}, {37.5f, 28.5f},
    {62.5f, 28.5f}, {12.5f, 47.5f}, {37.5f, 47.5f}, {62.5f, 47.5f}
};

void    enemy_reset(t_enemy list[MAX_ENEMIES], t_map const *map)
{
    int i;

    i = 0;
    while (i < MAX_ENEMIES) {
        list[i].pos.x = g_spawn[i][0];
        list[i].pos.y = g_spawn[i][1];
        list[i].health = ENEMY_HEALTH;
        list[i].cooldown = 0;
        list[i].firing = 0;
        /* refuse to spawn inside a wall, whatever the table says */
        list[i].alive = !map_is_wall(map, (int)list[i].pos.x, (int)list[i].pos.y);
        i++;
    }
}

/*
** Line of sight is the same cast the renderer uses, aimed along the
** line between us. If the first wall the ray meets is further away than
** the player is, nothing is in the way.
**
** Note what this does NOT do: it does not walk the grid itself, and it
** does not approximate with a distance check. The renderer already owns
** an exact answer to "where is the first wall along this ray", so the
** enemy asks it rather than keeping a second, subtly different opinion.
*/
int enemy_can_see(t_enemy const *e, t_camera const *cam, t_map const *map)
{
    t_vec2  to_player;
    t_vec2  dir;
    float   dist;
    t_camera from;
    t_hit   hit;

    to_player = vec2_sub(cam->pos, e->pos);
    dist = sqrtf(vec2_dot(to_player, to_player));
    if (dist > ENEMY_SIGHT)
        return (0);
    if (dist < 0.0001f)
        return (1);
    dir.x = to_player.x / dist;
    dir.y = to_player.y / dist;
    from = *cam;
    from.pos = e->pos;
    hit = raycaster_cast_dir(&from, map, dir);
    return (hit.perp_dist >= dist);
}

/*
** Move toward the player, one axis resolved at a time so an enemy
** slides along a wall instead of pressing into it -- the same
** resolution the player uses, for the same reason.
*/
static void step_toward(t_enemy *e, t_camera const *cam, t_map const *map)
{
    t_vec2  to_player;
    float   dist;
    float   nx;
    float   ny;

    to_player = vec2_sub(cam->pos, e->pos);
    dist = sqrtf(vec2_dot(to_player, to_player));
    if (dist < ENEMY_FIRE_MIN || dist < 0.0001f)
        return ;
    nx = e->pos.x + to_player.x / dist * ENEMY_SPEED;
    ny = e->pos.y + to_player.y / dist * ENEMY_SPEED;
    if (!map_is_wall(map, (int)nx, (int)e->pos.y))
        e->pos.x = nx;
    if (!map_is_wall(map, (int)e->pos.x, (int)ny))
        e->pos.y = ny;
}

/* Returns total damage dealt to the player this frame. */
int enemy_update(t_enemy list[MAX_ENEMIES], t_camera const *cam,
        t_map const *map)
{
    int i;
    int damage;

    damage = 0;
    i = 0;
    while (i < MAX_ENEMIES) {
        list[i].firing = 0;
        if (list[i].alive) {
            if (list[i].cooldown > 0)
                list[i].cooldown--;
            if (enemy_can_see(&list[i], cam, map)) {
                step_toward(&list[i], cam, map);
                if (list[i].cooldown == 0) {
                    list[i].cooldown = ENEMY_COOLDOWN;
                    list[i].firing = 1;
                    damage += ENEMY_DAMAGE;
                }
            }
        }
        i++;
    }
    return (damage);
}
