#include "stdio.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "backend.h"

// Note that "chao" is pronounced "cow" and referes to one fractal shape.
// "chaos" is plural of chao and is pronounced "cows".

//int scale;
int width;
int height;
int mask_cutoff = 1;

int clamp(int x, int max) {
    if (x < 0)
        return 0;
    if (x >= max)
        return max - 1;
    return x;
}

int eclamp(int x, int max) {
    if (x < 0)
        return -1;
    if (x >= max)
        return -1;
    return x;
}

float better_random() {
    //uint32_t x = mers_get_number();
    uint32_t x = rand();
    //return ((float) x) / ((float) UINT32_MAX);
    return ((float) x) / ((float) INT32_MAX);
}
int randint(int max) {
    // If max is not a power of 2, this may not be uniform
    //uint32_t x = mers_get_number();
    uint32_t x = rand();
    return (int) (x % max);
}

//==============
// Shapes
//==============
pair_t pent[5] = {{0.5, 0.0}, {0.9755282581475768, 0.3454915028125263}, 
                    {0.7938926261462367, 0.9045084971874737}, 
                    {0.2061073738537635, 0.9045084971874737}, 
                    {0.02447174185242318, 0.3454915028125264}};

pair_t hex[6] = {{0.5, 0.0},
                 {0.9330127018922193, 0.25},
                 {0.9330127018922194, 0.7499999999999999},
                 {0.5, 1.0},
                 {0.06698729810778076, 0.7500000000000002},
                 {0.0669872981077807, 0.24999999999999994}};

pair_t triangle[3] = {{0.5, 0.0},
                 {0.9330127018922194, 0.7499999999999999},
                 {0.06698729810778076, 0.7500000000000002}};
#define v 0.5
const float phi = 1.6180339887498948482045868;
const float gap = 0.13397459621;
//pair_t triangle[3] = {{0, gap/2}, {v, 1-gap/2}, {1, gap/2}};
pair_t corner_box[4] = {{.1,.1}, {.1, .9}, {.9, .9}, {.9, .1}};
pair_t edge_box[4] = {{.1, v}, {v, .9}, {.9, v}, {v, .1}};
pair_t eight_box[8] = {{0,0}, {0, v}, {0, 1}, {v, 1}, {1, 1}, {1, v}, {1, 0}, {v, 0}};
pair_t rot_eight_box[8] = {{0, v}, {0.25, 0.75}, {v, 1}, 
                           {0.75, 0.75}, {1, v}, {0.75, 0.25}, {v, 0}, {0.25, 0.25}};
pair_t center_box[5] = {{v, v}, {1, 0}, {1, 1}, {0, 1}, {0, 0}};

//=================
// Point choosers
//=================

int pc_any(int corner, int last_corner, int n) {
    return randint(n);
}

int pc_other(int corner, int last_corner, int n) {
    corner += randint(n - 1) + 1;
    corner = corner % n;
    return corner;
}

int pc_not_plus_two(int corner, int last_corner, int n) {
    int adder = randint(n - 1);
    if (adder >= 2)
        adder += 1;
    corner = (corner + adder) % n;
    return corner;
}
    
int pc_not_neighbor_if_double(int corner, int last_corner, int n) {
    if (corner == last_corner)
        return (corner + randint(n - 2) + 1) % n;
    return randint(n);
}

int pc_not_minus_one(int corner, int last_corner, int n) {
    return (corner + randint(n-1)) % n;
}

int pc_not_plus_two_or_three(int corner, int last_corner, int n) {
    int adder = randint(n-2);
    if (adder >= 2)
        adder += 2;
    return (corner + adder) % n;
}



