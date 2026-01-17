#include "SDL3/SDL.h"
typedef struct {
    SDL_Window* window; 
    SDL_Renderer* renderer;
} Sdlstate;

typedef struct{
    char text[32];
    _Bool selected;
    SDL_FRect rect;
}textfield;

typedef struct {
    textfield* tf;
    int rows;
    int cols;
    textfield rows_input;
    textfield cols_input;
    _Bool showmatrix;
}matrix_sdl;

typedef struct{
    double** data;
    matrix_sdl* in_out;
}matrix;

void cleanup(Sdlstate* State);
_Bool initialize_window(Sdlstate* State, const int width, const int height);