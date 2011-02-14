#ifndef __TILM_TILM_H
#define __TILM_TILM_H

#include <gmp.h>

#include <llhdl/structure.h>

enum {
	TILM_SHANNON = 0,
	TILM_BDSPGA,
	TILM_COUNT /* must be last */
};

#define TILM_DEFAULT TILM_SHANNON

struct tilm_desc {
	const char *handle;
	const char *description;
};

extern struct tilm_desc tilm_mappers[];

int tilm_get_mapper_by_handle(const char *handle);

/* Create a LUT and returns a opaque handle
 *  <inputs> Number of inputs
 *  <contents> Bitmap of the LUT contents
 *  <user> User pointer from the tilm_param structure
 */
typedef void * (*tilm_create_c)(int inputs, mpz_t contents, void *user);

/* Create a D flip flop and returns a opaque handle */
typedef void * (*tilm_create_fd_c)(void *user);

/* Connect the output of LUT/FD <a> to the input <n> of LUT/FD <b>
 *  <a> and <b> are opaque handles previously returned by a tilm_create*_c callback
 *  <user> is the user pointer from the tilm_param structure
 */
typedef void (*tilm_connect_c)(void *a, void *b, int n, void *user);

struct tilm_param {
	int mapper_id;
	int max_inputs;
	void *extra_mapper_param;
	tilm_create_c create;
	tilm_connect_c connect;
	void *user;
};

struct tilm_param_fd {
	tilm_create_c create;
	tilm_create_fd_c create_fd;
	tilm_connect_c connect;
	int d_pin, c_pin;
	void *user;
};

#define TILM_CALL_CREATE(p, inputs, contents) ((p)->create(inputs, contents, (p)->user))
#define TILM_CALL_CREATE_FD(p) ((p)->create_fd((p)->user))
#define TILM_CALL_CONNECT(p, a, b, n) ((p)->connect(a, b, n, (p)->user))

struct tilm_input_assoc {
	struct llhdl_node *signal;
	int bit;
	void *inst;
	int n;
	struct tilm_input_assoc *next;
};

struct tilm_merge {
	struct llhdl_node *signal;
	int bit;
};

struct tilm_result {
	int vectorsize;
	void **out_insts;
	struct tilm_merge *merges;
	struct tilm_input_assoc *ihead;
};

struct tilm_result *tilm_map(struct tilm_param *p, struct llhdl_node *top);
struct tilm_result *tilm_map_fd(struct tilm_param_fd *p, struct llhdl_node *top);

struct tilm_result *tilm_create_result(int vectorsize);
void tilm_result_add_input(struct tilm_result *r, struct llhdl_node *signal, int bit, void *inst, int n);
void tilm_result_add_merge(struct tilm_result *r, int i, struct llhdl_node *signal, int bit);
void tilm_free_result(struct tilm_result *r);

#endif /* __TILM_TILM_H */

