#include "kmeans.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

double xcoord[point], ycoord[point];
int mark[point];
double **centrbest = NULL;
// чтение точек из data.csv
int read(const char *fname)
{
    FILE *f = fopen(fname, "r");
    char line[256];
    int n = 0;
    while (n < point && fgets(line, sizeof line, f))
    {
        char *p = strtok(line, ",\r\n");
        if (!p)
            continue;
        xcoord[n] = atof(p);
        p = strtok(NULL, ",\r\n");
        if (!p)
            continue;
        ycoord[n++] = atof(p);
    }
    fclose(f);
    return n;
}

// сам алгоритм kmeans
double kmeans(int n, int K, double **centroids)
{
    // инициализация центроид
    for (int k = 0; k < K; k++)
    {
        int idx = rand() % n;
        centroids[k][0] = xcoord[idx];
        centroids[k][1] = ycoord[idx];
    }

    double prev_wcss = 1e300;
    for (int iter = 0; iter < maxit; iter++)
    {
        // присваиваем каждой точке ближайший центр
        for (int i = 0; i < n; i++)
        {
            double bestd = 1e300;
            int bestk = 0;
            for (int k = 0; k < K; k++)
            {
                double dx = xcoord[i] - centroids[k][0];
                double dy = ycoord[i] - centroids[k][1];
                double d2 = dx * dx + dy * dy;
                if (d2 < bestd)
                {
                    bestd = d2;
                    bestk = k;
                }
            }
            mark[i] = bestk;
        }
        double *sumx = calloc(K, sizeof(double));
        double *sumy = calloc(K, sizeof(double));
        int *cnt = calloc(K, sizeof(int));

        for (int i = 0; i < n; i++)
        {
            int k = mark[i];
            sumx[k] += xcoord[i];
            sumy[k] += ycoord[i];
            cnt[k]++;
        }
        for (int k = 0; k < K; k++)
        {
            if (cnt[k] > 0)
            {
                centroids[k][0] = sumx[k] / cnt[k];
                centroids[k][1] = sumy[k] / cnt[k];
            }
        }
        free(sumx);
        free(sumy);
        free(cnt);

        // вычислить WCSS
        double wcss = 0.0;
        for (int i = 0; i < n; i++)
        {
            int k = mark[i];
            double dx = xcoord[i] - centroids[k][0];
            double dy = ycoord[i] - centroids[k][1];
            wcss += dx * dx + dy * dy;
        }

        // проверка на сходимость
        if ((prev_wcss - wcss) < E)
            break;
        prev_wcss = wcss;
    }

    return prev_wcss;
}
// метод локтя если пользователь не задал строгое количесвто кластеров
int selectK(int n, int *outK)
{
    double *wcss_vals = malloc(maxk * sizeof(double));
    for (int K = 1; K <= maxk; K++)
    {
        double **c = malloc(K * sizeof(double *));
        for (int k = 0; k < K; k++)
            c[k] = malloc(2 * sizeof(double));
        wcss_vals[K - 1] = kmeans(n, K, c);
        for (int k = 0; k < K; k++)
            free(c[k]);
        free(c);
    }
    // находим место излома
    int bestK = 1;
    double bestDiff = 0;
    for (int i = 1; i < maxk - 1; i++)
    {
        double diff = wcss_vals[i - 1] - wcss_vals[i];
        if (diff > bestDiff)
        {
            bestDiff = diff;
            bestK = i + 1;
        }
    }
    free(wcss_vals);

    // выбираем лучший центроид
    *outK = bestK;
    centrbest = malloc(bestK * sizeof(double *));
    for (int k = 0; k < bestK; k++)
        centrbest[k] = malloc(2 * sizeof(double));
    // и считаем финальный раз
    kmeans(n, bestK, centrbest);
    return bestK;
}