chao_t* make_chao(pair_t* points, int n_points, point_chooser_t point_chooser, 
                    float jump, uint32_t color, int center_x, int center_y, int scale) {
    chao_t* chao = malloc(sizeof(chao_t));

    chao->points = malloc(sizeof(pair_t) * n_points);
    memcpy(chao->points, points, sizeof(pair_t) * n_points);

    chao->n_points = n_points;
    chao->point_chooser = point_chooser;
    chao->jump = jump;
    chao->color = color;
    chao->mask = NULL;
    chao->x = center_x - scale/2;
    chao->y = center_y - scale/2;
    chao->scale = scale;
    chao->target = calloc(width * height, sizeof(float));
    chao->color_target = calloc(width * height, sizeof(uint32_t));
    chao->other_mask = calloc(width * height, sizeof(unsigned char));
    chao->needs_mask_redraw = 0;
    return chao;
}





chao_t* make_chao_arr(pair_t* points, int n_points, int* arr, int n,
                    float jump, uint32_t color, int center_x, int center_y, int scale) {
    chao_t* chao = make_chao(points, n_points, NULL, jump, color, center_x, center_y, scale);
    chao->point_chooser_array = arr;
    chao->n_point_chooser_array = n;
    return chao;
}


typedef struct {
    int* arr;
    int len;
} int_array_len_t;

int_array_len_t binary_to_chooser_array(uint32_t n) {

    uint32_t number_of_ones = 0;
    for (uint32_t temp = n; temp; temp >>= 1) {
        number_of_ones += temp & 1u;
    }
    int* res = malloc(sizeof(int) * number_of_ones);
    int j = 0;
    int i = 0;
    for (int temp = n; temp; temp >>= 1, i++) {
        if (temp & 1u) {
            res[j++] = i;
        }
    }
    //printf("%d %d %d\n", j, number_of_ones, i);
    return (int_array_len_t) {res, number_of_ones};
}


chao_t* make_chao_bin(pair_t* points, int n_points, int magic_number,
                    float jump, uint32_t color, int center_x, int center_y, int scale) {
    int_array_len_t int_arr_len = binary_to_chooser_array(magic_number);
    chao_t* chao = make_chao(points, n_points, NULL, jump, color, center_x, center_y, scale);
    chao->point_chooser_array = int_arr_len.arr;
    chao->n_point_chooser_array = int_arr_len.len;
    return chao;
}


ipair_t convert_xy_to_ij_origin(float x, float y, chao_t* chao) {
    int i = eclamp(x * chao->scale, chao->scale);
    int j = eclamp(y * chao->scale, chao->scale);
    return (ipair_t) {i, j};
}

ipair_t convert_xy_to_ij_offset(float x, float y, chao_t* chao) {
    int i = eclamp(x * chao->scale + chao->x, width);
    int j = eclamp(y * chao->scale + chao->y, height);
    return (ipair_t) {i, j};
}

ipair_t convert_xy_to_ij_global(float x, float y, chao_t* chao) {
    int i = eclamp(x * chao->scale, width);
    int j = eclamp(y * chao->scale, height);
    return (ipair_t) {i, j};
}

ipair_t convert_mask_ij_to_global_ij(int i, int j, chao_t* chao) {
    int i2 = eclamp(i + chao->x, width);
    int j2 = eclamp(j + chao->y, height);
    return (ipair_t) {i2, j2};
}

// TODO use
const int number_prep_iterations = 6000000;

void make_mask(chao_t* chao) {
    if (chao->mask != NULL)
        free(chao->mask);
    chao->mask = calloc(chao->scale * chao->scale, sizeof(unsigned char));
    printf("scale: %d\n", chao->scale);

    for (int run = 0; run < 1000000; run++) {
        float x = better_random();
        float y = better_random();
        int corner = randint(chao->n_points);
        int last_corner = corner;
        for (int i = 0; i < 10; i++) {
            int temp = corner;
            // this could use optimization
            if (chao->point_chooser == NULL) {
                corner += chao->point_chooser_array[randint(chao->n_point_chooser_array)];
                corner = corner % chao->n_points;
            } else {
                corner = chao->point_chooser(corner, last_corner, chao->n_points);
            }
            last_corner = temp;
            pair_t corner_xy = chao->points[corner];
            x += (corner_xy.x - x) * chao->jump;
            y += (corner_xy.y - y) * chao->jump;
        }
        // this is/was pretty expensive too
        int i = eclamp(x * chao->scale, chao->scale);
        int j = eclamp(y * chao->scale, chao->scale);
        if (i == -1 || j == -1)
            continue;
        unsigned char* m = &chao->mask[i + j*chao->scale];
        *m = clamp(*m+1, 256);
    }
    chao->needs_mask_redraw = 0;
}

