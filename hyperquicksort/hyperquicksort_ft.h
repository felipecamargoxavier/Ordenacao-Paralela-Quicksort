#include "cisj.h" // calculo da cis - usado para conhecer os parceiros de um nodo i em um cluster de tamanho s


/***** Functions Headers ***********/
int *criaInt(int tamanho);

void mapeamentoInstavelEstavel(int *mapeamento, int *instaveis, int myRank, int tam, int nCluster);

void insereFalhas(int cenario, int **processo, int **rodada, int size, int myRank);

int estaMeusRanks(int p, int *vetor, int tam);

long int atribuiTamanho(char *arquivo);

void copia(int *v, int *b, int vi, int tv, int tb);

void nodosFalhosGlobal(int size, MPI_Comm **world);

void recuperaDadosFalhos(int rank, int size, int *v, int nCluster, int *tamPorcao);

void lerDadosInicio(int rank, int *dados);

void lerDadosRank(int rank, int *dados, int *tam);

void salvarDados(int rank, int *dados, int tam);

int encontrarParceiro(int rank, int s);

int encontrarParceiro2(int rank, int s);

int encontrarParceiro3(int rankVirtual, int s, int myRank, int nCluster);

void barreira(int rank, int size, MPI_Comm *comm);

void distribuiNumeroPivo(int s, int myRank, long int tamBloco, int *pivo, MPI_Comm comm_sorting);

void distribuiNumeroPivo2(int s, int myRank, int *vetorInt, MPI_Comm comm_sorting, int *mapeamento);

void distribuiNumeroPivo3(int s, int myRank, int *vetorInt, MPI_Comm comm_sorting, int *mapeamento, int *mapPivo, int nNodos);

void juntaPartes(int *v, int *b, int vi, int vf, int tb);

int primeiroSemFalha(int inicio, int fim);

void repairComm(int error, int rank, int size, MPI_Comm *world);

int nFalhos(int size);

void killRank(int rank, int n);

/** Função recursiva do quicksort. */
void qs(int *v, int left, int rigth);

/** Função principal do quickort*/
void quick(int *v, int tam);

/** Geração dos números aleatório no vetor v de tamanho t*/
void geraNumeros(int *v, int t);

/** Função usada pela função ring para exibir os números contidos no vetor v
 de tamanho quant em cada processo. O parâmetro n se refere ao pivo de ordenação
 usado
*/
void exibe(int *v, int rank, int quant);

/**Função que distribui um bloco de números de tamanho t entre os nNodos processos
sem falha.
Cada processo tem uma quantidade tamV de números e os armazena no vetor v.
 **/
void distribuiNumerosProcessos(int myRank, int nNodos, int *v, int tamPorcao, int tam, MPI_Comm comm_s );

void distribuiNumerosProcessosFromFile(int myRank, int nNodos, int *v, int tamPorcao, long int tam, MPI_Comm comm_s );

/** Função que simula um anel de processos. Usada para imprimir o vetor v
iniciando do processo p0 sequencialmente até o processo N (size)*/
void ring(int rank, int size, int *v, int quant, MPI_Comm comm_s );


/** Busca Binária para encontrar a posição de um número igual a chave no vetor v.
Se não há um número igual a chave então retorna a posição entre o menor e o maior
número
*/
int buscaBinaria(int *v, int inicio, int fim, int chave);

/***** Functions Headers end ***********/



/***** functions ****************/


/*
  Um anel. O processo 0 envia para o processo 1 e assim por diante
  até o último receber.

  Usado para saida
*/
void ring(int rank, int size, int *v, int quant, MPI_Comm comm_s){
    int n = 0; // um sinal "pode imprimir".
    int tag = 80;

    if (rank == 0) {
        exibe(v, rank, quant);
        MPI_Send( &n, 1, MPI_INT, rank + 1, tag, comm_s );

    }
    else {
        //printf("Process %d waiting\n", rank);
         MPI_Recv( &n, 1, MPI_INT, rank - 1, tag, comm_s, MPI_STATUS_IGNORE );
         exibe(v, rank, quant);
         if (rank < size - 1)
             MPI_Send( &n, 1, MPI_INT, rank + 1, tag, comm_s );
    }
}

