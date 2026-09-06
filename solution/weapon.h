#ifndef WEAPON_H
# define WEAPON_H

# include "camera.h"
# include "enemy.h"
# include "map.h"

# define WEAPON_RANGE     14.0f
# define WEAPON_COOLDOWN  18
# define WEAPON_SPREAD    0.35f

/*
** Hitscan, not a projectile. g03's shots were sprites that travelled
** `+x` a little each frame -- which worked there because that game's
** camera never rotated, and its own comment says so. A raycaster
** rotates freely, so a shot stops being a thing that moves and becomes
** a question asked once: what is first along this line?
**
** raycaster_cast_dir() already answers exactly that.
*/
int weapon_fire(t_camera const *cam, t_map const *map,
        t_enemy list[MAX_ENEMIES]);

#endif
