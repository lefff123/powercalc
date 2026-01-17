#include "render.h"
#include "calc.h"
#include <stdio.h>
/*
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
*/
int main()
{
    //создаем энергосистему
    pwrsys sys;

    Sdlstate State = {NULL, NULL};
    matrix m = {NULL, NULL};
    if (initialize_window(&State, 1280, 720)){
        return 1;
    }
    
    //window loop
    _Bool is_running = true;
    
    while (is_running){
        SDL_Event event={0};
        
        while(SDL_PollEvent(&event)){
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                is_running = false;
                break;
            
            default:
                break;
            }
            SDL_SetRenderDrawColor(State.renderer, 255, 255, 255, 255);
            SDL_RenderClear(State.renderer);

            SDL_RenderPresent(State.renderer);
        }
            
    }
    // считаем пример на экзамен
    double g_mat[NODES][NODES] = {
        {-0.23, 0.09, 0.09, 0.05},
        {0.09, -0.09, 0, 0},
        {0.09, 0, -0.23, 0.14},
        {0.05, 0, 0.14, -0.19}};
    double p_mat[NODES] = {-156.61, -272.92, 102.81, 136.04,-136.4, 102.54, -135.08, 25.05, -96.4, -19.76, -46.77, -50.08, 119.47, 192.13};
    double v_base = 114.32;

    puts("Калькулятор электрической сети постоянного тока");

    // Инициализация системы
    initialize_system(&sys, g_mat, p_mat, v_base, 3);

    // Выполнение расчета
    solver(&sys);

    // Вывод результатов
    printResults(&sys);

    cleanup(&State);
    return 0;
}