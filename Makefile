FLAGS = -Wall
MKDIR_P = mkdir -p

make: make all

make all: out_dir bin/balcao bin/ger_cl

out_dir:
	${MKDIR_P} bin
	
bin/balcao: balcao.c
	gcc $(FLAGS) balcao.c -o bin/balcao
	
bin/ger_cl: ger_cl.h ger_cl.c
	gcc $(FLAGS) ger_cl.c -o bin/ger_cl