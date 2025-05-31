#include "hdbscan.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

// для удаление пробелов и символов перевода строки
void del(char *str)
{
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1]))
    {
        str[--len] = '\0';
    }
}

// загрузка data.csv
//  dataset массив чисел
//  prochstrok число прочитанных строк
//  prochstolb число прочитанных столбцов
int loadcsv(const char *filename, double **dataset, int *prochstrok, int *prochstolb)
{
    FILE *file = fopen(filename, "r");
    char buffer[8192];
    int stroka = 0, stolb = 0;

    // определяем размеры строк и столбцов
    while (fgets(buffer, sizeof(buffer), file))
    {
        del(buffer);
        if (stroka == 0)
        {
            for (char *p = buffer; *p; ++p)
            {
                if (*p == ',')
                    stolb++;
            }
            stolb++; // столбцы = запятые + 1
        }
        stroka++;
    }
    *dataset = malloc((size_t)stroka * stolb * sizeof(double));
    rewind(file);
    int row = 0;
    // читаем и парсим числа
    while (fgets(buffer, sizeof(buffer), file))
    {
        del(buffer);
        if (buffer[0] == '\0')
            continue;

        char *token = strtok(buffer, ",");
        for (int col = 0; col < stolb; ++col)
        {
            (*dataset)[row * stolb + col] = atof(token);
            token = strtok(NULL, ",");
        }
        row++;
    }

    fclose(file);
    *prochstrok = stroka;
    *prochstolb = stolb;
    return 0;
}

// для Union All
typedef struct
{
    int *parent;   // родитель в дереве
    int *rank;     // ранг/высота дерева
    int *numnodes; // размер компонента (количество узлов)
} UnionFind;

// Создать пустую структуру Union-Find на n элементов
UnionFind *uf_create(int n)
{
    UnionFind *uf = malloc(sizeof *uf);
    uf->parent = malloc(n * sizeof(int));
    uf->rank = calloc(n, sizeof(int));
    uf->numnodes = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
    {
        uf->parent[i] = i;
        uf->numnodes[i] = 1;
    }
    return uf;
}

// найти корень элемента x с путём сжатия
int uf_find(UnionFind *uf, int x)
{
    if (uf->parent[x] != x)
    {
        uf->parent[x] = uf_find(uf, uf->parent[x]);
    }
    return uf->parent[x];
}

// объединить компоненты a и b
void uf_merge(UnionFind *uf, int a, int b)
{
    a = uf_find(uf, a);
    b = uf_find(uf, b);
    if (a == b)
        return;

    // объединяем по рангу
    if (uf->rank[a] < uf->rank[b])
    {
        uf->parent[a] = b;
        uf->numnodes[b] += uf->numnodes[a];
    }
    else if (uf->rank[b] < uf->rank[a])
    {
        uf->parent[b] = a;
        uf->numnodes[a] += uf->numnodes[b];
    }
    else
    {
        uf->parent[b] = a;
        uf->rank[a]++;
        uf->numnodes[a] += uf->numnodes[b];
    }
}

// удалить всё с чем поработали
void uf_destroy(UnionFind *uf)
{
    free(uf->parent);
    free(uf->rank);
    free(uf->numnodes);
    free(uf);
}

