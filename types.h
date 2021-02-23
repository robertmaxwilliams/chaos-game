#ifndef TYPES_GUARD
#define TYPES_GUARD

typedef struct {
    float x;
    float y;
} pair_t;

typedef struct {
    int i;
    int j;
} ipair_t;

typedef int (*point_chooser_t) (int,int,int);
typedef int ia[];

typedef struct {
    pair_t* points;
    int n_points;
    point_chooser_t point_chooser;
    int* point_chooser_array;
    int n_point_chooser_array;
    float jump;
    uint32_t color; 
    unsigned char* mask; // scale*scale, holds first order form
    int x; int y; int scale; // offset and scale for points and mask.
    float* target; // width*height, holds number of times each point has been hit
    uint32_t* color_target; // width*height, target gets rendered onto here
    unsigned char* other_mask; // width*height, holds first order form of all other chaos
    int needs_mask_redraw;
} chao_t;

#endif