void add_other_to_other_mask(chao_t* chao, chao_t* other) {
    for (int j = 0; j < other->scale; j++) {
        for (int i = 0; i < other->scale; i++) {
            ipair_t gc = convert_mask_ij_to_global_ij(i, j, other);
            if (gc.i == -1 || gc.j == -1)
                continue;
            // add (with clamp) the other's mask value to my mask
            unsigned char* p = &chao->other_mask[gc.i + gc.j * width];
            *p = clamp(*p + other->mask[i + j * other->scale], 256);
        }
    }
}

void one_batch_to_target(chao_t* chao) {
    for (int run = 0; run < 100; run++) {
        float x = 20 * better_random() - 10;
        float y = 20 * better_random() - 10;
        int corner = randint(chao->n_points);
        int last_corner = corner;
        ipair_t next_coords;
        float next_x;
        float next_y;
        for (int iter = 0; iter < 20; iter++) {
            int temp = corner;
            if (chao->point_chooser == NULL) {
                corner += chao->point_chooser_array[randint(chao->n_point_chooser_array)];
                corner = corner % chao->n_points;
            } else {
                corner = chao->point_chooser(corner, last_corner, chao->n_points);
            }
            last_corner = temp;
            pair_t corner_xy = chao->points[corner];
            next_x = x + (corner_xy.x - x) * chao->jump;
            next_y = y + (corner_xy.y - y) * chao->jump;
            int i = eclamp(next_x * chao->scale + chao->x, width);
            int j = eclamp(next_y * chao->scale + chao->y, height);
            // if we're outside bounds or if we're on a blank spot in other_mask, then iterate
            if (i == -1 || j == -1 || chao->other_mask[i + j * width] <= mask_cutoff) {
                x = next_x;
                y = next_y;
            }
            //ipair_t coords = convert_xy_to_ij_offset(x, y, chao);
            //if (coords.i == -1 || coords.j == -1)
            //    continue;
            //chao->target[coords.i + coords.j*width] += 1;
        }
        int i = eclamp(x * chao->scale + chao->x, width);
        int j = eclamp(y * chao->scale + chao->y, height);
        if (i == -1 || j == -1)
            continue;
        chao->target[i + j*width] += 1;
    }
}


unsigned char double_inverse_add(unsigned char a, unsigned char b) {
    return clamp(256.0*(1- ((1 - a / 256.0) * (1 - b /256.0))), 256);
}

void draw_target(chao_t* chao) {
    float max = 0;
    for (int j = 1; j < height-1; j++) {
        for (int i = 1; i < width-1; i++) {
            float scale = chao->target[i + j * width];
            if (scale > max)
                max = scale;
        }
    }
    uint32_t* p = backend_get_pixel_pointer(0, 0);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            unsigned char* tar = (void*) &p[i + (height - j) * width];
            unsigned char* color = (void*) &(chao->color);
            float scale = chao->target[i + j * width];
            if (scale > 0) {
                for (int n = 0; n < 3; n++) {
                    tar[n] = clamp(tar[n] + color[n] * 1.5 * scale/max, 256);
                    //if (scale != 0)
                    //    tar[n] = double_inverse_add(tar[n], color[n] * 1.0);// * scale/max);
                }
                tar[3] = 0xff;
            }
        }
    }
}

void draw_other_mask(chao_t* chao) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            uint32_t* p = backend_get_pixel_pointer(i, j);
            unsigned char m = chao->other_mask[i + j*width];
            if (m > mask_cutoff)
                *p = 0xffffffff;
        }
    }
}