// вычисление расстояние между точками x и y размерности dim и core distance
double distance(const double *x, const double *y, int dim)
{
    double sum = 0.0;
    for (int i = 0; i < dim; i++)
    {
        double diff = x[i] - y[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

// cравнение для qsort
int cmpdistance(const void *a, const void *b)
{
    const double da = *(const double *)a;
    const double db = *(const double *)b;

    if (da < db)
    {
        return -1; // первое значение меньше второго
    }
    else if (da > db)
    {
        return 1; // первое значение больше второго
    }
    else
    {
        return 0; // значения равны
    }
}

// вычисляем core distance для каждой точки:
// coredistance1 = расстояние до k ближайшего соседа
void coredistance(const double *dataset, int point_count, int dim, int k, double *coredistance1)
{
    double *dist_buffer = malloc(point_count * sizeof(double));

    for (int i = 0; i < point_count; i++)
    {
        const double *point_i = dataset + i * dim;
        // считаем расстояния от точки i до всех остальных
        for (int j = 0; j < point_count; j++)
        {
            dist_buffer[j] = distance(
                point_i, dataset + j * dim, dim);
        }
        // сортируем и берём k
        qsort(dist_buffer, point_count, sizeof(double), cmpdistance);
        coredistance1[i] = dist_buffer[k];
    }

    free(dist_buffer);
}
// сравниваем ребра
int cmp_edge(const void *a, const void *b)
{
    const Edge *ea = (const Edge *)a;
    const Edge *eb = (const Edge *)b;
    double wa = ea->w;
    double wb = eb->w;

    if (wa < wb)
    {
        return -1; // вес первого ребра меньше веса второго
    }
    else if (wa > wb)
    {
        return 1; // вес первого ребра больше веса второго
    }
    else
    {
        return 0; // веса ребер равны
    }
}

// вот и сам алгоритм HDBSCAN
int hdbscan(const double *dataset, int point_count, int dim, double frac, Cluster **clusters_out, int *num_clusters_out, int **labels_out)
{
    // минимальное число соседей k = round(point_count * frac), не менее 2
    int min_pts = (int)(point_count * frac + 0.5);
    if (min_pts < 2)
    {
        min_pts = 2;
    }

    // вычисляем core distance
    double *coredistance1 = malloc(point_count * sizeof(double));
    coredistance(dataset, point_count, dim, min_pts, coredistance1);

    // строим полный граф взаимной достижимости
    int edge_count = point_count * (point_count - 1) / 2;
    Edge *edge_list = malloc(edge_count * sizeof(Edge));
    int edge_index = 0;
    for (int i = 0; i < point_count; i++)
    {
        for (int j = i + 1; j < point_count; j++)
        {
            double d = distance(dataset + i * dim, dataset + j * dim, dim);
            double w = fmax(d, fmax(coredistance1[i], coredistance1[j]));
            edge_list[edge_index++] = (Edge){i, j, w};
        }
    }
    // сортируем рёбра по весу
    qsort(edge_list, edge_count, sizeof(Edge), cmp_edge);

    // строим минимальное отставное дерево методом Краскала
    UnionFind *uf = uf_create(point_count);
    Edge *mst = malloc((point_count - 1) * sizeof(Edge));
    int mst_edges = 0;
    for (int i = 0; i < edge_count && mst_edges < point_count - 1; i++)
    {
        int a = edge_list[i].u;
        int b = edge_list[i].v;
        if (uf_find(uf, a) != uf_find(uf, b))
        {
            uf_merge(uf, a, b);
            mst[mst_edges++] = edge_list[i];
        }
    }

    // находим самую большую разницу по весу в мин ост дер
    double max_gap = -DBL_MAX;
    int gap_position = -1;
    for (int i = 0; i < mst_edges - 1; i++)
    {
        double gap = mst[i + 1].w - mst[i].w;
        if (gap > max_gap)
        {
            max_gap = gap;
            gap_position = i;
        }
    }
    double porog;
    if (gap_position >= 0)
    {
        porog = mst[gap_position + 1].w;
    }
    else
    {
        porog = mst[mst_edges - 1].w;
    }

    // отрезаем рёбра с весом >= порог, восстанавливаем UF
    uf_destroy(uf);
    uf = uf_create(point_count);
    for (int i = 0; i < mst_edges; i++)
    {
        if (mst[i].w < porog)
        {
            uf_merge(uf, mst[i].u, mst[i].v);
        }
        else
        {
            break;
        }
    }

    // определяем уровень рождения каждого корня
    double *birth = calloc(point_count, sizeof(double));
    for (int i = 0; i < mst_edges; i++)
    {
        if (mst[i].w < porog)
        {
            int root = uf_find(uf, mst[i].u);
            if (mst[i].w > birth[root])
            {
                birth[root] = mst[i].w;
            }
        }
        else
        {
            break;
        }
    }

    // собираем устойчивые кластеры
    int *clusterind = malloc(point_count * sizeof(int));
    for (int i = 0; i < point_count; i++)
    {
        clusterind[i] = -1;
    }
    Cluster *cluster_list = malloc(point_count * sizeof(Cluster));
    int cluster_count = 0;
    for (int i = 0; i < point_count; i++)
    {
        int root = uf_find(uf, i);
        if (clusterind[root] == -1)
        {
            double stability = porog - birth[root];
            if (stability > 0)
            {
                clusterind[root] = cluster_count;
                cluster_list[cluster_count].birth = birth[root];
                cluster_list[cluster_count].death = porog;
                cluster_list[cluster_count].size = uf->numnodes[root];
                cluster_count++;
            }
            else
            {
                clusterind[root] = -2; // нестабильно
            }
        }
    }

    // присваиваем метки точкам (-1 это шум, отмечается черным цветом и на поле такие точки не видно)
    int *point_labels = malloc(point_count * sizeof(int));
    for (int i = 0; i < point_count; i++)
    {
        int idx = clusterind[uf_find(uf, i)];
        if (idx >= 0)
        {
            point_labels[i] = idx;
        }
        else
        {
            point_labels[i] = -1;
        }
    }

    // память чистим
    free(coredistance1);
    free(edge_list);
    free(mst);
    uf_destroy(uf);
    free(birth);
    free(clusterind);

    // РЕЗУЛЬТ
    *clusters_out = cluster_list;
    *num_clusters_out = cluster_count;
    *labels_out = point_labels;
    return 0;
}
