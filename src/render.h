#include "raylib.h"
#include "raylib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SCREENWIDTH 1280
#define SCREENHEIGHT 720


typedef struct{
    Rectangle r;
    _Bool is_shaped;
    char* str;
    unsigned long long  strlen;
}textfield;

typedef struct {
    textfield rows;
    textfield colls;
    textfield btn_make;
    textfield btn_clear;
    textfield btn_solve;
} up_menu;

typedef struct {
    double** data;
    textfield** tf;
    unsigned long long n_tf;
    up_menu menu;
}
matrix;

void handle_input(matrix* m, int chosen);
int check_collision(matrix* m);
void draw_graphics(matrix* m, int scr_w, int scr_h, int chosen, _Bool mouse_button_pressed);
void make_tf(matrix* m, int scr_w, int scr_h);
void init_m(matrix* m);
void destroy_data(matrix* m);
void make_m(matrix *m);
void transfer_data(matrix* m);