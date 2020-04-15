/**

Autor: Edson Tavares de Camargo
11/2014

Algoritmo Hyperquicksort adaptado para suportar até n-1 processos falhos (crash),
onde n é o número total de processos.

Somente os processos que não falham executam a computação. Ao se detectar uma falha de um processo (do tipo crash)
o processo é excluído permantemente da execução da aplicação. As falhas de processos são detectadas através da
proposta de tolerância a falhas do MPI-Forum, a espeicificação ULFM (User-Level Failure Mitigation).

Após identificar a falha as primitivas da biblioteca ULFM devem ser usadas para revogar o comunicador
que contém o processo falho e estabelecer um novo comunicador que contém somente os que não falharam.
Não há tratamento da instabilidade nesta versão.

A falha de um processo é identificado através de um função chamada barreira, similar a MPI_Barrier(),
que sincroniza os processos e verifica se há algum código de erro (MPI_ERR_PROC_FAILED ou MPI_ERR_REVOKED)

Os processos falham aleatoriamento durante a execução do algoritmo

 mpiexec -np 8 -am ft-enable-mpi ./bin/Debug/HyperQuickSortFT_NovoModelo_Sequencial 1024KB.dat 1

./exec <nome_arquivo>  <número de nodos falhos>
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <mpi-ext.h>
#include <sys/types.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include "global.h"

#include "hyperquicksort_ft.h"

//#include "novo_modelo_diagnostico.h"

int main(int argc, char *argv[])
{
    int myRank;             /* identificador do processo */
    int size;               /* quantidade de processos no comunicador MPI */

    int tamPorcao;          /* quantidade de números recebida por cada processo */
    int tamPorcaoInstavel;  /* tamanho da porção de dados do processo instável */
    int posPivo;            /* posição do número do pivo usado na ordenação da porção de números*/

    int source = 0;         /* o processo pivô da rodada */
    int tMetade1;           /* tamanho das partes 1 e 2 considerando a posição do pivo no vetor, respectivamente */
    int tMetade2;
    int tMetade3;
    int tMetade4;
    int tb;                 /* tamanho do bloco recebido na troca das porções*/
    int tn;                 /* novo tamanho: quantidade de números que parmanece no vetor mais a nova quantidade recebida em bloco*/
    int tn2;
    int s;                  /* identifica o cluster de tamanho s*/
    int parceiro;           /* processo parceiro na troca das metades*/
    int i, j;               /* variáveis de incremento */
    int nRodada;            /* armazena a rodada de ordenação, iniciando em 1 indo até o tamanho do cluster*/
    int nNodosRodada;

    int cenario;             /* cenario de falhas */
    int *rodada;            /* falhas na rodada*/
    int *processo;          /* falhas nos processos */

    long int tam;           /* quantidade de número a serem ordenados*/
    int tagTroca = 4;       /* tag usada na troca das partes*/
    int receiveOk;          /* sinaliza que uma mensagem está disponível para recebimento */
    int nCluster;           /* tamanho do cluster */

    int *vetorInt;          /* vetor de números inteiros a ser ordenador em cada processo */
    int *vetorIntParceiro;  /* recuperar os dados de um nodo falho*/
    int *mapeamento;
    int *mapeamentoPivo;    /* os pivôs de cada nodo em uma determinada rodada*/
    int *meusInstaveis;
    int *bloco;             /* conjunto de números enviados e recebidos */
    int *meuBloco;

    double inicio;          /* começo do processamento */
    double fim;             /* fim do processamento */


    MPI_Status status;
    MPI_Comm comm_sorting;          /* comunicador atualizado. Será utilizado nas fuções coletivas, como a barreira, e
                                    para anotar os novos nodos falhos na função nodosFalhosGlobais()*/
    MPI_Comm comm_sorting_original;



    if(argc!= 3){
        printf("Informar o nome do arquivo e nível de instabilidade \n");
        printf("./exec <nome_arquivo>  <número de nodos falhos>  \n");
        exit(0);

    }else{
        strcpy(nomeArquivo, argv[1]);
        cenario = atoi(argv[2]);

    }

    tam = atribuiTamanho(nomeArquivo);
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&myRank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    /* error handler */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /* Duplicação dos comunidadores. O comm_sorting será sempre recuperado após uma falha, podendo
    assim ser usado nas operações coletivas.

    Já o comm_sorting_original permanecerá inalterado pois contém os ranks originais*/

    MPI_Comm_dup(MPI_COMM_WORLD, &comm_sorting);
    MPI_Comm_dup(MPI_COMM_WORLD, &comm_sorting_original);

    /* aloca uma quantidade maior de memoria para evitar novas alocações durante o processo. Inicialmente
    um processo precisas apenas de tam/size bytes de memória. Mas no final um processo pode conter
    todos os números distribuídos a melhor solução é alocar tam. */

    vetorInt = NULL;
    vetorIntParceiro = NULL;
    timestamp = NULL;
    mapeamento = NULL;
    mapeamentoPivo = NULL;
    meusInstaveis = NULL;

    tamPorcao = 0;
    tamPorcaoInstavel = 0;


    vetorInt = criaInt(tam);
    vetorIntParceiro = criaInt(tam);
    mapeamento = criaInt(size);
    mapeamentoPivo = criaInt(size);
    meusInstaveis = criaInt(size);
    timestamp = criaInt(size);

    // inicializa vetores
    for (i = 0; i < size; i++){

        timestamp[i] = ESTAVEL;   // todos os nodos estão estáveis

    }

    /* função que aleatoriamente escolhe a rodada e o processo em que ocorre a falha de acordo com o cenario (0,1,2,3) */
    insereFalhas(cenario, &processo, &rodada, size, myRank);

    /** DISTRIBUIÇÃO DOS NÚMEROS ENTRE OS PROCESSOS

    No início cada processo recebe a mesma quantidade de números, ou seja,
    o tamanho do conjunto divido pelo número de processos estáveis.**/

    /* Porção de números de cada processo.  */
    tamPorcao = (int) (tam/(size - nFalhos(size) ) );

    /* distribui blocos de números de igual tamanho entres os processos */
    distribuiNumerosProcessosFromFile(myRank, size, vetorInt, tamPorcao, tam, comm_sorting_original);

    /* cada processo ordena sua porção em ordem crescente */
    quick(vetorInt, tamPorcao);

    /* cada processo salva o seu bloco de números */
    salvarDados(myRank, vetorInt, tamPorcao);

    /**** INÍCIO DA ORDENAÇÃO ***********/
    inicio = MPI_Wtime();  // tempo em segundos

    barreira(myRank, size, &comm_sorting);

    /* Suposição: durante a ordenação não há a ocorrência de falhas.

    Para contemplar a ocorrência de falhas durante a ordenação seria preciso marcar o início e o fim do ordenação.
    Caso houver falhas então desfazer o que foi realizado e começar novamente a
    ordenação com base nesse cenário de falhas.*/

    /* número de clusters */
    nCluster = (int) log2(size);

    /* número da rodada de ordenação */
    nRodada = 0;

    //if(myRank == primeiroSemFalha(0, size)){
    //    printf("rodada: %d | falhos: %d\n", nRodada, nFalhos(size) );
    //}

    for (s = nCluster; s > 0; s--){

        MPI_Barrier(comm_sorting_original);

        nRodada++; // rodada começa em 1

        /* verifica quais processos devem falhar nessa rodada de acordo com o cenario*/
        aplicaFalhas(cenario, processo, rodada, size, myRank, nRodada);

        barreira(myRank, size, &comm_sorting);

        // Somente processos estáveis participam de uma rodada de ordenação
        if(timestamp[myRank] % 2 == ESTAVEL){


            // Mapeia quais processos estáveis assumem os processos instáveis de acordo com a função Cis
            mapeamentoInstavelEstavel(mapeamento, meusInstaveis, myRank, size, nCluster);

            // distribui o número pivo aos processos estáveis do cluster de acordo com o cluster de tamanho s
            distribuiNumeroPivo3(s, myRank, vetorInt, comm_sorting_original, mapeamento, mapeamentoPivo, size);

            // Cada processo verifica no vetor meusInstaveis quais processos deve assumir.
            i = 0;

            do{

                nNodosRodada = POW_2(s); // número de nodos por cluster na rodada

                source = (meusInstaveis[i] >> s) * nNodosRodada; // o pivo do nodo meusInstaveis[i]

                // Abre o arquivo do processo de sua responsabilidade e inclui os dados no vetor de inteiros

                lerDadosRank(meusInstaveis[i], vetorInt, &tamPorcao);

                /*** Procura o pivo e marca a sua posição **/
                /* cada processo busca o pivo entre os seus valores e retorna a posição
                    do pivo caso o encontre ou a posição do número imediatamente maior que o pivo.
                */

                posPivo = buscaBinaria(vetorInt, 0, tamPorcao-1, mapeamentoPivo[source]);

                /* A posição do pivo marca duas partes*/
                tMetade1 = posPivo;                 /* tamanho da primeira parte */
                tMetade2 = tamPorcao - posPivo;     /* tamanho da segunda parte*/
                tn = tMetade1 + tMetade2;           /* será usado para calcular o novo tamanho*/

                /***************** Troca das metades *******************/
                /* O processado de menor índice envia seus valores maiores do que seu valor médio (pivô)
                   para seu parceiro e recebe do seu parceiro os valores menores do que o valor médio.

                   O parceiro sera o primeiro sem falha da Cis
                */
                parceiro = encontrarParceiro2(meusInstaveis[i], s);

                /* parceiro de menor rank envia ao parceiro a sua segunda metade (números maiores que o pivo)*/
                if(meusInstaveis[i] < parceiro){

                    // nesse caso não é necessário sends e receives pois o mesmo processo vai lidar com os dados de dois arquivos diferentes
                    if(myRank == mapeamento[parceiro]){

                        printf("Rodada Ordenação %d (myRank: %d)  |  p%d <-> p%d \n", nRodada, myRank, meusInstaveis[i], parceiro );


                        // copia a segunda metade dos dados para "transmitir" para o parceiro
                        meuBloco = criaInt(tMetade2);

                        for(j = 0; j < tMetade2; j++){
                            meuBloco[j] = vetorInt[j+tMetade1];
                        }


                        // abre e armazena os dados de parceiro
                        lerDadosRank(parceiro, vetorIntParceiro, &tamPorcaoInstavel);

                        source = (parceiro >> s) * nNodosRodada;

                        posPivo = buscaBinaria(vetorIntParceiro, 0, tamPorcaoInstavel-1, mapeamentoPivo[source]);


                        // A posição do pivo marca duas partes
                        tMetade3 = posPivo;                 // tamanho da primeira parte
                        tMetade4 = tamPorcaoInstavel - posPivo;     // tamanho da segunda parte
                        tn2 = tMetade3 + tMetade4;           // será usado para calcular o novo tamanho

                        // copia a primeira metade dos dados para "transmitir" para o parceiro
                        bloco = criaInt(tMetade3);

                        for(j = 0; j < tMetade3; j++){
                            bloco[j] = vetorIntParceiro[j];
                        }

                        copia(vetorInt, bloco, 0, tMetade1, tMetade3);
                        free(bloco);

                        copia(vetorIntParceiro, meuBloco, tMetade3, tMetade4, tMetade2);
                        free(meuBloco);


                        // novo tamanho
                        tn = tMetade1 + tMetade3;
                        tn2 = tMetade4 + tMetade2;

                        // cada processo ordena o seu vetor v
                        quick(vetorInt, tn);
                        quick(vetorIntParceiro, tn2);

                        salvarDados(meusInstaveis[i], vetorInt, tn);
                        salvarDados(parceiro, vetorIntParceiro, tn2);

                    }else{

                        printf("Rodada Ordenação %d (myRank: %d) | p%d -> p%d \n", nRodada, myRank, meusInstaveis[i], parceiro );

                        /* enviar ao processo parceiro a segunda parte do processo myRank */
                        MPI_Send(&vetorInt[tMetade1], tMetade2 , MPI_INT, mapeamento[parceiro], tagTroca, comm_sorting_original );

                        /* receber do processo parceiro um bloco de números. Esse bloco será a nova segunda parte do
                        processo myRank */

                        do
                            MPI_Iprobe(mapeamento[parceiro], tagTroca, comm_sorting_original, &receiveOk, &status);
                        while(!receiveOk);

                        /* tb é o tamanho do bloco recebido */
                        MPI_Get_count(&status, MPI_INT, &tb);


                        /* vetor temporário que armazena os números recebidos de um processo parceiro*/
                        bloco = (int*)malloc (sizeof(int)* (tb)  );

                        if (bloco == NULL){
                            printf("\nNão foi possível alocar bloco com %d de mem", tb);
                            exit(0);
                        }

                        MPI_Recv(&bloco[0], tb , MPI_INT, mapeamento[parceiro], tagTroca, comm_sorting_original, MPI_STATUS_IGNORE);

                        /* primeira parte se mantem, incluir tb no fim*/
                        //juntaPartes(v, bloco, 0, tMetade1, tb);
                        copia(vetorInt, bloco, 0, tMetade1, tb);

                        free(bloco);

                        /* novo tamanho */
                        tn = tMetade1 + tb;

                        tamPorcao = tn;

                        /****** SALVA OS DADOS NO ARQUIVO ***/

                        /* cada processo ordena o seu vetor v */
                        quick(vetorInt, tamPorcao);

                        salvarDados(meusInstaveis[i], vetorInt, tamPorcao);
                    }

                }else if ( (meusInstaveis[i] > parceiro) && (myRank != mapeamento[parceiro])) {

                    // nesse caso não é necessário sends e receives pois o mesmo processo vai lidar com os dados de dois arquivos diferentes
                    //if(myRank == mapeamento[parceiro]){
                       // printf(" %d <- %d \n", meusInstaveis[i], parceiro );

                    //}else{
                        printf("Rodada Ordenação %d (myRank: %d) | p%d <- p%d \n", nRodada, myRank, meusInstaveis[i], parceiro );

                        /* receber do processo parceiro um bloco de números. Esse bloco será a sua nova primera parte */
                        do
                            MPI_Iprobe(mapeamento[parceiro], tagTroca, comm_sorting_original, &receiveOk, &status);
                        while(!receiveOk);

                        /* tb é o tamanho do bloco recebido */
                        MPI_Get_count(&status, MPI_INT, &tb);


                        /* vetor temporário que armazena os números recebidos de um processo parceiro*/
                        bloco = (int*)malloc (sizeof(int)* (tb)  );

                        if (bloco == NULL){
                            printf("\nNão foi possível alocar bloco com %d de mem", tb);
                            exit(0);
                        }

                        /* receber do processo parceiro uma bloco de números. Esse bloco será a nova primeira parte deste processo */
                        MPI_Recv(&bloco[0], tb, MPI_INT, mapeamento[parceiro], tagTroca, comm_sorting_original, MPI_STATUS_IGNORE);

                        /* enviar ao processo parceiro a primeira parte  */
                        MPI_Send(&vetorInt[0], tMetade1 , MPI_INT, mapeamento[parceiro], tagTroca, comm_sorting_original );

                        /*soprepor a primeira parte com a segunda parte e incluir tb*/
                        //juntaPartes(v, bloco, tMetade1, tMetade2, tb);
                        copia(vetorInt, bloco, tMetade1, tMetade2, tb);
                        free(bloco);

                        /* novo tamanho */
                        tn = tMetade2 + tb;

                        tamPorcao = tn;

                        /****** SALVA OS DADOS NO ARQUIVO ***/

                        /* cada processo ordena o seu vetor v */
                        quick(vetorInt, tamPorcao);

                        salvarDados(meusInstaveis[i], vetorInt, tamPorcao);
                   // }

                }
                //printf("%d (%d) -> %d (%d) | pivo: %d\n", myRank, meusInstaveis[i] , mapeamento[parceiro], parceiro, mapeamentoPivo[source] );
                i++;

            }while( (meusInstaveis[i] != -1) && (  i < size ) );

        }
        /** Monitoramento **/
        //printf("(myRank: %d) | fim da rodada %d de ordenação \n", myRank, nRodada);
        if(myRank == primeiroSemFalha(0, size)){
            printf("rodada: %d | falhos: %d\n", nRodada, nFalhos(size) );
        }

        MPI_Barrier(comm_sorting_original);

    }
    fim = MPI_Wtime();   // tempo em segundos

    if(myRank == primeiroSemFalha(0, size)){
        printf("tempo total: %.2f\n", fim - inicio);


    }


    free(vetorInt);
    free(vetorIntParceiro);
    free(mapeamento);
    free(mapeamentoPivo);
    free(meusInstaveis);
    free(rodada);
    free(processo);
    free(timestamp);

    MPI_Finalize();


    return 1;

}

