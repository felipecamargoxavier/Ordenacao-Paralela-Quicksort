
#include "QuickMerge.h"
#include "cisj.h"

// ESCOLHE DE ACORDO COM O CENÁRIO, ALEATORIAMENTE, QUAIS PROCESSOS DEVEM FALHAR
void insereFalhas(Processo *processo)
{
    int temp;
    int i, j;
    
    srand(time(NULL));

    switch (processo->cenarioFalha)
    {
        case 0:         //SEM FALHAS
            break;
        case 1:         //1 FALHA
            processo->rodadasFalhas = (int*) malloc(sizeof(int) * 1);
            processo->processosFalhos = (int*) malloc(sizeof(int) * 1);

            if( (processo->rodadasFalhas == NULL) || (processo->processosFalhos == NULL))
            {
                printf("%d: Nao foi possivel alocar memoria.\n", processo->my_rank);
                exit(0);
            }

            *(processo->rodadasFalhas) = rand() % (int)log2(processo->comm_sz)+1;
            *(processo->processosFalhos) = rand() % processo->comm_sz;
            break;
        case 2:         //METADE DOS PROCESSOS FALHAM
            processo->rodadasFalhas = (int*) malloc(sizeof(int) * (int) (processo->comm_sz/2) );
            processo->processosFalhos = (int*) malloc(sizeof(int) * (int) (processo->comm_sz/2));
            
            if( (processo->rodadasFalhas == NULL) || (processo->processosFalhos == NULL))
            {
                printf("%d: Nao foi possivel alocar memoria.\n", processo->my_rank);
                exit(0);
            }

            for(i = 0; i < processo->comm_sz/2; i++)
            {
                processo->rodadasFalhas[i] = rand() % (int)log2(processo->comm_sz)+1;

                do
                {
                    temp = rand() % processo->comm_sz;
                    j = 0;
                    while( (temp != processo->processosFalhos[j]) && (j <= i) )
                        j++;

                }while(j < i);

                processo->processosFalhos[i] = temp;
            }
            break;
        case 3:         //n -1 FALHAS, n: NUMERO DE PROCESSOS
            processo->rodadasFalhas = (int*) malloc(sizeof(int) * (int) (processo->comm_sz - 1) );
            processo->processosFalhos = (int*) malloc(sizeof(int) * (int) (processo->comm_sz - 1));

            if( (processo->rodadasFalhas == NULL) || (processo->processosFalhos == NULL))
            {
                printf("%d: Nao foi possivel alocar memoria.\n", processo->my_rank);
                exit(0);
            }

            for(i = 0; i < (processo->comm_sz - 1); i++)
            {
                processo->rodadasFalhas[i] = rand() % (int)log2(processo->comm_sz)+1;

                do
                {
                    temp = rand() % processo->comm_sz;
                    j = 0;
                    while( (temp != processo->processosFalhos[j]) && (j <= i)) 
                        j++;

                }while(j < i);

                processo->processosFalhos[i] = temp;
            }
            break;
    }
}

// RETORNA O PRIMEIRO PROCESSO SEM FALHA
int getProcessoEstavel(Processo *processo)
{
    int i = 0;

    while( i < processo->comm_sz )
    {
        if(processo->flagProcessos[i] == ESTAVEL)
            return i;
        i++;
    }

    return -1;
}

