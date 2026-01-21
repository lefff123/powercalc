//header file for all solver shit
#define NODES 15
#define MAX_ITER 20
#define TOLERANCE 1e-6
#define NUM_Z 5

typedef struct
{
    double** G; // Матрица проводимостей
    double* P;        // Активная мощность
    double* V;        // Напряжения
    int node_types[];  // 0 - PQ, 2 - база
} pwrsys;

void initialize_system(pwrsys *sys, double g_mat[NODES][NODES], double *p_mat, double v, int base, int Nodes); //initializing system with the nums

void printResults(pwrsys *sys); //printing results of solution

void solver(pwrsys *sys); //solver for powersystem