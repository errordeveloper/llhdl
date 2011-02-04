#ifndef __TILM_TILM_H
#define __TILM_TILM_H

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

#endif /* __TILM_TILM_H */

