#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <tilm/tilm.h>

#include "internal.h"
#include "variables.h"

static void add_single_variable(struct tilm_variable **head, struct llhdl_node *n, int bit)
{
	struct tilm_variable *v;
	
	v = *head;
	while(v != NULL) {
		if((v->n == n) && (v->bit == bit))
			return;
		v = v->next;
	}
	
	v = alloc_type(struct tilm_variable);
	v->n = n;
	v->bit = bit;
	v->value = 0;
	v->next = *head;
	*head = v;
}

static void add_variable(struct tilm_variable **head, struct llhdl_node *n, int bit)
{
	int i, count;
	
	count = llhdl_get_vectorsize(n);
	if(bit == -1) {
		for(i=0;i<count;i++)
			add_single_variable(head, n, i);
	} else {
		if(bit < count)
			add_single_variable(head, n, bit);
		else if(llhdl_get_sign(n))
			add_single_variable(head, n, count-1);
	}
}

static void enumerate_bit(struct tilm_variable **head, struct llhdl_node *n, int bit)
{
	int i, arity;
	int j, len;
	
	if(n->user != NULL) {
		/* Already mapped */
		add_variable(head, n, bit);
		return;
	}
	
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			/* nothing to do */
			break;
		case LLHDL_NODE_LOGIC:
			arity = llhdl_get_logic_arity(n->p.logic.op);
			for(i=0;i<arity;i++)
				enumerate_bit(head, n->p.logic.operands[i], bit);
			break;
		case LLHDL_NODE_MUX:
			enumerate_bit(head, n->p.mux.select, -1);
			for(i=0;i<n->p.mux.nsources;i++)
				enumerate_bit(head, n->p.mux.sources[i], bit);
			break;
		case LLHDL_NODE_VECT:
			if(bit == -1) {
				for(i=0;i<n->p.vect.nslices;i++)
					for(j=n->p.vect.slices[i].start;j<=n->p.vect.slices[i].end;j++)
						enumerate_bit(head, n->p.vect.slices[i].source, j);
			} else {
				for(i=0;i<n->p.vect.nslices;i++) {
					len = n->p.vect.slices[i].end - n->p.vect.slices[i].start + 1;
					if(bit < len) {
						enumerate_bit(head, n->p.vect.slices[i].source, n->p.vect.slices[i].start+bit);
						break;
					}
					bit -= len;
				}
			}
			break;
		default:
			add_variable(head, n, bit);
			break;
	}
}

struct tilm_variables *tilm_variables_enumerate(struct llhdl_node *n)
{
	struct tilm_variables *r;
	int i;

	r = alloc_type(struct tilm_variables);
	r->vectorsize = llhdl_get_vectorsize(n);
	r->heads = alloc_size0(r->vectorsize*sizeof(struct tilm_variable **));
	
	for(i=0;i<r->vectorsize;i++)
		enumerate_bit(&r->heads[i], n, i);
	
	return r;
}

void tilm_variables_dump(struct tilm_variables *r)
{
	int i;
	struct tilm_variable *v;
	
	for(i=0;i<r->vectorsize;i++) {
		printf("Bit %d:\n", i);
		v = r->heads[i];
		while(v != NULL) {
			printf("  %s@%p:%d", llhdl_strtype(v->n->type), v->n, v->bit);
			if(v->n->type == LLHDL_NODE_SIGNAL)
				printf(" (%s)", v->n->p.signal.name);
			printf("\n");
			v = v->next;
		}
	}
}

int tilm_variables_remaining(struct tilm_variable *v)
{
	int r;
	
	r = 0;
	while(v != NULL) {
		r++;
		v = v->next;
	}
	return r;
}

int tilm_variable_value(struct tilm_variables *r, int obit, struct llhdl_node *n, int bit)
{
	struct tilm_variable *v;
	int vectorsize;
	
	vectorsize = llhdl_get_vectorsize(n);
	if(bit >= vectorsize) {
		if(llhdl_get_sign(n))
			bit = vectorsize-1;	/* sign extend */
		else
			return 0;		/* fill with 0 */
	}
	
	v = r->heads[obit];
	while(v != NULL) {
		if((v->n == n) && (v->bit == bit))
			return v->value;
		v = v->next;
	}
	return 0;
}

void tilm_variables_free(struct tilm_variables *r)
{
	int i;
	struct tilm_variable *v1, *v2;
	
	for(i=0;i<r->vectorsize;i++) {
		v1 = r->heads[i];
		while(v1 != NULL) {
			v2 = v1->next;
			free(v1);
			v1 = v2;
		}
	}
	free(r->heads);
	free(r);
}

