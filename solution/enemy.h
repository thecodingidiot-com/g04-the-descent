#ifndef ENEMY_H
# define ENEMY_H

# include "camera.h"
# include "map.h"
# include "vec2.h"

# define MAX_ENEMIES     8
# define ENEMY_SPEED     0.022f
# define ENEMY_SIGHT     12.0f
# define ENEMY_FIRE_MIN  2.0f
# define ENEMY_COOLDOWN  90
# define ENEMY_DAMAGE    12
# define ENEMY_HEALTH    2

typedef struct s_enemy
{
    t_vec2  pos;
    int     alive;
    int     health;
    int     cooldown;
    int     firing;     /* set for the frame it shoots, for the renderer */
}   t_enemy;

/*
** An enemy is a position and a state, not a sprite. What it looks like
** is the renderer's problem; what it does is here.
*/
void    enemy_reset(t_enemy list[MAX_ENEMIES], t_map const *map);
int     enemy_can_see(t_enemy const *e, t_camera const *cam, t_map const *map);
int     enemy_update(t_enemy list[MAX_ENEMIES], t_camera const *cam,
            t_map const *map);

#endif
