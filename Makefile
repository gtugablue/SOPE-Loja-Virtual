FLAGS = -Wall
MKDIR_P = mkdir -p

make: make all

make all: out_dir bin/balcao

out_dir:
	${MKDIR_P} bin
	
bin/balcao: balcao.c
	gcc $(FLAGS) balcao.c -o bin/balcao