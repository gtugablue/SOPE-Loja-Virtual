A nossa implementação divide-se nos 2 grupos de ficheiros exigidos no enunciado (balcao.c, balcao.h, ger_cl.c, ger_cl.c) e em mais 2 grupos.

log.c e log.h:
	Nestes ficheiros está implementada uma interface que facilita o registo dos vários eventos ocorridos nos balcões e clientes, escrevendo essas informações no
	ficheiro correspondente a essa memória partilhada (p.e. loja.log), formatando-a numa tabela de texto onde é fácil perceber como se procederam os eventos do programa.
	
utils.c e utils.h:
	Nestes ficheiros estão definidas algumas funções com utilidade comum a balcões, clientes e, possivelmente, outros programas. As principais funcionalidades implementadas
	são funções que permitem simplificar o lock, unlock e destroy de mutexes, permitindo ao programa que as chamas fazer ou não debug do que acontece quando se utilizam os mutexes.
	Para além disso, existem as funções parse_int e parse_long que tentam converter uma string para um inteiro (int ou long), retornando erro se não for possível (ou seja, 
	se a string não representar um inteiro). Por fim, existe a função filenameFromPath que permite converter uma string como /tmp/fc_1234, ou seja, uma string com o caminho
	completo para um ficheiro, numa string com apenas o nome do ficheiro correspondente (neste caso, fc_1234).
	
Os programas balcao e ger_cl (compilados com o comando make) permitem diversos tipos de utilização:
	
	ger_cl permite uma execução normal através da ordem de comandos especificada no enunciado (ger_cl <nome_mem_partilhada> <num_clientes>) mas, para além disso, 
	se entre os argumentos se escrever "-db", em qualquer ordem (desde que, obviamente, depois de ger_cl), o programa descreve mais detalhadamente o que se passa internamente
	na sua execução, fazendo uma espécie de debug de ele próprio. Esta funcionalidade permite perceber a ordem pela qual as ações ocorrem na execução do programa uma vez
	que as mensagens geradas indicam quem as executa.
	
	balcao permite também a execução especificada no enunciado (balcao <nome_memoria_partilhada> <tempo_de_abertura>) bem como a funcionalidade "-db" descrita no parágrafo anterior.
	Para além disso, se por algum motivo uma versão anterior de balcao terminar sem sucesso e se pretender "limpar" uma região de memória partilhada definida anteriormente, 
	basta chamar o programa com "balcao -c <nome_memoria_partilhada>" para limpar (fazer unlink) dessa memória partilhada.