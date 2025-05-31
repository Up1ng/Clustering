#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <plplot/plplot.h>
#include "kmeans.h"
#include "hdbscan.h"

int main(int argc, char *argv[])
{
    const char *slovo = argv[1];
    const char *fname = argv[2];

    // настройка PLplot
    plsdev("pngcairo");
    // размер для графика 8×6 дюймов
    plsetopt("PAPER", "8x6");

    // если выбран kmeans
    if (strcmp(slovo, "kmeans") == 0)
    {
        // читаем данные из файла и возвращается число точек n
        int n = read(fname);
        int K;
        double **centroids;

        // если пользователь передал параметр K в argv[3]
        if (argc >= 4)
        {
            K = atoi(argv[3]);
            centroids = malloc(K * sizeof(double *));
            for (int k = 0; k < K; k++)
                centroids[k] = malloc(2 * sizeof(double));
            kmeans(n, K, centroids);
        }
        else
        {
            // если K не задан, выбираем автоматически selectK
            K = selectK(n, &K);
            centroids = centrbest;
        }

        // смотрим границы для нашего графика
        double xmin = xcoord[0], xmax = xcoord[0];
        double ymin = ycoord[0], ymax = ycoord[0];
        for (int i = 1; i < n; i++)
        {
            if (xcoord[i] < xmin)
                xmin = xcoord[i];
            if (xcoord[i] > xmax)
                xmax = xcoord[i];
            if (ycoord[i] < ymin)
                ymin = ycoord[i];
            if (ycoord[i] > ymax)
                ymax = ycoord[i];
        }
        // даём имя
        plsfnam("kmeans.png");
        plinit();
        plenv(xmin, xmax, ymin, ymax, 0, 0);
        // заголовок + оси x и y
        pllab("X", "Y", "k-means Clusters");

        // рисуем точки и расскрашиваем их
        for (int i = 0; i < n; i++)
        {
            double xi = xcoord[i], yi = ycoord[i];
            int lab = mark[i]; // метка кластера для этой точки

            int col; // переменная для цвета
            if (lab < 0)
            {
                col = 1; // шум — цвет 1
            }
            else
            {
                col = (lab % 15) + 2; // для остальных
            }

            plcol0(col);            // устанавливаем текущий цвет
            plpoin(1, &xi, &yi, 2); // рисуем точку
        }

        // отрисовываем центроид чуть по другому
        for (int k = 0; k < K; k++)
        {
            double cx = centroids[k][0], cy = centroids[k][1];
            int col = (k % 15) + 2;
            plcol0(col);
            plpoin(1, &cx, &cy, 14); // размер центройда больше
        }
        plend(); // конец
        if (centrbest != centroids)
        {
            for (int k = 0; k < K; k++)
            {
                free(centroids[k]);
            }
            free(centroids);
        }
        else
        {
            for (int k = 0; k < K; k++)
            {
                free(centrbest[k]);
            }
            free(centrbest);
        }
    }
    // для алгоритма hdbscan
    else if (strcmp(slovo, "hdbscan") == 0)
    {
        // читаем дата сет
        double *data;
        int n_samples, n_features;
        loadcsv(fname, &data, &n_samples, &n_features);

        // cчитываем frac или ставим 0.05 по умолчанию
        double frac;
        if (argc >= 4)
        {
            frac = atof(argv[3]);
        }
        else
        {
            frac = 0.05;
        }
        // запуск алгоритма
        Cluster *clusters;
        int n_clusters;
        int *labels;
        hdbscan(data, n_samples, n_features, frac, &clusters, &n_clusters, &labels);

        // тоже самое что и с kmeans
        double xmin = data[0], xmax = data[0];
        double ymin = data[1], ymax = data[1];
        for (int i = 1; i < n_samples; i++)
        {
            double xi = data[i * n_features + 0];
            double yi = data[i * n_features + 1];
            if (xi < xmin)
                xmin = xi;
            if (xi > xmax)
                xmax = xi;
            if (yi < ymin)
                ymin = yi;
            if (yi > ymax)
                ymax = yi;
        }
        plsfnam("hdbscan.png");
        plinit();
        plenv(xmin, xmax, ymin, ymax, 0, 0);
        pllab("X", "Y", "HDBSCAN Clusters");
        for (int i = 0; i < n_samples; i++)
        {
            double xi = data[i * n_features + 0];
            double yi = data[i * n_features + 1];
            int lab = labels[i];

            int col;
            if (lab < 0)
            {
                col = 1;
            }
            else
            {
                col = (lab % 15) + 2;
            }

            plcol0(col);
            plpoin(1, &xi, &yi, 2);
        }

        plend();
        free(data);
        free(clusters);
        free(labels);
    }
    return 0;
}
