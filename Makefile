FLAGS = -Wall
MKDIR_P = mkdir -p

make: make all

make all: out_dir balcao ger_cl

out_dir:
	${MKDIR_P} bin
	
balcao:
	gcc $(FLAGS) balcao.c utils.c -o bin/balcao
	
ger_cl:
	gcc $(FLAGS) ger_cl.c utils.c -o bin/ger_cl