/**Função que gera e distribui um bloco de tamanho t dos números entre os processos
não falhos.

O processo sem falha de menor rank é responsável por gerar os números e distribuí-los
aos demais processos.
**/
void distribuiNumerosProcessos(int myRank, int nNodos, int *v, int tamPorcao, int tam, MPI_Comm comm_s ){
    int *numeros; /* números gerados pelo processo sem falha de menor rank*/
    int source; /* responsável por gerar o números e distribuí-los*/
    int i;
    int j;
    int qtdadeNum;
    int tag_distr = 30;

    // procurar o primeiro nodo sem falha entre todos os processos
    source = primeiroSemFalha(0, nNodos);

    numeros = NULL;

    if(myRank == source){
        //printf("process %d generating numbers\n", rank);
        srand(time(NULL) );
        numeros = (int*)malloc(sizeof(int) * tam);
        if(numeros == NULL){
            printf("\nNão foi possível alocar numeros com %d de mem", tam);
            exit(0);
        }
        geraNumeros(numeros, tam); /* função geradora dos números*/
        j = 0;
        for(i = 0; i < nNodos; i++){
            if(timestamp[i] == ESTAVEL){
                 qtdadeNum = j * tamPorcao;
                 MPI_Send(&numeros[qtdadeNum], tamPorcao, MPI_INT, i, tag_distr, comm_s );
                 printf("qtdadeNum %d\n", qtdadeNum);
                 j++;
            }

        }
    }
    MPI_Recv(&v[0], tamPorcao, MPI_INT, source, tag_distr, comm_s, MPI_STATUS_IGNORE );

    free(numeros);

}

void geraNumeros(int *v, int t){
    int i;

    for (i = 0; i < t; i = i + 4){
        *(v + i) = (rand() % (t*10) ) + i;
        *(v + i+1) = (rand() % (t*10) ) + i+1;
        *(v + i+2) = (rand() % (t*10) ) + i+2;
        *(v + i+3) = (rand() % (t*10) ) + i+3;
    }
}

void exibe(int *v, int rank, int quant){
    int i;

    printf("Process %d |", rank);
    for(i = 0; i < quant; i++){
        printf("%d  ", v[i]);
    }
    printf("\nTOTAL: %d\n", i);
    printf("\n---------------------------------\n");

}

/** Copia o bloco de números b no vetor v de forma que os números fiquem
intercalados */

void copia(int *v, int *b, int vi, int tv, int tb){
    int i = 0;
    int j = 0;
    int *temp;
    int menor = tv;
    int maior = tb;
    int tam;

    if ( menor > tb){
        maior = menor;
        menor = tb;
    }
    tam = menor + maior;

    temp = (int*) malloc(sizeof(int)*(tam) );

    if (temp == NULL){
        printf("\nNão foi possível alocar tem com de mem");
        exit(0);
    }
    /* tanto o conteúdo do vetor v quanto do vetor b são copiados para temp de forma intercalada até o tamanho de menor*/
    while(i < menor*2 ){
        *(temp + i) = *(v + vi + j);
        i++;
        *(temp + i) = *(b + j);
        i++;
        j++;
    }

    /* necessário copiar mais dados em temp*/
    if (menor != maior){
        j = maior - menor;
        if(maior == tb){
            while (j > 0){
                *(temp+i) = *(b + maior - j);
                i++;
                j--;
            }
        }
        else{
            while (j > 0){
                *(temp+i) = *(v + vi + maior - j);
                i++;
                j--;
            }
        }

    }
    /* copia os dados novamente em v */

    for (i = 0; i < tam; i = i + 4){
        *(v+i) = *(temp+i);
        *(v+i+1) = *(temp+i+1);
        *(v+i+2) = *(temp+i+2);
        *(v+i+3) = *(temp+i+3);
    }
    free(temp);
}