// DISTRIBUI ELEMENTOS DO ARQUIVO PARA TODOS OS PROCESSOS
void distribuiElementosProcessosFromFile(Processo *processo)
{
    int *numeros, *splitterLocal; 
    int source;   
    int i = 0;
    int j, k, l;
    int qtdadeNum;
    int tag_distr = 30;

    // PROCURA O PRIMEIRO PROCESSO SEM FALHA
    source = getProcessoEstavel(processo);
    if(source == -1)
    {
        printf("ERRO: Todos os processos estao falhos.\n");
        exit(0);
    }

    numeros = NULL;

    if(processo->my_rank == source)
    {
        numeros = (int*)malloc(sizeof(int) * processo->qtdeElementos);

        if(numeros == NULL)
        {
            printf("\n%d: Nao foi possivel alocar memoria.\n", processo->my_rank);
            exit(0);
        }

        lerArquivo(processo, numeros);

        j = 0;

        for(i = 0; i < processo->comm_sz; i++)
        {
            if(processo->flagProcessos[i] == ESTAVEL)
            {
                qtdadeNum = j * processo->elementosPorProcesso;
                if(processo->my_rank != i)
                {
                    MPI_Send(&numeros[qtdadeNum], processo->elementosPorProcesso, MPI_INT, i, tag_distr, processo->Comm_origem);
                }
                else
                {
                    for (k = qtdadeNum, l = 0; k < qtdadeNum + processo->elementosPorProcesso; k = k + 4, l = l + 4)
                    {
                        processo->list[l]   = numeros[k];
                        processo->list[l+1] = numeros[k+1];
                        processo->list[l+2] = numeros[k+2];
                        processo->list[l+3] = numeros[k+3];
                    }
                }

                j++;
            }
        }

        free(numeros);
    }
    else
    {
        MPI_Recv(&processo->list[0], processo->elementosPorProcesso, MPI_INT, source, tag_distr, processo->Comm_origem, MPI_STATUS_IGNORE);
    }

    processo->lengthList = processo->elementosPorProcesso;
    //ORDENA A PRIMEIRA SUBLISTA RECEBIDA PELO ROOT, USANDO QUICKSORT SEQUENCIAL
    qsort(processo->list, processo->lengthList, sizeof(int), cmpfunc);

    // GERA-SE O VETOR DE PIVOS QUE SERA USADO EM TODO ALGORITMO
    // O SPLITTER SERÁ A MÉDIA DE TODOS OS SPLITTERS
    splitterLocal = (int*) malloc(sizeof(int) * processo->comm_sz);
    int n = powCast(2,processo->dim);
    for(i = 1; i <= n - 1; i++)
        splitterLocal[i] = (int) (processo->list[(i*processo->elementosPorProcesso)/n]/processo->comm_sz);

    MPI_Allreduce(&splitterLocal[1], &processo->splitter[1], processo->comm_sz - 1, MPI_INT, MPI_SUM, processo->Comm_origem);

    free(splitterLocal);
}

// IDENTIFICA QUAIS PROCESSOS FALHARAM
void getProcessosFalhos(Processo *processo)
{
    int size_old;
    int rank_old;
    int *temp_ranks;
    int *failed_ranks;
    int size_failed;
    int i, j;
    int cont;

    MPI_Group failed_group;
    MPI_Group group_old;

    MPI_Comm_size(processo->Comm,  &size_old );                 /*quantidade de processos original*/
    MPI_Comm_group(processo->Comm, &group_old);                 /* cria um grupo com os processos originais*/
    MPI_Comm_rank(processo->Comm, &rank_old);                   /*rank dos processos originais*/
    MPIX_Comm_failure_ack(processo->Comm);                      /* conhecimento local das falhas*/
    MPIX_Comm_failure_get_acked(processo->Comm, &failed_group); /* grupo com os processos falhos*/
    MPI_Group_size(failed_group, &size_failed);

    /*descobrir processos que falharam */
    temp_ranks = (int*) malloc(sizeof(int) * size_old);
    failed_ranks = (int*) malloc(sizeof(int) * size_old);

    if( (temp_ranks == NULL) || (failed_ranks == NULL))
    {
        printf("%d: Nao foi possivel alocar memoria.\n", processo->my_rank);
        exit(0);
    }

    for(i = 0; i < size_old; i++)
        temp_ranks[i] = i;

    MPI_Group_translate_ranks(failed_group, size_failed, temp_ranks, group_old, failed_ranks);

    if( processo->qtdeProcessosFalhos == 0)
    { /* primeira vez que houve falha. Mapeamento direto */
        for(i = 0; i < size_failed; i++)
        {
            processo->flagProcessos[ failed_ranks[i] ] = INSTAVEL;
        }
    }
    else
    {   /* ranks mudaram. Precisa mapear*/
        /* ordena em ordem crescente o vetor de ranks falhos*/
        qsort(failed_ranks, size_failed, sizeof(int), cmpfunc);

        /* começa pelo maior  */
        for(i = size_failed-1; i >= 0; i--)
        {
            cont = -1;
            j = 0;
            while( (cont < failed_ranks[i]) && (cont < processo->comm_sz)  )
            {
                if (processo->flagProcessos[j] == ESTAVEL)
                {
                    cont++;
                }

                j++;
            }

            processo->flagProcessos[j - 1] = INSTAVEL;
        }
    }

    processo->qtdeProcessosFalhos += size_failed;

    free(temp_ranks);
    free(failed_ranks);
    MPI_Group_free(&failed_group);
    MPI_Group_free(&group_old);
}

