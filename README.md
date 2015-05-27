Organização da memória partilhada:
==================================

* mutex global
* tempo de abertura
* número de balcões
* número de balcões abertos (por motivos de eficiência)
* array de balcões (máximo 100):
	* mutex do balcão
	* ID
	* tempo de abertura
	* duração
	* nome da FIFO (máximo 20 caracteres)
	* número de clientes em atendimento
	* número de clientes atendidos
	* tempo de atendimento médio

Estratégias utilizadas
======================

Balcão
------

Depois de criada e inicializada a memória partilhada e a FIFO do balcão, é criada uma thread que faz o seguinte:

1. Abre a FIFO do balcão para escrita (para forçar as chamadas read() nessa FIFO a bloquearem enquanto o tempo de atendimento não terminar).
2. "Adormece" pelo tempo de abertura do balcão.
3. Fecha a FIFO do balcão.

Sempre que um cliente é atendido o balcão atualiza as suas informações na memória partilhada bloqueando apenas o seu mutex, o que permite que vários balcões acedam à memória partilhada ao mesmo tempo.
No entanto, para evitar problemas relacionados com *racing conditions*, ao terminar o balcão este bloqueia o mutex global. Isto evita que um cliente escolha um balcão no instante anterior a ele fechar.
Para fazer o atendimento dos clientes, sempre que um novo é recebido é criada uma thread de atendimento cujas ações são:

1. Abre a FIFO do cliente para escrita de forma a "comunicar" ao cliente que o seu atendimento vai ser iniciado.
2. Executa um sleep correspondente ao tempo de atendimento calculado (soma de 1 com o número de clientes em atendimento no instante anterior a se iniciar o atendimento do cliente, limitado a um máximo de 10 segundos).
3. Escreve a string "fim_atendimento" na FIFO do cliente, fechando-a de seguida.
4. Atualiza as estatísticas do balcão e termina libertando a memória alocada dinâmicamente, referente apenas a essa thread.

Cliente
-------

Um cliente, ao entrar na loja, bloqueia o mutex global e procura o balcão com menor número de clientes em atendimento, bloqueando o seu mutex de seguida. É neste momento que o cliente abre a FIFO do balcão e envia o nome da sua FIFO privada para a FIFO do balcão. Depois de fazer isto, abre a sua própria FIFO, sendo esta uma abertura bloqueante que só é terminada quando o balcão abre a FIFO do cliente para escrita. Quando esta abertura é concluída, o cliente sabe que o balcão já iniciou seu atendimento, libertando nesse momento o mutex global. Isto impossibilita um cálculo errado dos tempos de atendimento de cada cliente, uma vez que força a receção sequencial e não simultânea dos clientes. De seguida, o cliente entra numa leitura bloqueante da sua própria FIFO até receber a string "fim_atendimento". Por fim, limpa a memória que ocupou dinâmicamente e destrói a FIFO antes de terminar.

Divisão por ficheiros
=====================

A implementação divide-se nos 2 grupos de ficheiros exigidos no enunciado (balcao.c, balcao.h, ger_cl.c, ger_cl.c) e em mais 2 grupos.

### Principais

Os programas balcao e ger_cl (compilados com o comando make) permitem diversos tipos de utilização:

#### ger_cl
	
O ger_cl permite uma execução normal através da ordem de comandos especificada no enunciado (ger_cl <nome_mem_partilhada> <num_clientes>) mas, para além disso, se entre os argumentos se escrever "-db", em qualquer ordem (desde que, obviamente, depois de ger_cl), o programa descreve mais detalhadamente o que se passa internamente na sua execução, fazendo uma espécie de debug de ele próprio. Esta funcionalidade permite perceber a ordem pela qual as ações ocorrem na execução do programa uma vez que as mensagens geradas indicam quem as executa.
	
#### balcao
	
O balcao permite também a execução especificada no enunciado (balcao <nome_memoria_partilhada> <tempo_de_abertura>) bem como a funcionalidade "-db" descrita no parágrafo anterior.
Para além disso, se por algum motivo uma versão anterior de balcao terminar sem sucesso e se pretender "limpar" uma região de memória partilhada definida anteriormente, basta chamar o programa com "balcao -c <nome_memoria_partilhada>" para limpar (fazer unlink) dessa memória partilhada.

### Auxiliares

#### log.c e log.h

Nestes ficheiros está implementada uma interface que facilita o registo dos vários eventos ocorridos nos balcões e clientes, escrevendo essas informações no ficheiro correspondente a essa memória partilhada (p.e. loja.log), formatando-a numa tabela de texto onde é fácil perceber como se procederam os eventos do programa.
	
#### utils.c e utils.h

Nestes ficheiros estão definidas algumas funções com utilidade comum a balcões, clientes e, possivelmente, outros programas. As principais funcionalidades implementadas são funções que permitem simplificar o lock, unlock e destroy de mutexes, permitindo ao programa que as chamas fazer ou não debug do que acontece quando se utilizam os mutexes.
Para além disso, existem as funções parse_int e parse_long que tentam converter uma string para um inteiro (int ou long), retornando erro se não for possível (ou seja, se a string não representar um inteiro).
Por fim, existe a função filenameFromPath que permite converter uma string como /tmp/fc_1234, ou seja, uma string com o caminho completo para um ficheiro, numa string com apenas o nome do ficheiro correspondente (neste caso, fc_1234).
