#ifndef __PARTITION_H
#define __PARTITION_H

#include <llhdl/structure.h>
#include <mapkit/mapkit.h>

/*
 * If the node is not the parent of a partition that should be mapped
 * to LUTs (types CONSTANT, LOGIC, MUX and VECT), NULL is returned.
 * Otherwise, the returned mapkit_result has the following filed in:
 *   <ninput_nodes> is number of nodes at the partition boundary
 *   <input_nodes> point to the nodes at the partition boundary
 *   <input_nets> contains empty nets
 *   <output_nets> contains empty nets
 */
struct mapkit_result *tilm_try_partition(struct tilm_sc *sc, struct llhdl_node **n);

#endif /* __PARTITION_H */
