#include <stdio.h>
#define point 10000 // макс. число точек
#define maxk 10     // макс. K для локтя
#define maxit 100   // макс. итераций
#define E 1e-4      // порог сходимости

// глобальные массивы точек заполняются в read()
extern double xcoord[point], ycoord[point];
extern int mark[point];
extern double **centrbest;

// чтение точек из data.csv
int read(const char *fname);

double kmeans(int n, int K, double **centroids);
//    n – число точек
//    K – число кластеров
//    centroids – будут записаны центроиды
//    Возвращает WCSS (сумма квадратичных ошибок)

int selectK(int n, int *outK);
// selectK — выбирает оптимальное число кластеров методом «локтя»
//  n  — число точек
//  outK  — указатель, куда записать найденное K
//  возвр наилучшее K