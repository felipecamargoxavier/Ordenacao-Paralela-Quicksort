#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#include "QuickMerge.h"

#define ROOT 0

/*
	FELIPE XAVIER  05/07/2018
	IMPLEMENTAÇÃO DO ALGORITMO QUICKMERGE
*/
int main(int argc, char *argv[])
{
	Processo *processo = malloc(sizeof(Processo)); 

	if(argc != 3)
	{
		printf("-> ERRO: Informe o nome do arquivo (argv[1]) e o cenario de falhas (argv[2]).\n");
		return -1;
	}
	else
	{
		processo->sArquivo = argv[1];
		processo->cenarioFalha = atoi(argv[2]);
	}

	MPI_Init(NULL, NULL);				

	MPI_Comm_size(MPI_COMM_WORLD, &processo->comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &processo->my_rank);

	MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

	//processo->Comm = MPI_COMM_WORLD;
	MPI_Comm_dup(MPI_COMM_WORLD, &processo->Comm);
    MPI_Comm_dup(MPI_COMM_WORLD, &processo->Comm_origem);
	processo->root_rank = ROOT;
	processo->fatorMemoria = processo->comm_sz;

	alocaMemoria(processo);
	insereFalhas(processo);

	quickMerge(processo);

	MPI_Finalize();

    deleteProcesso(processo);

	return 0;
}