void clear_target(chao_t* chao) {
    free(chao->target); 
    chao->target = calloc(width * height, sizeof(float));
}
void clear_other_mask(chao_t* chao) {
    free(chao->other_mask); 
    chao->other_mask = calloc(width * height, sizeof(unsigned char));
}

void cross_all_other_masks(chao_t** chaos, int n_chaos) {
    for (int i = 0; i < n_chaos; i++) {
        clear_other_mask(chaos[i]);
    }
    for (int i = 0; i < n_chaos; i++) {
        for (int j = 0; j < n_chaos; j++) {
            if (i == j)
                continue;
            add_other_to_other_mask(chaos[i], chaos[j]);
        }
    }
}

// from https://digitalsynopsis.com/design/minimal-web-color-palettes-combination-hex-code/
const uint32_t c_white = 0xffffff;
const uint32_t c_green = 0xa8e6ce;
const uint32_t c_yellow = 0xdcedc2;
const uint32_t c_orange = 0xffd3b5;
const uint32_t c_pink = 0xffaaa6;
const uint32_t c_red = 0xff8c94;
const uint32_t c_teal = 0x0a91ab;
const uint32_t c_yellow2 = 0xffc045;
const uint32_t c_rose = 0xff0000;
const uint32_t c_cyan = 0x00c0c0;

// 0xAARRGGBB
int main(int argc, char** argv) {
    if (argc == 1) {
        height = 600;
        printf("fuck you\n");
    } else {
        height = atoi(argv[1]);
    }
    width = height * 1.9;
    //width = height;

    backend_init(width, height, 30);
    //mers_initialize(5489);
    srand((unsigned) 5489);


    //make_mask(foo);

    //chao_t* foo = make_chao(edge_box, 4, pc_not_minus_one, 0.5, c_teal, 
    //chao_t* bar = make_chao(hex, 6, pc_any, 0.5, c_yellow2, 
    //chao_t* bar = make_chao(corner_box, 4, pc_not_plus_two, 0.5, c_yellow2, 
    //chao_t* bar = 
    //make_mask(bar);

    //int w = width * 0.15;
    //int h = height * 0.25;
    //int s = width * 0.15;
    int w = width * 0.12;
    int h = height * 0.11;
    int s = width * 0.05;

    
    const int nnn = 64;
    chao_t* chaos[nnn-1];
    for (int i = 0; i < nnn-1; i++) {
        chaos[i] = make_chao_bin(hex, 6, i+1, 0.5, c_white, (((i+1)%8)+1)*w, height-(((i+1)/8)+1)*h, s);
    }
    int n_chaos = sizeof(chaos)/sizeof(chaos[0]);

    for (int i = 0; i < n_chaos; i++) {
        make_mask(chaos[i]);
    }

    // add every other chao's mask to each chao's other_mask
    cross_all_other_masks(chaos, n_chaos);

    printf("Ready\n");

    int render_choice = 0;
    uint32_t hit_clear = 0;
    while (1) {
        for (int i = 0; i < n_chaos; i++)
            one_batch_to_target(chaos[i]);
        if (backend_is_render_ready()) {
            backend_clear_screen();
            if (render_choice == 0) {
                for (int i = 0; i < n_chaos; i++)
                    draw_target(chaos[i]);
            } else if (render_choice == 1) {
                for (int i = 0; i < n_chaos; i++)
                    draw_other_mask(chaos[i]);
            }
            backend_loop(&mask_cutoff, &render_choice, &hit_clear, chaos, n_chaos);
            // code for check mask redraw
            if (hit_clear & 2) {
                for (int i = 0; i < n_chaos; i++) {
                    printf("fudsk me4\n");
                    if (chaos[i]->needs_mask_redraw) {
                        make_mask(chaos[i]);
                    }
                }
            }
            // code for remake other_masks and clear targets
            if (hit_clear & 1) {
                cross_all_other_masks(chaos, n_chaos);
                for (int i = 0; i < n_chaos; i++)
                    clear_target(chaos[i]);
            }
            hit_clear = 0;
        }
    }
}
