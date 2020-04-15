/**** CIS ***/

#define POW_2(num) (1<<(num))
#define VALID_J(j, s) ((POW_2(s-1)) >= j)

/* |-- node_set.h  da cisj*/
typedef struct node_set {
	int* nodes;
	ssize_t size;
	ssize_t offset;
} node_set;


/************* CABEÇALHO DAS FUNÇÕES DA CIS ***************/

static node_set* set_new(ssize_t size);
static void set_insert(node_set* nodes, int node);
static void set_merge(node_set* dest, node_set* source);
static void set_free(node_set* nodes);
node_set* cis(int i, int s);


/************* IMPLEMENTAÇÃO DAS FUNÇÕES DA CIS ***************/

static node_set* set_new(ssize_t size)
{
	node_set* node_new;

	node_new = (node_set*)malloc(sizeof(node_set));
	node_new->nodes = (int*)malloc(sizeof(int)*size);
	node_new->size = size;
	node_new->offset = 0;
	return node_new;
}

static void set_insert(node_set* nodes, int node)
{
	if (nodes == NULL) return;
	nodes->nodes[nodes->offset++] = node;
}

static void set_merge(node_set* dest, node_set* source)
{
	if (dest == NULL || source == NULL) return;
	memcpy(&(dest->nodes[dest->offset]), source->nodes, sizeof(int)*source->size);
	dest->offset += source->size;
}

static void set_free(node_set* nodes)
{
	free(nodes->nodes);
	free(nodes);
}
/* node_set.h --| */

node_set* cis(int i, int s)
{
	node_set* nodes, *other_nodes;
	int _xor = i ^ POW_2(s-1);
	int j;

	/* starting node list */
	nodes = set_new(POW_2(s-1));

	/* inserting the first value (i XOR 2^^(s-1)) */
	set_insert(nodes, _xor);

	/* recursion */
	for (j=1; j<=s-1; j++) {
		other_nodes = cis(_xor, j);
		set_merge(nodes, other_nodes);
		set_free(other_nodes);
	}
	return nodes;
}
/*******************  FIM DAS FUNÇÕES DA CIS *******************/

