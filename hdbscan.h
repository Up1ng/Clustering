
// стандартный граф с ребрами
typedef struct
{
    int u, v;
    double w;
} Edge;

// класстеры
typedef struct
{
    double birth; // класстер рождается
    double death; // класстер умирает
    int size;     // число точек сод в кластере
} Cluster;

int hdbscan(const double *dataset, int point_count, int dim, double frac, Cluster **clusters_out, int *num_clusters_out, int **labels_out);
// point_count число точек в выбоке
// dim размерность каждой точки
// frac для расчета
// clusters_out резултатный кластер
// num_clusters_out число записанных класстеров
// labels_out номера кластеров
//  загрузка дата сета
int loadcsv(const char *filename, double **dataset, int *prochstrok, int *prochstolb);
// filename путь к файлу
// dataset массив для хранения
// prochstrok сколько строк прочитали
// prochstolb сколько прочитали столбцов
