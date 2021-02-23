#ifndef BACKEND_GUARD
#define BACKEND_GUARD

#include <stdint.h>

void backend_init(int _width, int _height, int _fps);
void backend_clear_screen();
uint32_t backend_get_pixel(int x, int y);
uint32_t* backend_get_pixel_pointer(int x, int y);
void backend_set_pixel(int x, int y, uint32_t bgra);
int backend_is_render_ready();
void backend_loop(int* mask_cutoff, int* render_choice, uint32_t* hit_clear, chao_t** chaos, 
                  int n_chaos);
void backend_quit();





#endif
