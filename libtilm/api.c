#include <stdlib.h>
#include <string.h>

#include <tilm/tilm.h>

struct tilm_desc tilm_mappers[] = {
	{
		.handle = "shannon",
		.description = "Naive Shannon decomposition based"
	},
	{
		.handle = "bdspga",
		.description = "BDS-PGA"
	}
};

int tilm_get_mapper_by_handle(const char *handle)
{
	int i;
	
	for(i=0;i<TILM_COUNT;i++) {
		if(strcmp(tilm_mappers[i].handle, handle) == 0)
			return i;
	}
	return -1;
}