// VERIFICA SE OCORREU FALHAS
void detectaFalhos(Processo *processo)
{
    int erro;
    erro = MPI_Barrier(processo->Comm); 
    if( (erro == MPI_ERR_PROC_FAILED) || (erro == MPI_ERR_REVOKED ) )
    {
        int errorAll;
        MPI_Comm temp;
        errorAll = (erro == MPI_SUCCESS);
        MPIX_Comm_agree(processo->Comm, &errorAll);
        if(!errorAll) 
        {
            MPIX_Comm_revoke(processo->Comm);
            
            getProcessosFalhos(processo);

            MPIX_Comm_shrink(processo->Comm, &temp);
            MPI_Comm_free(&processo->Comm);
            processo->Comm = temp;
        }
    }

}

// MATA O PROCESSO CASO SEJA O PROCESSO SORTEADO PARA FALHAR
void killRank(int my_rank, int rank_killed)
{
    if(my_rank == rank_killed)
        kill(getpid(), SIGKILL);
}

// APLICA AS FALHAS SORTEADAS NO INÍCIO DA ORDENAÇÃO
void aplicaFalhas(Processo *processo)
{
    int i;

    switch (processo->cenarioFalha)
    {
        case 0:         //SEM FALHAS
            break;
        case 1:         //1 FALHA
            if(processo->nRodada == *(processo->rodadasFalhas))
               killRank(processo->my_rank, *(processo->processosFalhos));
            break;
        case 2:         // METADE DOS PROCESSOS FALHAM
            for(i = 0; i < processo->comm_sz/2; i++)
            {
                if(processo->nRodada == processo->rodadasFalhas[i])
                    killRank(processo->my_rank, processo->processosFalhos[i]);
            }
            break;
        case 3:        // n -1 PROCESSOS FALHAM, n: NUMERO DE PROCESSOS
            for(i = 0; i < processo->comm_sz - 1; i++)
            {
                if(processo->nRodada == processo->rodadasFalhas[i])
                    killRank(processo->my_rank, processo->processosFalhos[i]);
            }
            break;
    }
}

// RETORNA O PROCESSO QUE ASSUME O PROCESSO FALHO
int getRankAssumeFalho(Processo *processo, int rank, int s)
{
    int j = 0;
    node_set* nodes;       
    int parceiro;

    nodes = cis(rank,s);

    //PROCURA O PRIMEIRO PROCESSO SEM FALHA
    while( (j < nodes->size) && (processo->flagProcessos[ nodes->nodes[j] ] == INSTAVEL) )
            j++;

    if(j == nodes->size)
       parceiro = -1;
    else
        parceiro = nodes->nodes[j];

    set_free(nodes);

    return parceiro;
}

