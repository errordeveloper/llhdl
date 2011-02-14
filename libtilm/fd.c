#include <assert.h>
#include <stdlib.h>
#include <gmp.h>

#include <util.h>

#include <llhdl/structure.h>
#include <llhdl/tools.h>

#include <tilm/tilm.h>

enum {
	MAP_FD_SIGNAL,
	MAP_FD_INST
};

struct map_fd_result_signal {
	struct llhdl_node *signal;
	int bit;
};

struct map_fd_result_inst {
	void *inst;
};

struct map_fd_result {
	int type;
	union {
		struct map_fd_result_signal signal;
		struct map_fd_result_inst inst;
	} p;
};

static struct map_fd_result *mkresult_signal(struct llhdl_node *signal, int bit)
{
	struct map_fd_result *r;
	
	r = alloc_type(struct map_fd_result);
	r->type = MAP_FD_SIGNAL;
	r->p.signal.signal = signal;
	r->p.signal.bit = bit;
	
	return r;
}

static struct map_fd_result *mkresult_inst(void *inst)
{
	struct map_fd_result *r;
	
	r = alloc_type(struct map_fd_result);
	r->type = MAP_FD_INST;
	r->p.inst.inst = inst;
	
	return r;
}

struct map_level_param {
	struct tilm_param_fd *p;
	struct tilm_result *result;
};

static struct map_fd_result *map_level(struct map_level_param *mlp, struct llhdl_node *n, int bit)
{
	struct map_fd_result *r;
	mpz_t v;
	void *inst;
	struct map_fd_result *data;
	int len, i;
	
	r = NULL;
	switch(n->type) {
		case LLHDL_NODE_CONSTANT:
			mpz_init_set_ui(v, mpz_tstbit(n->p.constant.value, bit));
			inst = TILM_CALL_CREATE(mlp->p, 1, v);
			mpz_clear(v);
			r = mkresult_inst(inst);
			break;
		case LLHDL_NODE_SIGNAL:
			r = mkresult_signal(n, bit);
			break;
		case LLHDL_NODE_VECT:
			for(i=n->p.vect.nslices-1;i>=0;i--) {
				len = n->p.vect.slices[i].end - n->p.vect.slices[i].start + 1;
				if(bit < len) {
					r = map_level(mlp, n->p.vect.slices[i].source, n->p.vect.slices[i].start + bit);
					break;
				}
				bit -= len;
			}
			break;
		case LLHDL_NODE_FD:
			inst = TILM_CALL_CREATE_FD(mlp->p);
			tilm_result_add_input(mlp->result, n->p.fd.clock, 0, inst, mlp->p->c_pin);
			data = map_level(mlp, n->p.fd.data, bit);
			if(data->type == MAP_FD_SIGNAL)
				tilm_result_add_input(mlp->result, data->p.signal.signal, data->p.signal.bit, inst, mlp->p->d_pin);
			else
				TILM_CALL_CONNECT(mlp->p, data->p.inst.inst, inst, mlp->p->d_pin);
			free(data);
			r = mkresult_inst(inst);
			break;
	}
	
	if(r == NULL) {
		mpz_init(v);
		inst = TILM_CALL_CREATE(mlp->p, 1, v);
		mpz_clear(v);
		r = mkresult_inst(inst);
	}
	
	return r;
}

struct tilm_result *tilm_map_fd(struct tilm_param_fd *p, struct llhdl_node *top)
{
	struct map_level_param mlp;
	int vectorsize;
	int i;
	struct map_fd_result *r;

	vectorsize = llhdl_get_vectorsize(top);

	mlp.p = p;
	mlp.result = tilm_create_result(vectorsize);

	for(i=0;i<vectorsize;i++) {
		r = map_level(&mlp, top, i);
		if(r->type == MAP_FD_SIGNAL) {
			tilm_result_add_merge(mlp.result, i, r->p.signal.signal, r->p.signal.bit);
			mlp.result->out_insts[i] = NULL;
		} else
			mlp.result->out_insts[i] = r->p.inst.inst;
		free(r);
	}

	return mlp.result;
}