/** Copia o bloco de números b no vetor v de forma que os números fiquem
intercalados */

void juntaPartes(int *v, int *b, int vi, int vf, int tb){
    int i;
    int j;

    /* primeira parte se mantem, incluir tb no fim*/
    if (vi == 0){
        for (i = vf, j = 0; j < tb; j++, i++){
            v[i] = b[j];
        }

    }else{ /*soprepor a primeira parte com a segunda parte e incluir tb*/
        for (i = vi, j = 0; j < vf; j++, i++){
            v[j] = v[i];
        }
        /*incluir o bloco no fim*/
        for (i = 0; i < tb; i++, j++){
            v[j] = b[i];

        }
    }
}

int buscaBinaria(int *v, int inicio, int fim, int chave){

    while (inicio <= fim){
        int i = (inicio+fim)/2;

        if( *(v+i) == chave){
            return i;
        }

        else if(*(v+i) < chave){
            return buscaBinaria(v,i+1,fim,chave);
        }
        else{
            return buscaBinaria(v,inicio,i-1,chave);
        }
    }
    return inicio;
}

/** Função principal do algorito quicksort*/
void quick(int *v, int tam){
    qs(v, 0, tam-1);
}
/** Função recursiva do quicksort. O vetor v com início em left e fim em rigth
é rearranjado de acordo com uma chave retirada do próprio vetor v. Os números
menor que a chave ficam na sua esquerda e os números maiores que a chave ficam
na sua direita.
A chave é escolhida no meio do vetor */

void qs(int *v, int left, int rigth){
    register int i, j;
    int x, y;
    i = left;
    j = rigth;
    x = v[(left+rigth)/2];
    do{
        while( (v[i] < x) && (i < rigth) ) i++;
        while((x < v[j]) && (j > left) ) j--;

        if (i <= j){
            y = v[i];
            v[i] = v[j];
            v[j] = y;
            i++;
            j--;
        }
    }while(i <= j);

    if(left < j)
        qs(v,left,j);
    if(i < rigth)
        qs(v, i, rigth);
}

void killRank(int rank, int n){
     /* Matar o processo 1*/
    if(rank == n){
        kill(getpid(), SIGKILL);
    }
}

int primeiroSemFalha(int inicio, int fim){
    while( inicio < fim ){

        if(timestamp[inicio] == ESTAVEL)
            return inicio;
        inicio++;
    }
    return -1;

}

int nFalhos(int size){
    int i;
    int cont = 0;
    for(i = 0; i < size; i++){
        if (timestamp[i] % 2 == INSTAVEL)
            cont++;
    }
    return cont;
}


void reparaComm(int erro, int rank, MPI_Comm *comm_s){
   int verificaErros;
   MPI_Comm temp;

   if (erro == MPI_ERR_PROC_FAILED){
       printf("Repara comunicador do processo: %d\n", rank);

       verificaErros = (erro == MPI_SUCCESS);
       MPIX_Comm_agree(*comm_s, &verificaErros);

       if(!verificaErros) {
            printf("Acordo %d \n", rank);
            MPIX_Comm_revoke(*comm_s);
            MPIX_Comm_shrink(*comm_s, &temp);
            MPI_Comm_free(&(*comm_s) );
            *comm_s = temp;
        }
    }
}

void distribuiNumeroPivo(int s, int myRank, long int tamBloco, int *pivo, MPI_Comm comm_sorting){

    int raiz;           /* processo raiz de uma rodada */
    int nodosRodada;    /* quantidade de nodos a cada rodada que receberão o número pivo enviado pelo processo raiz */
    int tag_pivo = 20;
    int k;
    int source;         /* primeiro nodo do cluster s*/

    nodosRodada = POW_2(s);
    source = (myRank >> s) * nodosRodada;
    raiz = primeiroSemFalha(source, (source + nodosRodada) );

    /*Se existe um nodo sem falha no cluster*/
    if(raiz != -1){
        /* sou a raiz. Raiz distribui número médio, pivô, para todo os processos  */
        if (myRank == raiz  ){
            k = 1;
            while( k < nodosRodada ){
                if( (timestamp[source + k] == ESTAVEL) && ( (source + k) != myRank ) ){
                    MPI_Send( &(*pivo), 1, MPI_INT, (source + k), tag_pivo, comm_sorting);

                }
                k++;
            }
        }
        else{
            if(timestamp[myRank]!= ESTAVEL){
                MPI_Recv(&(*pivo), 1, MPI_INT, raiz, tag_pivo, comm_sorting, MPI_STATUS_IGNORE);
            }
        }
    }
}

