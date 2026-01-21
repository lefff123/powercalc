#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


#define NODES 15
#define MAX_ITER 20
#define TOLERANCE 1e-6
#define NUM_Z 5

// Структура для хранения параметров сети постоянного тока
typedef struct
{
    double** G; // Матрица проводимостей
    double* P;        // Активная мощность
    double* V;        // Напряжения
    int* node_types;  // 0 - PQ, 2 - база
} pwrsys;
 
void initialize_system(pwrsys *sys, double** g_mat, double *p_mat, double v, int base, int Nodes)
{ // система, проводимость, мощности, напряжение базы, номер базы
    // задаем проводимости
    sys->G = g_mat;

    // задаем мощности и напряжения
    sys->P = malloc(sizeof(double)*Nodes);
    sys->V = malloc(sizeof(double)*Nodes);
    sys->node_types = malloc(sizeof(int)*Nodes);

    puts("bytes managed");
    for (int i = 0; i < Nodes; i++)
    {
        sys->P[i] = p_mat[i];
        sys->V[i] = v;
        sys->node_types[i] = 0;
    }

    puts("nodes initialized");
    if (base< Nodes){
        sys->node_types[base] = 2; // база
    }
    else{
        sys->node_types[Nodes-1] = 2; //проверка, не вылезли ли мы за границы
    }

    puts("Initialized:");
    for (int i = 0; i < Nodes; ++i)
    {
        printf("G%d:", i + 1);
        for (int j = 0; j < Nodes; ++j)
        {
            printf("%.2f ", sys->G[i][j]);
        }
        printf("\n");
        printf("P = %.2f, V = %.2f", sys->P[i], sys->V[i]);
        printf("\n");
    }
    puts("");
}

void printResults(pwrsys *sys)
{
    printf("\n======== РЕЗУЛЬТАТЫ РАСЧЕТА ==========\n");
    printf("Узел |  Тип  | Напряжение(pu) | P(pu)\n");
    printf("-----|-------|----------------|-------\n");

    for (int i = 0; i < NODES; i++)
    {
        const char *type_str = (sys->node_types[i] == 2) ? " база " : "  PQ  ";

        printf("%3d  | %6s | %14.4f | %5.2f\n",
               i + 1, type_str, sys->V[i], sys->P[i]);
    }
}


double **form_j(pwrsys *sys)
{
    double **jacob = malloc(sizeof(double *) * NODES);
    for (size_t i = 0; i < NODES; ++i)
    {
        jacob[i] = malloc(sizeof(double) * NODES);
    }

    for (size_t i = 0; i < NODES; ++i)
    { // заполняем якобиан проводимостями
        for (size_t j = 0; j < NODES; ++j)
        {
            if (i == j)
            {
                jacob[i][j] = sys->G[i][j] - (sys->P[i] / (sys->V[i] * sys->V[i])); // example: G11 - P1/U1^2
            }
            else
            {
                jacob[i][j] = sys->G[i][j];
            }
        }
    }
    return jacob;
}



void calc_z(pwrsys *sys)
{
    // метод Зейделя
    //  вычисляем новые значения напряжений
    for (size_t eq = 0; eq < NODES; ++eq)
    {
        /*
        считаем уравнение вида
        U_k = (сумма((Gij*Ui)) + (Pi/U_k))G_k,где i!=k
        рассчитываем его так, сперва сумма проводимостей, умноженных на напряжения, потом добавляем мощность
        */
        if (sys->node_types[eq] == 2)
        { // если узел - база, то не выполняем для него вычисления
            continue;
        }
        long double result = 0.;

        for (size_t piece = 0; piece < NODES; ++piece)
        { // сумма((Gij*Ui)
            if (piece == eq)
            { // если мы пытаемся посчитать член уравнения с тем же номером, то мы продолжаем
                continue;
            }
            result += sys->G[eq][piece] * sys->V[piece];
        }
        result -= sys->P[eq] / sys->V[eq]; // + Pi/U_k

        sys->V[eq] = (result / -sys->G[eq][eq]); // делим на G_k
    }
}

