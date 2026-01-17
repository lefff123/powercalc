//header file for all solver shit
#define NODES 15
#define MAX_ITER 200
#define TOLERANCE 1e-6
#define NUM_Z 5

typedef struct
{
    double G[NODES][NODES]; // Матрица проводимостей
    double P[NODES];        // Активная мощность
    double V[NODES];        // Напряжения
    int node_types[NODES];  // 0 - PQ, 2 - база
} pwrsys;

void initialize_system(pwrsys *sys, double g_mat[NODES][NODES], double *p_mat, double v, int base); //initializing system with the nums

void printResults(pwrsys *sys); //printing results of solution

void solver(pwrsys *sys); //solver for powersystem