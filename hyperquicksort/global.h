

#define DESCONHECIDO -1     // estado do nodo ainda não foi verificado.
#define ESTAVEL 0           // nodo está estável.
#define INSTAVEL 1          // nodo instável.
#define ZETA 5              // número de rodadas de testes em  que o processo se encontra estável, ou sem-falha. */
#define ALFA 0.9            // constante alfa do algoritmo do TCP
#define BETA 4              // constante beta do algoritmo do TCP
#define BILLION 1000000000  // tempo em milissegundos
#define TIMEOUT_INICIAL  5   // valor do timeout inicial

// Paxos

#define N 10
#define ACK 1
#define NACK 0
#define ZETA 5
#define PREPARE_REQ_RESP 1
#define ACCEPT_REQ_RESP 2
#define DECIDE_REQ_RESP 3
#define TIMEOUT 0.5


/** Estrutura para controle em cada nodo do timeout */
struct timeout_type{
    double inicio;
    double fim;
    double tempo;
    double media;
    double desvio;
    double timeout;
};

typedef struct timeout_type timeout;

/** Variáveis Globais **/

int numFalhos = 0;          /* informa o número de processos falhos */
char nomeArquivo[124];

int *timestamp;        // Vetor de timestamp do nodo
int *zeta;             // vetor com a quantidade de vezes em que o nodo instável foi seguidamente testado como estável
timeout *timeouts;     // vetor de timeouts. Cada nodo mantém o tempo que deve aguardar por uma requisição enviada a outro nodo
int *recvTimestamp;    // vetor de timestamp recebido do nodo testado*/
int *stableNodes;      // estado dos nodos
int *statusMensagens;  // verifica se a mensagem de teste foi recebida

int contResp = 1;      // quantidade de omissões na resposta.
int uLevel;
int nRodadas;          // quantidade de rodadas de testes
int nConsensos = 0;        // quantidade de invocações do algoritmo de consenso