void distribuiNumeroPivo2(int s, int myRank, int *vetorInt, MPI_Comm comm_sorting, int *mapeamento){

    int source;           /* processo raiz de uma rodada */
    int nodosRodada;    /* quantidade de nodos a cada rodada que receberão o número pivo enviado pelo processo raiz */
    int tag_pivo = 20;
    int k;
    int tamPorcao;
    int pivo;

    nodosRodada = POW_2(s);
    source = (myRank >> s) * nodosRodada;

    /* sou a raiz. Raiz distribui número médio, pivô, para todo os processos  */
    if (myRank == mapeamento[source]  ){

        lerDadosRank(source, vetorInt, &tamPorcao);

        pivo = *(vetorInt + ( (int) tamPorcao/2) );

        k = 1;
        while( k < nodosRodada ){
            if( (timestamp[source + k] == ESTAVEL) && ( (source + k) != myRank ) ){
                MPI_Send( &pivo, 1, MPI_INT, (source + k), tag_pivo, comm_sorting);
                //printf("%d (%d) send (%d) to %d\n", myRank, source, pivo, (source + k));

            }else if( (timestamp[source + k] == ESTAVEL) && ( (source + k) == myRank ) ){
                //printf("%d (%d) send (%d) to %d\n", myRank, source, pivo, (source + k));
            }
            k++;
        }
    }
    else{
            MPI_Recv(&pivo, 1, MPI_INT, mapeamento[source], tag_pivo, comm_sorting, MPI_STATUS_IGNORE);
            //printf("%d receive (%d) from %d \n",myRank, pivo, mapeamento[source]);

    }


}

void distribuiNumeroPivo3(int s, int myRank, int *vetorInt, MPI_Comm comm_sorting, int *mapeamento, int *mapPivo, int nNodos){

    int source;           /* processo raiz de uma rodada */
    int nodosRodada;    /* quantidade de nodos a cada rodada que receberão o número pivo enviado pelo processo raiz */
    int tag_pivo = 20;
    int k, i;
    int tamPorcao;
    int pivo;

    nodosRodada = POW_2(s);
    for(i = 0; i < nNodos; i++ ){

        if( (i % nodosRodada) == 0){  // significa que i é pivo da rodada

            source = i;

            if( myRank == mapeamento[i] ){

                lerDadosRank(source, vetorInt, &tamPorcao);

                pivo = *(vetorInt + ( (int) tamPorcao/2) );
                mapPivo[source] = pivo;

                k = 1;

                while( k < nodosRodada ){

                    if(myRank !=  mapeamento[(source + k)] ){
                        //printf("%d (%d) send %d to %d (%d)\n", myRank, source, pivo, mapeamento[source + k], (source + k));
                        MPI_Send( &pivo, 1, MPI_INT, mapeamento[(source + k)], tag_pivo, comm_sorting);

                    }
                    k++;
                }


            }

        }else{ // não é o pivo da rodada então recebe o número pivo
            if( (myRank == mapeamento[i])  ){

                source = (i >> s) * nodosRodada;

                if (myRank != mapeamento[source]){


                    MPI_Recv(&pivo, 1, MPI_INT, mapeamento[source], tag_pivo, comm_sorting, MPI_STATUS_IGNORE);
                    //printf("%d (%d) receive %d from %d (%d) \n", myRank, i, pivo, mapeamento[source], source);



                    mapPivo[source] = pivo;
                }else{
                    //printf("%d (%d) atribui %d from %d (%d) \n", myRank, i, pivo, mapeamento[source], source);
                    mapPivo[source] = pivo;
                }
            }
        }

    }
}