// ADICIONA OS PROCESSOS FALHOS QUE DEVERÃO SER ASSUMIDOS
void adicionaInstaveis(Processo *processo, int rank_instavel)
{
    int i = 0;

    while( (processo->meusInstaveis[i] != -1) && (i < processo->comm_sz) )
        i++;
    
    processo->meusInstaveis[i] = rank_instavel;
}

// CADA PROCESSO ESTÁVEL VERIFICA QUAIS PROCESSOS FALHOS DEVERÁ ASSUMIR
void mapeamento(Processo *processo)
{
    int rankAssumeFalho;
    int i, j;

    for (i = 0; i < processo->comm_sz; i++)
    {
        processo->meusInstaveis[i] = -1;

        if( processo->flagProcessos[i] == INSTAVEL )
        {
            j = 1;

            while ( ( (rankAssumeFalho = getRankAssumeFalho(processo, i, j) ) == -1) && (j <= processo->dim) )
                j++;

            processo->mapeamentoProcessos[i] = rankAssumeFalho;

            if(processo->my_rank == rankAssumeFalho)
                adicionaInstaveis(processo, i);
        }
        else
        {
            processo->mapeamentoProcessos[i] = i;

            if(processo->my_rank == i)
                adicionaInstaveis(processo, i);
        }
    }
}

/* FIM TOLERÂNCIA A FALHAS*/

// ALOCA MEMÓRIA PARA AS LISTAS DE ELEMENTOS
int alocaMemoria(Processo *processo)
{
    int i;
    processo->qtdeElementos = getTamanhoArquivo(processo->sArquivo);
    processo->elementosPorProcesso = (int) ceil(processo->qtdeElementos/processo->comm_sz);

    processo->sizeList = processo->fatorMemoria*processo->elementosPorProcesso;
    processo->sizeVetorTemp = (int) ceil(processo->sizeList);

    processo->list = (int *) malloc(processo->sizeList * sizeof(int));
    processo->vetorTemp = (int *) malloc(processo->sizeVetorTemp * sizeof(int));
    processo->splitter = (int *) malloc(processo->comm_sz * sizeof(int));       
    
    processo->flagProcessos = ( int *) malloc(processo->comm_sz * sizeof( int));
    for(i = 0; i < processo->comm_sz; i++)
        processo->flagProcessos[i] = ESTAVEL;  

    processo->mapeamentoProcessos = ( int *) malloc(processo->comm_sz * sizeof( int));
    for(i = 0; i < processo->comm_sz; i++)
        processo->mapeamentoProcessos[i] = i;

    processo->meusInstaveis = ( int *) malloc(processo->comm_sz * sizeof( int));

    processo->qtdeProcessosFalhos = 0;  
    processo->dim = ( int) (log(processo->comm_sz)/log(2)); 

    return +1;
}

// LIMPA MEMÓRIA ALOCADA PARA O PROCESSO
void deleteProcesso(Processo *processo)
{
    free(processo->list);
    free(processo->vetorTemp);
    free(processo->flagProcessos);
    free(processo->mapeamentoProcessos);
    free(processo->meusInstaveis);
    free(processo->rodadasFalhas);
    free(processo->processosFalhos);
    free(processo);
}

// SALVA EM BINÁRIO OS DADOS RECEBIDOS
void salvarDados(int rank, int *dados, int tam)
{   
    FILE *file;
    char file_name[12];
    sprintf(file_name,"rank_%d.dat", rank );

    remove(file_name);

    if( (file = fopen(file_name, "wb")) == NULL)
    {
        printf("ERRO: Arquivo \"%s\" não pode ser aberto. \n", file_name);
        return;
    }

    fwrite(dados, sizeof(int), tam, file);

    fclose(file);
}

//RETORNA A QUANTIDADE DE INTEIROS DO ARQUIVO
long int getTamanhoArquivo(char *sArquivo)
{
    FILE* fp = fopen(sArquivo, "r"); 
  
    if (fp == NULL) 
        return -1; 
  
    fseek(fp, 0L, SEEK_END); 
  
    long int tam = ftell(fp); 
 
    fclose(fp); 
  
    return tam/sizeof(int); 
}

