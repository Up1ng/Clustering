a:
	gcc main.c kmeans.c hdbscan.c \
		-IC:/msys64/mingw64/include/plplot \
		-LC:/msys64/mingw64/lib -lplplot -lm \
		-o $@