/**
A função repairComm submete todos os processo sem falha a uma operação coletiva por meio da
função OMPI_Comm_agree. Ao final, todos os processos sem falha acordam no resultado da variável
&errorAll. Cada processo contribui com um valor lógico (verdadeiro ou falso) e uma operação AND
é realizada sobre o conjunto de entradas.

*/

void repairComm(int error, int rank, int size, MPI_Comm *world){
   int errorAll;
   MPI_Comm temp;
   //printf("Repairing: %d\n", rank);
   errorAll = (error == MPI_SUCCESS);
   MPIX_Comm_agree(*world, &errorAll);

   if(!errorAll) {
        //printf("Agree %d \n", rank);
        MPIX_Comm_revoke(*world);
        /**** lista global*/
        nodosFalhosGlobal(size, &world);

        MPIX_Comm_shrink(*world, &temp);
        MPI_Comm_free(&*world);
        *world = temp;
    }
   // printf("Repairing ok: %d\n", rank);

}

/** A função barreira atua tanto como sincronizadora dos processos quanto para recuperação do comunicador
no caso de falhas. Além disso, é através da recuperação dos processos falhos que o vetor de timestamp,
o qual identifica os nodos falhos e sem falhas, é preenchido. */

void barreira(int rank, int size, MPI_Comm *comm){
    int erro;
    erro = MPI_Barrier(*comm); // caso algum processo tenha falhado o retorno será um erro
    if( (erro == MPI_ERR_PROC_FAILED) || (erro == MPI_ERR_REVOKED ) ){
        repairComm(erro, rank, size, &(*comm) );
    }

}

int encontrarParceiro(int rank, int s){
    int j = 0;
    node_set* nodes;       /* Nodos da cisj*/
    int parceiro;

    nodes = cis(rank,s);

    /* procurar o primeiro nodo sem falha */

    while( (j < nodes->size) && (timestamp[ nodes->nodes[j] ] % 2 == INSTAVEL)   )
            j++;

    if(j == nodes->size)
       parceiro = -1;
    else
        parceiro = nodes->nodes[j];

    set_free(nodes);

    return parceiro;
}


int encontrarParceiro2(int rank, int s){
    int j = 0;
    node_set* nodes;       /* Nodos da cisj*/
    int parceiro;

    nodes = cis(rank,s);

    parceiro = nodes->nodes[j];

    set_free(nodes);

    return parceiro;
}

int encontrarParceiro3(int rankVirtual, int s, int myRank, int nCluster){
    int  j = 0;
    node_set* nodes;       /* Nodos da cisj*/
    int parceiro = -1;


    while( (s > 0) && (parceiro == -1) ) {

        nodes = cis(rankVirtual, s);

        /* procurar o primeiro nodo sem falha */
        j = 0;
        while( (j < nodes->size) &&  (timestamp[ nodes->nodes[j] ] == INSTAVEL) )
            j++;

        if(j == nodes->size)
            parceiro = -1;
        else
            parceiro = nodes->nodes[j];

        set_free(nodes);
        s--;
    }

    return parceiro;
}

