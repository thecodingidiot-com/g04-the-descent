#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "libtci.h"
#include "map.h"

/*
** Out of bounds is a wall. Every consumer -- the DDA, the collision
** test, the enemies' line of sight -- then needs no edge case at all,
** because the world is simply solid out there.
*/
int map_is_wall(t_map const *map, int x, int y)
{
    if (x < 0 || x >= map->width || y < 0 || y >= map->height)
        return (1);
    return (map->tiles[y * map->width + x] == '#');
}

int map_is_exit(t_map const *map, int x, int y)
{
    if (x < 0 || x >= map->width || y < 0 || y >= map->height)
        return (0);
    return (map->tiles[y * map->width + x] == 'E');
}

static int  parse_dimensions(int fd, int *width, int *height)
{
    char    *line;
    char    *space;

    line = tci_getline(fd);
    if (!line)
        return (0);
    space = tci_strchr(line, ' ');
    if (!space) {
        free(line);
        return (0);
    }
    *space = '\0';
    *width = tci_atoi(line);
    *height = tci_atoi(space + 1);
    free(line);
    return (*width > 0 && *height > 0);
}

int map_load(t_map *map, char const *path)
{
    int     fd;
    char    *line;
    int     y;
    int     x;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        tci_printf("error: cannot open '%s'\n", path);
        return (0);
    }
    if (!parse_dimensions(fd, &map->width, &map->height)) {
        tci_printf("error: '%s' does not start with '<width> <height>'\n", path);
        close(fd);
        return (0);
    }
    map->tiles = tci_calloc(map->width * map->height, sizeof(char));
    if (!map->tiles) {
        close(fd);
        return (0);
    }
    /* Facing east by default; the maze's own start cell decides where. */
    map->start_pos.x = 1.5f;
    map->start_pos.y = 1.5f;
    map->start_angle = 0.0f;
    y = 0;
    while (y < map->height && (line = tci_getline(fd)) != NULL) {
        x = 0;
        while (x < map->width && line[x] && line[x] != '\n') {
            if (line[x] == 'S') {
                map->start_pos.x = (float)x + 0.5f;
                map->start_pos.y = (float)y + 0.5f;
            }
            map->tiles[y * map->width + x] = line[x];
            x++;
        }
        free(line);
        y++;
    }
    close(fd);
    return (y == map->height);
}

void    map_free(t_map *map)
{
    free(map->tiles);
    map->tiles = NULL;
}
