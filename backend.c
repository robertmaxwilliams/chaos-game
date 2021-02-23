#include "stdio.h"
#include "SDL2/SDL.h"
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include "types.h"
#include "backend.h"


uint32_t reverse_bytes_32(uint32_t x) {
    unsigned char* bgra = (void*) &x;
    unsigned char out[4];
    // ...,  ..., ..., alpha
    out[0] = bgra[2];
    out[1] = bgra[1];
    out[2] = bgra[0];
    out[3] = bgra[3];
    return *((uint32_t*) out);
}

struct pollfd stdin_poll = { .fd = STDIN_FILENO
                           , .events = POLLIN | POLLRDBAND | POLLRDNORM | POLLPRI };

uint32_t increment_color(uint32_t x) {
    unsigned char* bgra = (void*) &x;
    if (bgra[0] == 255) {
        //bgra[1] -= 1;
        //bgra[2] -= 1;
    } else {
        for (int i = 0; i < 3; i++) {
            bgra[i] += 5;
        }
    }
    bgra[3] = 255;
    return x;
}

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture* texture;
uint32_t* pixels_array;
uint32_t* image_array;
int width;
int height;
uint32_t last_render;

float upsample = 0.5;
int fps;
int frame_ms;

void backend_clear_screen() {
    memset(pixels_array, 0, sizeof(uint32_t) * width * height);
    //for (int i = 0; i < width*height; i++) {
    //    //pixels_array[i] = 0x00ffffff;
    //    pixels_array[i] = 0x00000000;
    //}
}

void backend_init(int _width, int _height, int _fps) {
    width = _width;
    height = _height;
    fps = _fps;
    frame_ms = 1000 / fps;

    SDL_CreateWindowAndRenderer(upsample*width, upsample*height, 0, &window, &renderer);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
                                                SDL_TEXTUREACCESS_STREAMING, width, height);

    pixels_array = malloc(sizeof(uint32_t) * width * height);
    image_array = malloc(sizeof(uint32_t) * width * height);
    backend_clear_screen();

    SDL_SetRenderDrawColor(renderer, 255, 30, 255, 255);

}


uint32_t backend_get_pixel(int x, int y) {
        return pixels_array[(x % width) +  ((height - y) % height) * width];
}

uint32_t* backend_get_pixel_pointer(int x, int y) {
        return &pixels_array[(x % width) +  ((height - y) % height) * width];
}

void backend_set_pixel(int x, int y, uint32_t bgra) {
        pixels_array[(x % width) +  ((height - y) % height) * width] = bgra;
}


void backend_quit() {
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    exit(0);
}


int backend_is_render_ready() {
    uint32_t now = SDL_GetTicks();
    return now - last_render > frame_ms - 1;
}

void scale_and_correct(chao_t* chao, float scale) {
    int old_scale = chao->scale;
    chao->scale *= scale;
    int new_scale = chao->scale;
    chao->x += (old_scale - new_scale) / 2;
    chao->y += (old_scale - new_scale) / 2;
}

        
    
    

