#include "render.h"
#include "calc.h"

int main()
{
    //создаем энергосистему
    pwrsys sys;

    //создаем и инициализируем матрицу
    matrix m = {NULL, NULL, 0};
    init_m(&m);
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREENWIDTH, SCREENHEIGHT, "matrix calc V 0.1");
    SetWindowMinSize(1271, 200);
    SetTargetFPS(60);
    
    _Bool mouse_button_pressed = false;
    _Bool init_graphics = false;
    _Bool init_cursor = false;
    _Bool should_close = false;
    int text_chosen = 0;
    puts("init window yes");

    while(!WindowShouldClose()){
        int num_tf = check_collision(&m);//-1, -2, -3, -4 для строк, столбцов, кнопки решить и очистить соответственно
        
        mouse_button_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT); //если был произведен клик
        
        if (num_tf != 0){ // если мышка наведена на поле, то рисуем красивый курсор
            if (num_tf != -3 && num_tf != -4) //если поле текстовое
                SetMouseCursor(MOUSE_CURSOR_IBEAM);
        }
        else{
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }

        if (mouse_button_pressed){ // если мы кликнули по полю, то делаем его выделенным
            if (num_tf != 0){
                text_chosen = num_tf;
            }
            else{
                text_chosen = 0;
            }
        }
        switch (text_chosen)
        {
        case -3:
            if (m.n_tf != 0)
                destroy_data(&m);
            make_m(&m);
            text_chosen = 0;
            break;
        case -4:
            if (m.n_tf){
                destroy_data(&m);
                text_chosen = 0;
            }
            break;
        case -5:
            if (m.n_tf){
                transfer_data(&m);
                CloseWindow();
                should_close = true;
            }
            break;
        default:
            if (text_chosen != 0){
                handle_input(&m, text_chosen);
            }
            break;
        }
        
        if (should_close)
            break;
        

        if(!init_cursor){
            puts("init cursor yes");
            init_cursor = true;
        }
        //Задаем параметры окошек
        int scr_w = GetScreenWidth();
        int scr_h = GetScreenHeight();
        make_tf(&m, scr_w, scr_h); //задаем размеры полей
        draw_graphics(&m, scr_w, scr_h, num_tf, text_chosen); // рисуем графику
        if(!init_graphics){
            puts("init graphics yes");
            init_graphics = true;
        }

    }

    puts("window closed");
    
    int Nodes = atoi(m.menu.rows.str);
    printf("%.1f stack \n", m.data[0][0]);
    /*
    for (size_t i = 0; i< Nodes; ++i){
        for (size_t j = 0; j < Nodes; ++j){
            printf("%.1f ", *(m.data[i]+j));
        }
        printf("\n");
    }
    */
    double p_mat[] = {-156.61, -272.92, 102.81, 136.04,-136.4, 102.54, -135.08, 25.05, -96.4, -19.76, -46.77, -50.08, 119.47, 192.13};
    double v_base = 114.32;

    puts("Калькулятор электрической сети постоянного тока");

    // Инициализация системы
    initialize_system(&sys, m.data, p_mat, v_base, 3, Nodes);

    // Выполнение расчета
    solver(&sys);

    // Вывод результатов
    printResults(&sys);
    if (m.n_tf)
        destroy_data(&m);
    free(m.menu.rows.str);
    free(m.menu.colls.str);
    CloseWindow();

    return 0;
}