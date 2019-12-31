#include <stdlib.h>
#include <stdio.h>
#include "struct.h"

CLASS* CLASS_INFOS; //the start address of all CLASS in method area
unsigned int CLASS_INDEX = 0;

INSTANCE* INSTANCES; //the start address of all INSTANCE in heap
unsigned int INST_INDEX = 0;

int* STR_CONS_POOL; //the start address of String constant pool
unsigned int STR_CONS_INDEX = 0;

extern int PRINT_LOG;

void* method_area;
unsigned int method_area_offset = 0;
unsigned int method_area_init_size = 10 * 1024 * 1024;
void method_area_init() {
	//max load 100 class
	CLASS_INFOS = malloc(sizeof(CLASS) * 100);
	method_area = malloc(method_area_init_size);
}

void *req_method_area_memo(unsigned int size) {

	if (method_area_offset + size > method_area_init_size) {
		if (PRINT_LOG)
			printf("method area overflow!\n");
		return NULL;
	} else {
		unsigned char *ptr = method_area;
		ptr += method_area_offset;
		method_area_offset += size;
		return ptr;
	}
}

void* heap_area;
unsigned int heap_area_offset = 0;
unsigned int heap_area_init_size = 10 * 1024 * 1024;
void heap_init() {
	//max new 100 object
	INSTANCES = malloc(sizeof(INSTANCE) * 100);
	heap_area = malloc(heap_area_init_size);
}

void* req_heap_area_memo(unsigned int size) {
	if (heap_area_offset + size > heap_area_init_size) {
		if (PRINT_LOG)
			printf("method area overflow!\n");
		return NULL;
	} else {
		unsigned char *ptr = heap_area;
		ptr += heap_area_offset;
		heap_area_offset += size;
		return ptr;
	}
}

void* stack_area;
unsigned int stack_area_offset = 0;
unsigned int stack_area_init_size = 100 * 1024 * 1024;
void stack_init() {
	stack_area = malloc(stack_area_init_size);
}

void *req_stack_area_memo(unsigned int size) {

	if (stack_area_offset + size > stack_area_init_size) {
		if (PRINT_LOG)
			printf("method area overflow!\n");
		return NULL;
	} else {
		unsigned char *ptr = stack_area;
		ptr += stack_area_offset;
		stack_area_offset += size;

		return ptr;
	}
}

void rel_stack_area_memo(unsigned int size) {
	stack_area_offset -= size;
}

void str_cons_pool_init() {
	//max store 100 String in pool
	STR_CONS_POOL = malloc(sizeof(int) * 100);
}

/*
 * allocate memory for method_area,heap,stack
 * all areas have fixed memory size.
 * max load 10 class in method area and create 10 object in heap
 */
void init() {
	method_area_init();
	heap_init();
	stack_init();
	str_cons_pool_init();
}