int step = 1;
int dragged_square = -1;
int last_dragged_square = 0;
void backend_loop(int* mask_cutoff, int* render_choice, uint32_t* hit_clear, chao_t** chaos, 
                  int n_chaos) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        // exit from button
        if (event.type == SDL_QUIT) {
            backend_quit();
        }

        // y is negative because I'm using math convention for coordinates
        else if (event.type == SDL_MOUSEMOTION && dragged_square != -1) {
            chaos[dragged_square]->x += event.motion.xrel / upsample;
            chaos[dragged_square]->y -= event.motion.yrel / upsample;
            *hit_clear |= 1; 
        }
        else if (event.type == SDL_MOUSEBUTTONUP) {
            printf("mouse down %d\n", dragged_square);
            dragged_square = -1;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN) {
            int bx = event.button.x / upsample;
            int by = height - event.button.y / upsample;
            printf("mouse down %d %d\n", bx, by);
            for (int i = 0; i < n_chaos; i++) {
                int x = chaos[i]->x;
                int y = chaos[i]->y;
                int s = chaos[i]->scale;
                printf("%d <= %d < %d, %d <= %d < %d\n", x, bx, x+s, y, by, y+s);
                if (bx >= x && by >= y && bx < x+s && by < y+s) {
                    dragged_square = i;
                    break;
                }
            }
            last_dragged_square = dragged_square;
        }


        else if (event.type == SDL_KEYDOWN) {
            // exit from ESC or Q
            if ((event.key.keysym.sym == SDLK_ESCAPE) || (event.key.keysym.sym == SDLK_q)) {
                backend_quit();
            }
            // Save image and chaos-game source to images/
            if (event.key.keysym.sym == SDLK_s) {
                printf("Saviong\n");
                FILE *f = fopen("image.rgba", "wb");
                for (int i = 0; i < width*height; i++) {
                    image_array[i] = reverse_bytes_32(pixels_array[i]);
                    image_array[i] |= 0xff000000;
                }
                fwrite(image_array, sizeof(uint32_t), width*height, f);
                fclose(f);
                time_t t = time(NULL); struct tm tim = *localtime(&t);
                char* command = malloc(200);
                // convert image to unique filename
                sprintf(command, 
                        "convert -size %dx%d -depth 8 image.rgba \"images/image-%d-%02d-%02d-%02d-%02d-%02d.png\"",
                        width, height,
                        tim.tm_year + 1900, tim.tm_mon + 1, tim.tm_mday, tim.tm_hour, tim.tm_min,
                        tim.tm_sec);
                system(command);
                // copy chaos-game.c to unique place as well
                sprintf(command, 
                        "cp chaos-game.c \"images/chaos-game-%d-%02d-%02d-%02d-%02d-%02d.c\"",
                        tim.tm_year + 1900, tim.tm_mon + 1, tim.tm_mday, tim.tm_hour, tim.tm_min,
                        tim.tm_sec);
                system(command);
                free(command);
                printf("saved\n");
            }
            switch (event.key.keysym.sym) {
                case SDLK_EQUALS:
                    *mask_cutoff += 1; 
                    *hit_clear |= 1; 
                    break;
                case SDLK_MINUS:
                    *hit_clear |= 1; 
                    *mask_cutoff -= 1; 
                    break;
                case SDLK_SPACE:
                    *render_choice = (*render_choice + 1) % 2;
                    break;
                case SDLK_PERIOD:
                    step *= 1.5;
                    printf("step is %d\n", step);
                    break;
                case SDLK_COMMA:
                    step /= 1.5;
                    printf("step is %d\n", step);
                    break;

                case SDLK_j:
                    scale_and_correct(chaos[last_dragged_square], 1.25);
                    chaos[last_dragged_square]->needs_mask_redraw = 1;
                    *hit_clear |= 2 | 1; 
                    break;
                case SDLK_k:
                    scale_and_correct(chaos[last_dragged_square], 1/1.25);
                    chaos[last_dragged_square]->needs_mask_redraw = 1;
                    *hit_clear |= 2 | 1; 
                    break;

                case SDLK_u:
                    chaos[last_dragged_square]->jump -= 0.01;
                    printf("square %d has jump of %f\n", last_dragged_square, chaos[last_dragged_square]->jump);
                    chaos[last_dragged_square]->needs_mask_redraw = 1;
                    *hit_clear |= 1; 
                    break;
                case SDLK_i:
                    chaos[last_dragged_square]->jump += 0.01;
                    printf("square %d has jump of %f\n", last_dragged_square, chaos[last_dragged_square]->jump);
                    chaos[last_dragged_square]->needs_mask_redraw = 1;
                    *hit_clear |=  1; 
                    break;

                case SDLK_r:
                    for (int i = 0; i < n_chaos; i++) {
                        chaos[i]->jump -= 0.01;
                        chaos[i]->needs_mask_redraw = 1;
                    }
                    printf("square 0 has jump of %f\n", chaos[0]->jump);
                    *hit_clear |= 1; 
                    break;
                case SDLK_t:
                    for (int i = 0; i < n_chaos; i++) {
                        chaos[i]->jump += 0.01;
                        chaos[i]->needs_mask_redraw = 1;
                    }
                    printf("square 0 has jump of %f\n", chaos[0]->jump);
                    *hit_clear |=  1; 
                    break;

                case SDLK_n:
                    *hit_clear |= 2 | 1; 
                    break;

                case SDLK_LEFT:
                    chaos[last_dragged_square]->x -= step; 
                    *hit_clear |= 1; 
                    break;
                case SDLK_RIGHT:
                    chaos[last_dragged_square]->x += step; 
                    *hit_clear |= 1; 
                    break;
                case SDLK_UP:
                    chaos[last_dragged_square]->y += step; 
                    *hit_clear |= 1; 
                    break;
                case SDLK_DOWN:
                    chaos[last_dragged_square]->y -= step;
                    *hit_clear |= 1; 
                    break;
            }
        }
    }
    SDL_RenderClear(renderer);

    void *pixels; int pitch;
    uint32_t* pixels_u = pixels;
    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    memcpy(pixels, pixels_array, sizeof(uint32_t)*width*height); 
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // Wait to render so we have max 30 fps
    uint32_t now = SDL_GetTicks();
    int passed = now - last_render;
    if (passed < frame_ms) {
        SDL_Delay(frame_ms - passed);
    }

    last_render = SDL_GetTicks();
}

