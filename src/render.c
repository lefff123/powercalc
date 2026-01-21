#include "raylib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define SCREENWIDTH 1280
#define SCREENHEIGHT 720



typedef struct{
    Rectangle r;
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
    textfield* tf;
    unsigned long long n_tf;
    up_menu menu;
}
matrix;

void init_m(matrix* m){
    m->menu.rows.str = malloc(sizeof(char)*32);
    m->menu.rows.str[0] = '\0';
    m->menu.rows.strlen=0;
    m->menu.colls.str = malloc(sizeof(char)* 32);
    m->menu.colls.str[0] = '\0';
    m->menu.colls.strlen=0;
    m->n_tf = 0;
}
void destroy_data(matrix* m){
    for (size_t i=0; i< m->n_tf; ++i){
        free(m->tf[i].str);
    }
    int rows = atoi(m->menu.rows.str);
    for (size_t i = 0; i < rows; ++i){
        free(m->data[i]);
    }
    free(m->data);
    puts("destroyed strings!");
    free(m->tf);
    m->n_tf = 0;
}


void transfer_data(matrix* m){
    int rows = atoi(m->menu.rows.str);
    int cols = atoi(m->menu.colls.str);
    int num = 0;
    for (size_t i = 0; i < rows; ++i){
        for (size_t j = 0; j < cols; ++j){
            double num_t = atof(m->tf[i*cols + j].str);
            m->data[i][j] = num_t;
            printf("%.1f\n", m->data[i][j]);
            ++num;
        }
    }
    printf("%d elements transfered\n", num);
}

void make_m(matrix* m){
    int rows = atoi(m->menu.rows.str);
    int cols = atoi(m->menu.colls.str);
    m->tf = malloc(sizeof(textfield)*rows*cols);

    for (size_t i = 0; i < rows; ++i){
        for (size_t j = 0; j < cols; ++j){
            m->tf[i*cols + j].r.height = m->menu.rows.r.height;
            m->tf[i*cols + j].r.width = m->menu.rows.r.width;
            m->tf[i*cols + j].r.x = (m->menu.rows.r.width)*j + 20;
            m->tf[i*cols + j].r.y = (m->menu.rows.r.height)*(i+1) + 40;
            m->tf[i*cols + j].str = malloc(sizeof(char)*32);
            m->tf[i*cols + j].str[0] = '\0';
            m->tf[i*cols + j].strlen = 0;
        }
    }
    m->n_tf = rows*cols;

    m->data = malloc(sizeof(double*)*rows);
    for (size_t i = 0; i < rows; ++i){
        m->data[i] = malloc(sizeof(double)*rows);
    }

}

int check_collision(matrix* m){
    Vector2 mouse_pos = GetMousePosition();
    for (long long i=0; i < m->n_tf; ++i){
        if(CheckCollisionPointRec(mouse_pos, m->tf[i].r))
            return i + 1;
    }
    if (CheckCollisionPointRec(mouse_pos, m->menu.rows.r))
        return -1;
    if (CheckCollisionPointRec(mouse_pos, m->menu.colls.r))
        return -2;
    if (CheckCollisionPointRec(mouse_pos, m->menu.btn_make.r))
        return -3;
    if (CheckCollisionPointRec(mouse_pos, m->menu.btn_clear.r))
        return -4;
    if (CheckCollisionPointRec(mouse_pos, m->menu.btn_solve.r))
        return -5;
    return 0;
}

_Bool if_eq(int num)
{ // функция для проверки вводимых символов UTF8
    int start_n = 48;
    int end_n = 57;
    int num_sym = 3;
    int sym[] = {45, 43, 46};// + -  . 

    if (num >= start_n && num <= end_n)
    {
        return true;
    }
    else
    {
        for (unsigned long long i = 0; i < num_sym; ++i)
        {
            if (sym[i] == num)
                return true;
        }
    }
    return false;
}



