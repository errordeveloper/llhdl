#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <mapkit/mapkit.h>

struct mapkit_sc *mapkit_new(struct llhdl_module *module, mapkit_constant_c constant_c, mapkit_signal_c signal_c, mapkit_join_c join_c, void *user)
{
	struct mapkit_sc *sc;
	
	sc = alloc_type(struct mapkit_sc);
	sc->module = module;
	sc->process_head = NULL;
	sc->constant_c = constant_c;
	sc->signal_c = signal_c;
	sc->join_c = join_c;
	sc->user = user;
	
	return sc;
}

void mapkit_register_process(struct mapkit_sc *sc, mapkit_process_c process_c, mapkit_free_c free_c, void *user)
{
	struct mapkit_process_desc *pd;
	struct mapkit_process_desc *last;
	
	pd = alloc_type(struct mapkit_process_desc);
	pd->process_c = process_c;
	pd->free_c = free_c;
	pd->user = user;
	pd->next = NULL;
	
	if(sc->process_head == NULL)
		sc->process_head = pd;
	else {
		last = sc->process_head;
		while(last->next != NULL)
			last = last->next;
		last->next = pd;
	}
}

static int walk_free_results(struct llhdl_node **n2, void *user)
{
	struct llhdl_node *n = *n2;
	mapkit_free_result(n->user);
	n->user = NULL;
	return 1;
}

void mapkit_free(struct mapkit_sc *sc)
{
	struct mapkit_process_desc *pd1, *pd2;
	
	llhdl_walk_module(walk_free_results, NULL, sc->module);
	pd1 = sc->process_head;
	while(pd1 != NULL) {
		pd2 = pd1->next;
		if(pd1->free_c != NULL)
			pd1->free_c(pd1->user);
		free(pd1);
		pd1 = pd2;
	}
	free(sc);
}

struct mapkit_result *mapkit_create_result(int ninput_nodes, int ninput_nets, int noutput_nets)
{
	struct mapkit_result *r;
	
	r = alloc_type(struct mapkit_result);
	r->ninput_nodes = ninput_nodes;
	r->input_nodes = alloc_size0(ninput_nodes*sizeof(struct llhdl_node **));
	r->input_nets = alloc_size0(ninput_nets*sizeof(void *));
	r->output_nets = alloc_size0(noutput_nets*sizeof(void *));
	
	return r;
}

void mapkit_free_result(struct mapkit_result *r)
{
	if(r == NULL) return;
	free(r->input_nodes);
	free(r->input_nets);
	free(r->output_nets);
	free(r);
}

void mapkit_consume(struct mapkit_sc *sc, struct llhdl_node *n, struct mapkit_result *r)
{
	assert(n->type != LLHDL_NODE_SIGNAL);
	n->user = r;
}

static void run_process(struct llhdl_node **n, struct mapkit_process_desc *pd);

static void run_process_mapped(struct mapkit_result *r, struct mapkit_process_desc *pd)
{
	int i;
	for(i=0;i<r->ninput_nodes;i++)
		run_process(r->input_nodes[i], pd);
}

static void run_process_unmapped(struct llhdl_node *n, struct mapkit_process_desc *pd)
{
	int arity;
	int i;
	
	switch(n->type) {
		case LLHDL_NODE_SIGNAL:
		case LLHDL_NODE_CONSTANT:
			break;
		case LLHDL_NODE_LOGIC:
			arity = llhdl_get_logic_arity(n->p.logic.op);
			for(i=0;i<arity;i++)
				run_process(&n->p.logic.operands[i], pd);
			break;
		case LLHDL_NODE_MUX:
			run_process(&n->p.mux.select, pd);
			for(i=0;i<n->p.mux.nsources;i++)
				run_process(&n->p.mux.sources[i], pd);
			break;
		case LLHDL_NODE_FD:
			run_process(&n->p.fd.clock, pd);
			run_process(&n->p.fd.data, pd);
			break;
		case LLHDL_NODE_VECT:
			for(i=0;i<n->p.vect.nslices;i++)
				run_process(&n->p.vect.slices[i].source, pd);
			break;
		case LLHDL_NODE_ARITH:
			run_process(&n->p.arith.a, pd);
			run_process(&n->p.arith.b, pd);
			break;
		default:
			assert(0);
			break;
	}
}

static void run_process(struct llhdl_node **n, struct mapkit_process_desc *pd)
{
	if((*n)->user != NULL)
		/* Node is already mapped, carry on to the cut line */
		run_process_mapped((*n)->user, pd);
	else {
		/* Attempt mapping */
		pd->process_c(n, pd->user);
		/* Was mapping successful? */
		if((*n)->user != NULL)
			/* Yes, continue to the cut line */
			run_process_mapped((*n)->user, pd);
		else
			/* No, try mapping the children */
			run_process_unmapped(*n, pd);
	}
}

void mapkit_metamap(struct mapkit_sc *sc)
{
	struct llhdl_node *n;
	struct mapkit_process_desc *pd;
	
	n = sc->module->head;
	while(n != NULL) {
		assert(n->type == LLHDL_NODE_SIGNAL);
		
		/* Run mapper processes */
		pd = sc->process_head;
		while(pd != NULL) {
			run_process(&n->p.signal.source, pd);
			pd = pd->next;
		}
		
		/* Establish all connections */
		mapkit_interconnect_arc(sc, n);
		
		n = n->p.signal.next;
	}
}

