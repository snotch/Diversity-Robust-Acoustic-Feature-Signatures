fractalAnalysis : main.o calc_fractal_dimention.o
	gcc44 -O3 -std=c99 -lm -lsox -L/usr/local/lib -o fractalAnalysis main.o calc_fractal_dimention.o

calc_fractal_dimention.o : calc_fractal_dimention.c
	gcc44 -march=native -O3 -std=c99 -c calc_fractal_dimention.c

main.o : main.c
	gcc44 -march=native -O3 -std=c99 -c main.c
