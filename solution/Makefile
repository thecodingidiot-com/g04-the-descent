CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -D_REENTRANT -I libtci
LDFLAGS = $(shell sdl2-config --libs) -lm
SRCS    = main.c map.c vec2.c camera.c raycaster.c scaler.c enemy.c weapon.c render.c
OBJS    = $(SRCS:.c=.o)
LIBS    = libtci/libtci.a libtci/libtciutil.a
NAME    = descent

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJS) $(LIBS)
	$(CC) $(OBJS) $(LIBS) $(LDFLAGS) -o $(NAME)

$(LIBS):
	$(MAKE) -C libtci

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C libtci clean
	rm -f $(OBJS)

fclean: clean
	$(MAKE) -C libtci fclean
	rm -f $(NAME)

re: fclean all
