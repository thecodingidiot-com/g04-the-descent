#ifndef RAYCASTER_H
# define RAYCASTER_H

# include "vec2.h"
# include "camera.h"
# include "map.h"

# define PLANE_SCALE    0.66f

typedef struct s_hit
{
    float   perp_dist;
    int     side;
    float   wall_x;
    int     tex_id;
}   t_hit;

t_hit   raycaster_cast(t_camera const *cam, t_map const *map, int column);

/*
** The same DDA, aimed by hand instead of by a screen column. A weapon
** and a line of sight are both "what does this ray hit first", which is
** the question raycaster_cast already answers for every column of the
** screen sixty times a second.
*/
t_hit   raycaster_cast_dir(t_camera const *cam, t_map const *map, t_vec2 dir);

#endif