// RETORNA A LEITURA DE UM ARQUIVO BINÁRIO DO PROCESSO rank
long int lerDadosRank(int rank, int *v)
{   
    FILE *file;
    char file_name[12];
    sprintf(file_name,"rank_%d.dat", rank );

    if( (file = fopen(file_name, "rb")) == NULL)
        return 0;

    fseek(file, 0, SEEK_END); 

    long int tamanho = ftell(file) / sizeof(int);

    fseek(file, 0, SEEK_SET);

    fread(v, sizeof(int), tamanho, file);

    fclose(file);

    return tamanho;
}

// RETORNA A LEITURA DE UM ARQUIVO BINÁRIO
void lerArquivo(Processo *processo, int *vetor)
{
    FILE *file;

    if( (file = fopen(processo->sArquivo, "rb")) == NULL)
    {
        printf("%d: Arquivo \"%s\" não pode ser aberto. \n", processo->my_rank, processo->sArquivo);
        return;
    }

    fread(vetor, sizeof(int), processo->qtdeElementos, file);

    fclose(file);
}

//ESCREVE VETOR RECEBIDO
void imprimeVetor(int *vetor, int tamanho)
{
    int i;

    printf("[");
    for(i = 0; i < tamanho; i++)
        printf("%d, ", vetor[i]);
    printf("]\n");
}

//IMPLEMENTACAO DA FUNCAO POW COM UM CASTING PARA int 
int powCast(int b, int e)
{
    if(e == 0)
        return 1;

    int i;
    int p = b;
    e--;

    for(i = 0; i < e; i++)
        p *= b;
    
    return p;
}

// RETORNA ATRAVÉS DO VETOR v1, O MERGE ENTRE OS VETORES v1 E v2
long int merge(int *v1, int inicio_v1, int fim_v1, int *v2, int inicio_v2, int fim_v2)
{   
    int i, k;
    long int tam1, tam2;

    if(v1 == NULL)
        tam1 = 0;
    else
        tam1 = fim_v1 - inicio_v1;
          
    if(v2 == NULL)
        tam2 = 0;
    else
        tam2 = fim_v2 - inicio_v2;

    
    if(tam1 == 0 && tam2 == 0)
        return 0;
    else if(tam1 > 0 && tam2 == 0)
    {
        // MOVE OS ELEMENTOS DE v1 PARA O INÍCIO
        if(inicio_v1 > 0)
        {
            for(i = inicio_v1, k = 0; i < fim_v1; i++, k++)
                v1[k] = v1[i];
        }
        
        return tam1;
    }
    else if(tam1 == 0 && tam2 > 0)
    {
        // COPIA OS ELEMENTOS DE v2 PARA v1
        for(i = inicio_v2, k = 0; i < fim_v2; i++, k++)
            v1[k] = v2[i];

        return tam2;
    }
    else
    {
        // MOVE OS ELEMENTOS DE v1 PARA O INÍCIO
        if(inicio_v1 > 0)
        {
            for(i = inicio_v1, k = 0; i < fim_v1; i++, k++)
                v1[k] = v1[i];
        }

        // COPIA OS ELEMENTOS DE v2 PARA v1
        for(i = inicio_v2, k = tam1; i < fim_v2; i++, k++)
            v1[k] = v2[i];

        return tam1 + tam2;
    }
}

//RETORNA O TAMANHO DA MENSAGEM ENVIADA AO PROCESSO
int getMensagemLength(Processo *processo)
{
    MPI_Status status;
    int received;          // FLAG QUE INFORMA QUANDO UMA MENSAGEM FOI ENVIADA
    int amount;            // TAMANHO DA MENSAGEM

    do
        MPI_Iprobe(processo->parceiro_rank,  MPI_ANY_TAG, processo->Comm_origem, &received, &status);
    while(!received);

    MPI_Get_count(&status, MPI_INT, &amount);

    return amount;
}

