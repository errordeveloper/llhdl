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

/* Connect the output of LUT <a> to the input <n> of LUT <b>
 *  <a> and <b> are opaque handles previously returned by a tilm_create_c callback
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

#define TILM_CALL_CREATE(p, inputs, contents) ((p)->create(inputs, contents, (p)->user))
#define TILM_CALL_CONNECT(p, a, b, n) ((p)->connect(a, b, n, (p)->user))

struct tilm_input_assoc {
	struct llhdl_node *signal;
	int bit;
	void *lut;
	int n;
};

struct tilm_result {
	void **out_luts;
	int vectorsize;
	int n_input_assoc;
	struct tilm_input_assoc input_assoc[];
};

struct tilm_result *tilm_map(struct tilm_param *p, struct llhdl_node *top);
void tilm_free_result(struct tilm_result *r);

#endif /* __TILM_TILM_H */