void distribuiNumerosProcessosFromFile(int myRank, int nNodos, int *v, int tamPorcao, long int tam, MPI_Comm comm_s ){

    int *numeros; /* números gerados pelo processo sem falha de menor rank*/
    int source; /* responsável por gerar o números e distribuí-los*/
    int i;
    int j;
    int k;
    int l;
    int qtdadeNum;
    int tag_distr = 30;

    // procurar o primeiro nodo sem falha entre todos os processos
    source = primeiroSemFalha(0, nNodos);

    numeros = NULL;

    if(myRank == source){

        numeros = (int*)malloc(sizeof(int) * tam);
        if(numeros == NULL){
            printf("\nNão foi possível alocar numeros com %ld de mem", tam);
            exit(0);
        }
        lerDadosInicio(myRank, numeros);

        j = 0;

        for(i = 0; i < nNodos; i++){
            if(timestamp[i] == ESTAVEL){
                qtdadeNum = j * tamPorcao;
                if(myRank != i){
                    MPI_Send(&numeros[qtdadeNum], tamPorcao, MPI_INT, i, tag_distr, comm_s );
                }else{
                    for (k = qtdadeNum, l = 0; k < qtdadeNum + tamPorcao; k = k + 4, l = l + 4){
                        v[l] = numeros[k];
                        v[l+1] = numeros[k+1];
                        v[l+2] = numeros[k+2];
                        v[l+3] = numeros[k+3];
                    }
                }
                j++;
            }

        }
    }else{
       // printf("%d | aguardando recebimento \n", myRank);
        MPI_Recv(&v[0], tamPorcao, MPI_INT, source, tag_distr, comm_s, MPI_STATUS_IGNORE );
       //printf("%d | finalizado recebimento \n", myRank);

    }


    free(numeros);
}

void lerDadosInicio(int rank, int *dados){
    FILE *file;
    int i = 0;

    if( (file = fopen(nomeArquivo, "rb")) == NULL){
        printf("arquivo não pode ser aberto %d\n", rank);
        return;
    }

    while(!feof(file)){
        if( fread(&dados[i], sizeof(int), 1, file) != 1 ){
            if(feof(file)) break;
            printf("Erro de leitura");
        }
        i++;
    }
    fclose(file);

}

void salvarDados(int rank, int *dados, int tam){
    FILE *file;
    char file_name[10];
    sprintf(file_name,"rank_%d.dat", rank );
    int i = 0;

    if( (file = fopen(file_name, "wb")) == NULL){
        printf("arquivo não pode ser aberto %d\n", rank);
        return;
    }
    for(i = 0; i < tam; i++){
        if( fwrite(&dados[i], sizeof(int), 1, file) != 1 ){
            printf("Erro de leitura %s", file_name);
        }
    }
    fclose(file);
}

void lerDadosRank(int rank, int *dados, int *tam){
    FILE *file;
    char file_name[10];
    sprintf(file_name,"rank_%d.dat", rank );
    int i = 0;

    if( (file = fopen(file_name, "rb")) == NULL){
        //printf("arquivo não pode ser aberto %d\n", rank);
        *tam = 0;
        return;
    }
    while(!feof(file)){
        if( fread(& (* (dados + i ) ), sizeof(int), 1, file) != 1 ){
            if(feof(file)) break;
            printf("Erro de leitura %s", file_name);
        }
        i++;
    }
    *tam = i;
    fclose(file);
}

void removeFile(int rank){
    char file_name[10];
    sprintf(file_name,"rank_%d.dat", rank );
    if (remove(file_name) == -1) {
        printf("Erro de remocao %s", file_name);
    }

}

/*
Define quais processos estáveis assumem os dados dos nodos instáveis.
*/
void recuperaDadosFalhos(int rank, int size, int *v, int nCluster, int *tamPorcao){
    int i;
    int j;
    int rankAssumeFalho;
    int tb;
    int tPorcao;
    tPorcao = *tamPorcao;
    //j = 1; /*cluster inicial é o 1*/
    for (i = 0; i < size; i++){ /* é possível otimizar */
        if( timestamp[i] == INSTAVEL ){
            j = 1;
            while ( ( (rankAssumeFalho = encontrarParceiro(i, j) ) == -1) && (j <= nCluster) ){
                j++;
            }
            if(rankAssumeFalho != -1){
                if (rank == rankAssumeFalho){
                    //printf("%d assume %d\n", rank, i);
                    lerDadosRank(i, &v[tPorcao], &tb);
                    if(tb > 0){
                        tPorcao = tPorcao + tb;
                        removeFile(i);
                    }
                }
            }
        }
    }
    *tamPorcao = tPorcao;
}