void solver(pwrsys *sys)
{

    long double tolerance = 1;

   

    double **j = form_j(sys);                   // якобиан
    double *r = malloc(sizeof(double) * NODES); // невязки
    char is_n = 0;                              // флаг, считали ли мы невязки

    double *d_u = malloc(sizeof(double) * NODES); // изменения напряжения
    for (size_t i = 0; i < NODES; ++i)
    { // инициализируем массив
        d_u[i] = 0.;
    }

    for (size_t i = 0; i < MAX_ITER && tolerance > TOLERANCE; ++i)
    {
        double v_prev[NODES];

        for (size_t k = 0; k < NODES; ++k)
        {
            v_prev[k] = sys->V[k] + d_u[k];
        }

        if (i < NUM_Z)
        {
            calc_z(sys); // считаем по методу Зейделя
        }
        else
        {

            // считаем методом Ньютона
            // считаем невязки
            if (!is_n)
            {
                puts("Начало Метода Ньютона");
                for (size_t f = 0; f < NODES; ++f)
                { // формируем вектор невязок
                    double res = 0;
                    for (size_t s = 0; s < NODES; ++s)
                    {
                        res += sys->G[f][s] * sys->V[s]; // считаем Gij*Ui
                    }
                    res -= sys->P[f] / sys->V[f]; // считаем Pi/Ui
                    r[f] = res;
                }

                // считаем первый прогон метода Ньютона

                for (size_t k = 0; k < NODES; ++k)
                {
                    if (sys->node_types[k]!=2){
                        double res = 0.;
                        for (size_t j = 0; j < NODES; ++j)
                        {
                            if (j != k && sys->node_types[j] != 2) //если узел j!=k и не база, то записываем его как член уравнения
                            {
                                res += sys->G[k][j] * d_u[j];
                            }
                        }
                        res -= r[k];    // невязка
                        res /= j[k][k]; // деление на катый катый элемент якобиана
                        d_u[k] = res;   // присваиваем изменение
                    }
                }
                printf("Начальный расчет du: ");
                for (size_t k=0; k< NODES; ++k){
                    printf("%.2f, ", d_u[k]);
                }
                printf("\n");

                is_n = 1;
            }

            // считаем приближения
            /*
            уравнение вида:
            dU_k = (сумма(Gkj*dUj) - невязка[k])/J[k][k]
            */

            for (size_t k = 0; k < NODES; ++k)
            {
                if (sys->node_types[k]!= 2){
                    double res = 0.;
                    for (size_t j = 0; j < NODES; ++j)
                    {
                        if (j != k && sys->node_types[j]!=2)
                        {
                            res += sys->G[k][j] * d_u[j];
                        }
                    }
                    res -= r[k];    // невязка
                    res /= j[k][k]; // деление на катый катый элемент якобиана
                    d_u[k] = res;   // присваиваем изменение
            }
            }
            //кусок кода для вывода вектора d_u
            /*printf("%d расчет du: ", i - NUM_Z+2);
                for (size_t k=0; k< NODES; ++k){
                    printf("%.2f, ", d_u[k]);
                }
                printf("\n");*/
        }
        // считаем точность
        double av = 0;
        for (size_t k = 0; k < NODES; ++k)
        {
            if (sys->node_types[k] != 2)
            {
                if (sys->V[k]+d_u[k] > v_prev[k]) //не нашел нормальной функции модуля для double, сделал через костыль,
                {//чтобы число всегда было положительным
                    av += sys->V[k] + d_u[k] - v_prev[k];
                }
                else
                {
                    av += v_prev[k] - sys->V[k] - d_u[k];
                }
            }
        }
        if (av / (double)(NODES-1)  < TOLERANCE)
        {

            for (size_t k = 0; k < NODES; ++k)
            {
                if (sys->node_types[k]!=2)
                {
                    sys->V[k] += d_u[k];
                }
            }
            return;
        }
        printf("Итерация %ld, значения напряжений: ", i + 1);
        for (size_t k = 0; k < NODES; ++k)
        {
            printf("U%ld = %.9f, ", k + 1, sys->V[k]+ d_u[k]);
        }
        printf("\n");
    }
    free(j);
    free(r);
}