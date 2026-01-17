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

void cleanup(Sdlstate* State){
    SDL_DestroyRenderer(State->renderer);
    SDL_DestroyWindow(State->window);
    SDL_Quit();
}

_Bool initialize_window(Sdlstate* State, const int width, const int height){
//initialize gui
    if(!SDL_Init(SDL_INIT_VIDEO)){
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "No GUI was initialized!", NULL);
    }
    //create window
    State->window = SDL_CreateWindow("matrix_calc V 0.1", width, height, 0);
    if (!State->window){
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "No window was initialized!", NULL);
        cleanup(State);
        return 1;
    }
    //создаем рендерер
    State->renderer = SDL_CreateRenderer(State->window, NULL);
    if (!State->renderer){
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "No renderer was initialized!", NULL);
        cleanup(State);
        return 1;
    }
    return 0;
}

void render_text_field(SDL_Renderer* renderer, textfield* tf, SDL_Color bg, SDL_Color border) {
    // Фон
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(renderer, &tf->rect);
    
    // Рамка
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    SDL_RenderRect(renderer, &tf->rect);
    // Текст (здесь нужен SDL_ttf для рендеринга текста)
    // Для простоты можно выводить в консоль
}

void init_text_field(textfield* tf, float x, float y, float w, float h) {
    tf->text[0] = '\0';
    tf->selected = false;
    tf->rect.x = x;
    tf->rect.y = y;
    tf->rect.w = w;
    tf->rect.h = h;
}



void draw_matrix_size(Sdlstate* State, matrix_sdl m){
    if (!m.rows||!m.cols){
        init_text_field(&m.cols_input, 0, 0, 100, 100);
        init_text_field(&m.rows_input, 120, 0, 100, 100);
        render_text_field(State->renderer, &m.cols_input, SDL_bg, 0);
        render_text_field(State->renderer, &m.rows_input, 255, 00)
    }

    
}