/** Esta função recuperar lista global de nodos falhos ***/
void nodosFalhosGlobal(int size, MPI_Comm **world){

    int size_old;
    int rank_old;
    int *temp_ranks;
    int *failed_ranks;
    int size_failed;
    int i, j;
    int cont;

    MPI_Group failed_group;
    MPI_Group group_old;

    MPI_Comm_size(**world, &size_old );   /*quantidade de processos original*/
    MPI_Comm_group(**world, &group_old);  /* cria um grupo com os processos originais*/
    MPI_Comm_rank(**world, &rank_old);    /*rank dos processos originais*/
    MPIX_Comm_failure_ack(**world);       /* conhecimento local das falhas*/
    MPIX_Comm_failure_get_acked(**world, &failed_group); /* grupo com os processos falhos*/
    MPI_Group_size(failed_group, &size_failed);

    /*descobrir processos que falharam */
    temp_ranks = (int*) malloc(sizeof(int) * size_old);
    failed_ranks = (int*) malloc(sizeof(int) * size_old);

    for(i = 0; i < size_old; i++){
        temp_ranks[i] = i;
    }

    if( (temp_ranks == NULL) || (failed_ranks == NULL)){
        printf("mem problem\n");
        exit(0);
    }

    MPI_Group_translate_ranks(failed_group, size_failed, temp_ranks, group_old, failed_ranks);

    if( numFalhos == 0){ /* primeira vez que houve falha. Mapeamento direto */
        for(i = 0; i < size_failed; i++){
            timestamp[ failed_ranks[i] ] = INSTAVEL;
        }
    }
    else{ /* ranks mudaram. Precisa mapear*/
        /* ordena em ordem crescente o vetor de ranks falhos*/
        quick(failed_ranks, size_failed);

        /* começa pelo maior  */
        for(i = size_failed-1; i >= 0; i--){
            cont = -1;
            j = 0;
            // printf("%d | failed %d\n", rank, failed_ranks[i]  );
            while( (cont < failed_ranks[i]) && (cont < size)  ){
                if (timestamp[j] == ESTAVEL){
                    cont++;
                }
                j++;
            }
            timestamp[j - 1] = INSTAVEL;
        }
    }

    numFalhos = numFalhos + size_failed; /* quantidade total de processos falhos */

    free(temp_ranks);
    free(failed_ranks);
    MPI_Group_free(&failed_group);
    MPI_Group_free(&group_old);

}

long int atribuiTamanho(char *arquivo){

    if(strcmp(arquivo, "1MB.dat") == 0)
        return 1024000;
    else if(strcmp(arquivo, "100MB.dat") == 0)
        return 102400000;
    else if(strcmp(arquivo, "1GB.dat") == 0)
        return 1024000000;
    return 0;
}

void insereFalhas(int cenario, int **processo, int **rodada, int size, int myRank){
    int temp;
    int i, j;
    int *p;
    int *r;

    p = NULL;
    r = NULL;
    srand(time(NULL));


    switch (cenario){
        case 0:         // sem falhas

            break;
        case 1:         // 1 falha

            r = (int*) malloc(sizeof(int) * 1);
            p = (int*) malloc(sizeof(int) * 1);

            if( (r == NULL) || (p == NULL)){
                printf("nao ha memoria \n");
                exit(0);
            }

            *r = rand() % (int)log2(size) + 1;
            *p = rand() % size;

            printf("Process %d - rodada: %d, processo: %d \n", myRank, *r, *p);
            break;
        case 2:         // metade das falhas

            r = (int*) malloc(sizeof(int) * (int) (size/2) );
            p = (int*) malloc(sizeof(int) * (int) (size/2));

            if( (r == NULL) || (p == NULL)){
                printf("nao ha memoria \n");
                exit(0);
            }

            for(i = 0; i < size/2; i++){
                r[i] = rand() % (int)log2(size) + 1;

                do{
                    temp = rand() % size;

                    j = 0;
                    while( (temp != p[j]) && (j <= i) ){
                        j++;
                    }

                }while(j < i);

                p[i] = temp;

                printf("Process %d - rodada: %d, processo: %d \n", myRank, r[i], p[i]);
            }

            break;
        case 3:         // size - 1 falhas

            r = (int*) malloc(sizeof(int) * (int) (size - 1) );
            p = (int*) malloc(sizeof(int) * (int) (size - 1));

            if( (r == NULL) || (p == NULL)){
                printf("nao ha memoria \n");
                exit(0);
            }

            for(i = 0; i < (size - 1); i++){
                r[i] = rand() % (int)log2(size) + 1;

                do{
                    temp = rand() % size;

                    j = 0;
                    while( (temp != p[j]) && (j <= i) ){
                        j++;
                    }

                }while(j < i);

                p[i] = temp;

                printf("Process %d - rodada: %d, processo: %d \n", myRank, r[i], p[i]);
            }

            break;
    }

    *processo = p;
    *rodada = r;
}

