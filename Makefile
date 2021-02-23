.PHONY: chaos-game

#opt = -g -O1
opt = -Ofast

chaos-game:
	gcc $(opt) -o backend.o -c -std=c99 backend.c
	gcc $(opt) -o chaos-game -std=c99 `sdl2-config --cflags --libs` backend.o chaos-game.c
