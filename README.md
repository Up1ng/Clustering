# Clustering
для запуска кода нужно скачать библиотеку PLplot 

далее собрать программу через команду make

пример запуска екзешника

./a.exe kmeans data.csv 4

здесь мы строго задали количество кластеров 4, но если ничего не писать, то они определятся автоматически методом локтя

./a.exe hdbscan data.csv 0.04

0.04 это параметр плотности соседей, чем больше число тем более разряженный кластер получается. Если ничего не задать, то автоматически будет 0.05