// RETORNA A POSIÇÃO DO PIVO EM UMA LISTA ORDENADA DE ELEMENTOS
int buscaBinaria(int *v, int inicio, int fim, int chave)
{
    int i;

    while (inicio <= fim)
    {
        i = (inicio+fim)/2;

        if( *(v+i) == chave)
            return i;
        else if(*(v+i) < chave)
            return buscaBinaria(v,i+1,fim,chave);
        else
            return buscaBinaria(v,inicio,i-1,chave);
    }

    return inicio;
}

//FUNCAO QUE COMPARA DOIS ELEMENTOS
int cmpfunc (const void * a, const void * b) 
{
   return ( *(int*)a - *(int*)b );
}

// IMPLEMENTAÇÃO DO ALGORITMO QUICKMERGE: VERSÃO PARALELA DO ALGORITMO QUICKSORT
void quickMerge(Processo *processo)
{
    int i, k, count, j;                      // CONTADORES
    int index;                               // INDICE DO VETOR DE PIVOS (SPLITTER)
    int lengthMensagem;                      // TAMANHO DA MENSAGEM RECEBIDA
    int parceiro;                            // RANK DO PARCEIRO SEM MAPEAMENTO 
    int posicaoPivo;                         // POSICAO DO PIVO NA LISTA LOCAL ORDENADA

    int *temp1 = (int *) malloc(processo->sizeList * sizeof(int)); 
    if(temp1 == NULL)
    {
        printf("%d: Memoria insuficiente para temp1", processo->my_rank);
        exit(0);
    }       
    int *temp2 = (int *) malloc(processo->sizeList * sizeof(int));  
    if(temp2 == NULL)
    {
        printf("%d: Memoria insuficiente para temp2", processo->my_rank);
        exit(0);
    }    
    
    // DISTRIBUI ELEMENTOS DO ARQUIVO PARA TODOS OS PROCESSOS
    distribuiElementosProcessosFromFile(processo);

    //CADA PROCESSO SALVA SEU BLOCO DE ELEMENTOS
    salvarDados(processo->my_rank, processo->list, processo->lengthList);
    
    //DETECTA TODOS OS PROCESSOS QUE FALHARAM
    detectaFalhos(processo);

    processo->nRodada = 0;
    /*  
        EXECUTA-SE A ORDENAÇÃO 
        A TROCA DE LISTAS ENTRE OS PROCESSOS OCORRE N VEZES, N = DIMENSAO DO HIPERCUBO 
    */
    for(i = processo->dim-1, k = 0; i >= 0; i--, k++)
    {
        processo->nRodada++;

        aplicaFalhas(processo);

        //DETECTA TODOS OS PROCESSOS QUE FALHARAM
        detectaFalhos(processo);

        if(processo->flagProcessos[processo->my_rank] == ESTAVEL)
        {
            //MAPEIA QUAIS PROCESSOS ESTAVEIS ASSUMEM OS PROCESSOS INSTAVEIS DE ACORDO COM A FUNCAO CIS
            mapeamento(processo);

            count = 0;

            do
            {
                processo->lengthList = lerDadosRank(processo->meusInstaveis[count], processo->list);

                // DEFINE-SE O PARCEIRO ESTAVEL PARA A TROCA DE LISTAS
                parceiro = processo->meusInstaveis[count] ^ powCast(2,i); 
                // MAPEAMENTO PARA O PARCEIRO
                processo->parceiro_rank = processo->mapeamentoProcessos[parceiro];
                
                // DEFINE-SE O INDEX 
                index = (processo->meusInstaveis[count] & (powCast(2,processo->dim)-powCast(2,processo->dim-k))) | powCast(2, processo->dim-k-1);
                processo->pivo = processo->splitter[index]; 

                //SEPARA A LISTA ENTRE NUMEROS MENORES E MAIORES QUE O PIVO
                posicaoPivo = buscaBinaria(processo->list, 0, processo->lengthList-1, processo->pivo);
                processo->lengthLowList = posicaoPivo;
                processo->lengthHighList = processo->lengthList - posicaoPivo;

                /*
                        ENVIA LISTA COM NUMEROS MENORES QUE O PIVO PARA 
                    PARCEIRO COM MENOR RANK
                        ENVIA LISTA COM NUMEROS MAIORES QUE O PIVO PARA
                    PARCEIRO COM MAIOR RANK
                */
                if(processo->my_rank == processo->parceiro_rank)
                {
                    if(processo->meusInstaveis[count] < parceiro)
                    {
                        posicaoPivo = 0;

                        if(processo->lengthHighList > 0)
                        {
                            for(j = 0; j < processo->lengthHighList; j++)
                                temp1[j] = processo->list[j + processo->lengthLowList];   
                        }

                        processo->lengthVetorTemp = lerDadosRank(parceiro, processo->vetorTemp);

                        if(processo->lengthVetorTemp > 0)
                        {
                            posicaoPivo = buscaBinaria(processo->vetorTemp, 0, processo->lengthVetorTemp-1, processo->pivo);

                            if(posicaoPivo > 0)
                            {
                                for(j = 0; j < posicaoPivo; j++)
                                    temp2[j] = processo->vetorTemp[j]; 
                            }
                        }

                        processo->lengthList = merge(processo->list, 0, processo->lengthLowList, temp2, 0, posicaoPivo);
                        processo->lengthVetorTemp = merge(processo->vetorTemp, posicaoPivo, processo->lengthVetorTemp, temp1, 0, processo->lengthHighList);

                        qsort(processo->list, processo->lengthList, sizeof(int), cmpfunc);
                        qsort(processo->vetorTemp, processo->lengthVetorTemp, sizeof(int), cmpfunc);

                        salvarDados(processo->meusInstaveis[count], processo->list, processo->lengthList);
                        salvarDados(parceiro, processo->vetorTemp, processo->lengthVetorTemp);   
                    }
                }
                else if(processo->meusInstaveis[count] > parceiro)
                {
                    MPI_Send(&processo->list[0], processo->lengthLowList, MPI_INT, processo->parceiro_rank, 0, processo->Comm_origem); 
                    
                    lengthMensagem = getMensagemLength(processo);

                    MPI_Recv(&processo->vetorTemp[0], lengthMensagem, MPI_INT, processo->parceiro_rank, 0, processo->Comm_origem, MPI_STATUS_IGNORE);

                    processo->lengthList = merge(processo->list, processo->lengthLowList, processo->lengthList, processo->vetorTemp, 0, lengthMensagem);
                    
                    qsort(processo->list, processo->lengthList, sizeof(int), cmpfunc);
                    salvarDados(processo->meusInstaveis[count], processo->list, processo->lengthList);
                }
                else
                {
                    lengthMensagem = getMensagemLength(processo);

                    MPI_Recv(&processo->vetorTemp[0], lengthMensagem, MPI_INT, processo->parceiro_rank, 0, processo->Comm_origem, MPI_STATUS_IGNORE);

                    MPI_Send(&processo->list[processo->lengthLowList], processo->lengthHighList, MPI_INT, processo->parceiro_rank, 0, processo->Comm_origem); 

                    processo->lengthList = merge(processo->list, 0, processo->lengthLowList, processo->vetorTemp, 0, lengthMensagem);

                    qsort(processo->list, processo->lengthList, sizeof(int), cmpfunc);
                    salvarDados(processo->meusInstaveis[count], processo->list, processo->lengthList);
                }

                count++;
            }
            while( (processo->meusInstaveis[count] != -1) && (  count < processo->comm_sz ) );
        }
    }

    free(temp1);
    free(temp2);
}