void aplicaFalhas(int cenario, int *processo, int *rodada, int size, int myRank, int nRodada){
    int i;

    switch (cenario){
        case 0:         // sem falhas

            break;
        case 1:         // 1 falha

            if(nRodada == *rodada){
              // printf("kill - Process %d - rodada: %d, processo: %d \n", myRank, *rodada, *processo);
               killRank(myRank, *processo);
            }

            break;
        case 2:         // metade das falhas

            for(i = 0; i < size/2; i++){

                if(nRodada == rodada[i]){
                    killRank(myRank, processo[i]);
                }
            }

            break;
        case 3:         // size - 1 falhas

            for(i = 0; i < size - 1; i++){

                if(nRodada == rodada[i]){
                    killRank(myRank, processo[i]);
                }
            }
            break;
    }
}

void adicionaProcessosInstaveis(int *vetor, int tam, int processo){

    int i = 0;

    while( (vetor[i] != -1) && (i < tam) ){
        i++;
    }
    vetor[i] = processo;

}

int estaMeusRanks(int p, int *vetor, int tam){
    int i = 0;
    while( (vetor[i]!= -1 ) && (i < tam)){

        if (vetor[i] == p)
            return 1;

        i++;

    }
    return 0;
}

/**
    Cada processo estável verifica quais processos instáveis deve assumir. Um processo estavel por assumir de 1 até n-1 processos instáveis

    timestamp - informa os processos estáveis e instáveis
    mapeamento - estabelece qual processo estável assumiu o processo instavel i
    instaveis - os processos instáveis do processo estável p_i
*/
void mapeamentoInstavelEstavel(int *mapeamento, int *instaveis, int myRank, int tam, int nCluster){

    char aux[32];           // auxiliar na impressão
    char vector[1024];
    int rankAssumeFalho;
    int i, j;

    vector[0] = '\0';
    for (i = 0; i < tam; i++){
        // redefine o vetor de instaveis
        instaveis[i] = -1;

        if( timestamp[i] % 2 == INSTAVEL ){

            j = 1;

            while ( ( (rankAssumeFalho = encontrarParceiro(i, j) ) == -1) && (j <= nCluster) ){
                j++;
            }
            mapeamento[i] = rankAssumeFalho;

            if(myRank == rankAssumeFalho){
                adicionaProcessosInstaveis(instaveis, tam, i);
            }
        }
        else{
            mapeamento[i] = i;

            if(myRank == i){
                adicionaProcessosInstaveis(instaveis, tam, i);
            }
        }
        sprintf(aux, "  %d ", mapeamento[i]);
        strcat(vector, aux);
    }
//    printf("%d | %d | %s \n", rodada, myRank, vector);
    printf("Mapeamento (myRank %d) | %s \n", myRank, vector);

}

int *criaInt(int tamanho){
    int *aux;

    aux = (int*)malloc (sizeof(int) * tamanho );

    if ( aux == NULL ) {
        printf("\nNão foi possível criar estrutura de tamanho %d, \n", tamanho);
        exit(0);
    }
    return aux;

}

