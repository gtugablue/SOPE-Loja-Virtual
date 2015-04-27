FLAGS = -Wall
MKDIR_P = mkdir -p

make: make all

make all: out_dir bin/balcao bin/ger_cl utils

out_dir:
	${MKDIR_P} bin
	
utils:
	gcc $(FLAGS) utils.c -o bin/utils.o
	
bin/balcao: balcao.c utils.h utils.c
	gcc $(FLAGS) balcao.c -o bin/balcao
	
bin/ger_cl: ger_cl.h ger_cl.c utils.h utils.c
	gcc $(FLAGS) ger_cl.c -o bin/ger_cl