void draw_rect_shape(matrix* m, int chosen){
    switch (chosen)
    {
    case (-1):
        DrawRectangleLinesEx(m->menu.rows.r, 2, BLACK);
        break;
    case (-2):
        DrawRectangleLinesEx(m->menu.colls.r, 2, BLACK);
        break;
    case (-3):
        DrawRectangleLinesEx(m->menu.btn_make.r, 2, BLACK);
        break;
    case (-4):
        DrawRectangleLinesEx(m->menu.btn_clear.r, 2, BLACK);
        break;
    case (0):
        break;
    default:
        DrawRectangleLinesEx(m->tf[chosen-1].r, 2, BLACK);
        break;
    }
}
void draw_all_rect_shapes(matrix* m, int chosen) {
    // Сначала рисуем ВСЕ поля с обычной рамкой
    for (int i = 0; i < m->n_tf; i++) {
        DrawRectangleLinesEx(m->tf[i].r, 1.5, DARKGRAY);
    }
    
    // Рисуем меню с обычной рамкой
    DrawRectangleLinesEx(m->menu.rows.r, 1.5, DARKGRAY);
    DrawRectangleLinesEx(m->menu.colls.r, 1.5, DARKGRAY);
    DrawRectangleLinesEx(m->menu.btn_make.r, 1.5, DARKGRAY);
    DrawRectangleLinesEx(m->menu.btn_clear.r, 1.5, DARKGRAY);
    
    // Потом рисуем ПОВЕРХ толстую рамку для выбранного элемента
    draw_rect_shape(m, chosen);
}
void handle_input(matrix* m, int chosen)
{

    int key = GetCharPressed();
    if (!IsKeyPressed(KEY_BACKSPACE))
    { // если клавиша не backspace
        if (key != 0 && if_eq(key))
        {  // если мы нажали цифру или -  . +, то идем дальше
            switch (chosen)
            {
            case (-1): // ряды
                if (m->menu.rows.strlen < 31)
                {
                    m->menu.rows.str[m->menu.rows.strlen] = key;
                    ++m->menu.rows.strlen;
                    m->menu.rows.str[m->menu.rows.strlen] = '\0';
                }
                break;
            case (-2): // колонки
                if (m->menu.colls.strlen < 31)
                {
                    m->menu.colls.str[m->menu.colls.strlen] = key;
                    ++m->menu.colls.strlen;
                    m->menu.colls.str[m->menu.colls.strlen] = '\0';
                }
                break;
            case (-3):
                break;
            case (-4):
                break;
            case (-5):
                break;    
            case (0):
                break;

            default: // любое другое текстовое поле
                if (m->tf[chosen - 1].strlen < 31)
                {
                    m->tf[chosen - 1].str[m->tf[chosen - 1].strlen] = key;
                    ++m->tf[chosen - 1].strlen;
                    m->tf[chosen - 1].str[m->tf[chosen - 1].strlen] = '\0';
                }
                break;
            }
        }
    }
    else
    {
        switch (chosen)
        {
        case (-1): // ряды
            if (m->menu.rows.strlen > 0)
            {
                --(m->menu.rows.strlen);
                m->menu.rows.str[m->menu.rows.strlen] = '\0';
            }
            break;
        case (-2): // колонки
            if (m->menu.colls.strlen > 0)
            {
                --(m->menu.colls.strlen);
                m->menu.colls.str[m->menu.colls.strlen] = '\0';
            }
            break;
        case (0):
            break;
        case (-3):
            break;
        case (-4):
            break;
        case (-5):
            break;
        default: // любое другое текстовое поле
            if (m->tf[chosen - 1].strlen > 0)
            {
                --(m->tf[chosen - 1].strlen);
                m->tf[chosen - 1].str[m->tf[chosen - 1].strlen] = '\0';
            }
            break;
        }
    }

    return;
}

void make_tf(matrix* m, int scr_w, int scr_h){
    m->menu.btn_solve.r.x = 20;
    m->menu.btn_solve.r.y =  20;  
    m->menu.btn_solve.r.height = 50;
    m->menu.btn_solve.r.width = scr_w/8;

    m->menu.rows.r.x = (scr_w)/2. - scr_w/8 - 100;
    m->menu.rows.r.y =  20;  
    m->menu.rows.r.height = 50;
    m->menu.rows.r.width = scr_w/8;

    m->menu.colls.r.x = (scr_w)/2. + 100;
    m->menu.colls.r.y = 20;
    m->menu.colls.r.height = 50;
    m->menu.colls.r.width = scr_w/8;

    m->menu.btn_make.r.x = scr_w/4. * 3;
    m->menu.btn_make.r.y = 20;
    m->menu.btn_make.r.height = 50;
    m->menu.btn_make.r.width = scr_w/16; 

    m->menu.btn_clear.r.x = scr_w/8. * 7 - 20;
    m->menu.btn_clear.r.y = 20;
    m->menu.btn_clear.r.height = 50;
    m->menu.btn_clear.r.width = scr_w/16; 

}

void draw_graphics(matrix* m, int scr_w, int scr_h, int chosen, _Bool mouse_button_pressed)
{
    BeginDrawing();
    ClearBackground(RAYWHITE);
    //рисуем кнопки верхнего меню
    DrawRectangleRec(m->menu.rows.r, LIGHTGRAY); 
    DrawRectangleRec(m->menu.colls.r, LIGHTGRAY);
    DrawRectangleRec(m->menu.btn_make.r, LIGHTGRAY);
    DrawRectangleRec(m->menu.btn_clear.r, LIGHTGRAY);
    DrawRectangleRec( m->menu.btn_solve.r, LIGHTGRAY);

    DrawText("N rows", m->menu.rows.r.x, m->menu.rows.r.y, 12, BLACK);
    DrawText(m->menu.rows.str, m->menu.rows.r.x, m->menu.rows.r.y + m->menu.rows.r.height/2, 18, BLACK);

    DrawText("N cols",m->menu.colls.r.x, m->menu.colls.r.y, 12, BLACK);
    DrawText(m->menu.colls.str, m->menu.colls.r.x, m->menu.colls.r.y + m->menu.colls.r.height/2, 18, BLACK);

    DrawText("Make matrix", m->menu.btn_make.r.x, m->menu.btn_make.r.y + m->menu.btn_make.r.height/2, 13, BLACK);
    DrawText("Tap to clear", m->menu.btn_clear.r.x, m->menu.btn_clear.r.y + m->menu.btn_clear.r.height/2, 13, BLACK);
    DrawText("Tap to solve", m->menu.btn_solve.r.x, m->menu.btn_solve.r.y + m->menu.btn_solve.r.height/2, 13, BLACK);

    for (size_t i = 0; i< m->n_tf; ++i){
        DrawRectangleRec(m->tf[i].r, LIGHTGRAY);
        DrawText(m->tf[i].str, m->tf[i].r.x, m->tf[i].r.y + m->tf[i].r.height/2., 18, BLACK);
    }
    
    draw_all_rect_shapes(m, chosen);

    EndDrawing();
}

