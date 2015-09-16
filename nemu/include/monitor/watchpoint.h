#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	char expr[51];
	uint32_t expr_value;
	/* TODO: Add more members if necessary */


} WP;
void add_wp(char* args);
void delete_wp(char*args);
void print_wp();
#endif
