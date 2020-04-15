#include <math.h>
#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#define DESCONHECIDO -1     
#define ESTAVEL 0           
#define INSTAVEL 1

// PROCESSOS/RANKS
struct rank 
{
	int my_rank;                                     // IDENTIFICADOR DO PROCESSO
	int root_rank;							         // IDENTIFICADOR DO PROCESSO RAIZ
	int parceiro_rank;						         // IDENTIFICADOR DO PROCESSO PARCEIRO
	int comm_sz;							         // QUANTIDADE DE PROCESSOS
	int dim;                                         // DIMENSAO DO HIPERCUBO
	int nRodada;                                     // RODADA DE ORDENACAO ATUAL
	double start_time;
	double end_time;
	
	int *list;										 // LISTA LOCAL DE ELEMENTOS
	int *vetorTemp;                                  // VETOR TEMPORARIO
	int lengthList, lengthHighList, lengthLowList;   // QUANTIDADE DE ELEMENTOS DE CADA LISTA
	int lengthVetorTemp;                             // QUANTIDADE DE ELEMENTOS DO VETOR TEMPORÁRIO
	int pivo;										 // PIVÔ 
	int *splitter;                                   // VETOR DE PIVOS
	long int qtdeElementos;							 // QUANTIDADE TOTAL DE ELEMENTOS
	int elementosPorProcesso;						 // QUANTIDADE DE ELEMENTOS POR PROCESSO
	char *sArquivo;									 // NOME DO ARQUIVO A SER ORDENADO

	MPI_Comm Comm_origem;							 // COMUNICADOR DO MPI QUE CONTEM OS PROCESSOS ORIGINAIS
	MPI_Comm Comm;									 // COMUNICADOR DO MPI 

	int fatorMemoria;                                // FATOR ALOCAÇÃO DE MEMORIA
	int sizeList; 									 // MEMÓRIA ALOCADA PARA LISTA LOCAL DE ELEMENTOS
    int sizeVetorTemp;                               // MEMÓRIA ALOCADA PARA O VETOR TEMPORARIO

    int cenarioFalha;             			         // CENÁRIO DE FALHAS
    int *rodadasFalhas;                              // RODADAS EM QUE OCORREM FALHAS
    int *processosFalhos;                            // PROCESSOS FALHOS
    int *flagProcessos;                        		 // ESTADO DE CADA PROCESSO (ESTÁVEL OU INSTÁVEL)
    int *mapeamentoProcessos;                  		 // DETERMINA QUAIS PROCESSOS ESTAVEIS ASSUMIRAM OS PROCESSOS INSTÁVEIS
    int *meusInstaveis;                        		 // PROCESSOS INSTAVEIS DO PROCESSO (CASO ESTEJA ESTÁVEL)
    int qtdeProcessosFalhos;                   		 // QUANTIDADE ATUALIZADA DE PROCESSOS FALHOS
};

typedef struct rank Processo;

// ALOCA MEMÓRIA PARA AS LISTAS DE ELEMENTOS
int alocaMemoria(Processo *processo);
// LIMPA MEMÓRIA ALOCADA PARA O PROCESSO
void deleteProcesso(Processo *processo);
//ESCREVE VETOR RECEBIDO
void imprimeVetor(int *vetor, int tamanho);
//IMPLEMENTACAO DA FUNCAO POW COM UM CASTING PARA int 
int powCast(int b, int e);

//ORDENAÇÃO

// RETORNA ATRAVÉS DO VETOR v1, O MERGE ENTRE OS VETORES v1 E v2
long int merge(int *v1, int inicio_v1, int fim_v1, int *v2, int inicio_v2, int fim_v2);
// IMPLEMENTAÇÃO DO ALGORITMO QUICKMERGE: VERSÃO PARALELA DO ALGORITMO QUICKSORT
void quickMerge(Processo *processo);
//FUNCAO QUE COMPARA DOIS ELEMENTOS
int cmpfunc (const void * a, const void * b);
// RETORNA A POSIÇÃO DO PIVO EM UMA LISTA ORDENADA DE ELEMENTOS
int buscaBinaria(int *v, int inicio, int fim, int chave);
//RETORNA O TAMANHO DA MENSAGEM ENVIADA AO PROCESSO
int getMensagemLength(Processo *processo);

//ARQUIVOS

// SALVA EM BINÁRIO OS DADOS RECEBIDOS
void salvarDados(int rank, int *dados, int tam);
// RETORNA A LEITURA DE UM ARQUIVO BINÁRIO
void lerArquivo(Processo *processo, int *vetor);
//RETORNA A QUANTIDADE DE INTEIROS DO ARQUIVO
long int getTamanhoArquivo(char *sArquivo);
// RETORNA A LEITURA DE UM ARQUIVO BINÁRIO DO PROCESSO rank
long int lerDadosRank(int rank, int *v);

//TOLERÂNCIA A FALHAS          

// ESCOLHE DE ACORDO COM O CENÁRIO, ALEATORIAMENTE, QUAIS PROCESSOS DEVEM FALHAR
void insereFalhas(Processo *processo);
// DISTRIBUI ELEMENTOS DO ARQUIVO ORIGINAL PARA TODOS OS PROCESSOS
void distribuiElementosProcessosFromFile(Processo *processo);
// VERIFICA SE OCORREU FALHAS
void detectaFalhos(Processo *processo);
// IDENTIFICA QUAIS PROCESSOS FALHARAM
void getProcessosFalhos(Processo *processo);
// MATA O PROCESSO CASO SEJA O PROCESSO SORTEADO PARA FALHAR
void killRank(int my_rank, int rank_killed);
// APLICA AS FALHAS SORTEADAS NO INÍCIO DA ORDENAÇÃO
void aplicaFalhas(Processo *processo);
// RETORNA O PROCESSO QUE ASSUME O PROCESSO FALHO
int getRankAssumeFalho(Processo *processo, int rank, int s);
// ADICIONA OS PROCESSOS FALHOS QUE DEVERÃO SER ASSUMIDOS
void adicionaInstaveis(Processo *processo, int rank_instavel);
// CADA PROCESSO ESTÁVEL VERIFICA QUAIS PROCESSOS FALHOS DEVERÁ ASSUMIR
void mapeamento(Processo *processo);
// RETORNA O PRIMEIRO PROCESSO SEM FALHA
int getProcessoEstavel(Processo *processo);
