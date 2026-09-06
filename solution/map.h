#ifndef MAP_H
# define MAP_H

# include "vec2.h"

# define WINDOW_W       800
# define WINDOW_H       608

/*
** How tall a wall is, in the same units a map cell is wide.
**
** r03 had no such constant -- its walls were implicitly one unit, and
** its hand-drawn rooms were three or four cells across, so a wall
** filled the view. g02c's maze is authored for a top-down camera, where
** a room is 25 x 19 cells because that is what fills an 800 x 608
** screen. Rendered at one unit tall, a wall twelve cells away is a
** sliver near the horizon and a room reads as an empty plain.
**
** The map did not change; what a cell MEANS did. A top-down renderer
** measures a room in screens, a first-person one measures it in wall
** heights, and the same file is a corridor to one and a field to the
** other.
*/
# define WALL_HEIGHT    3.0f

/*
** r03 held its grid in `char grid[32][33]` -- a fixed array, sized for
** the small hand-written rooms that chapter needed. g02c's maze is
** 75 x 57, so it does not fit, and this loader allocates instead.
**
** The file format is g02c's, unchanged: a `<width> <height>` line, then
** one character per tile. '#' wall, '.' floor, 'S' start, 'E' exit. It
** is the same maze; only the renderer standing in it is different.
*/
typedef struct s_map
{
    int     width;
    int     height;
    char    *tiles;
    t_vec2  start_pos;
    float   start_angle;
}   t_map;

int     map_load(t_map *map, char const *path);
void    map_free(t_map *map);
int     map_is_wall(t_map const *map, int x, int y);
int     map_is_exit(t_map const *map, int x, int y);

#endif
