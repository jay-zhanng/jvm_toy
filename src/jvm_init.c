#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "struct.h"
#include "util.h"
#include "class_parser.h"
#include "class_loader.h"
#include "class_preparer.h"
#include "bytecode_parser.h"
#include "operand_stack.h"

CLASS* CLASS_INFOS;
INSTANCE* INSTANCES;
unsigned int CLASS_INFOS_INDEX = 0;
unsigned char* method_area;
unsigned int method_area_offset = 0;
unsigned int method_area_init_size = 100 * 1024 * 1024;

void method_area_init() {
	//max load 10 class
	CLASS_INFOS = malloc(sizeof(CLASS) * 10);
	method_area = malloc(method_area_init_size);
}

void *req_method_area_memo(unsigned int size) {

	if (method_area_offset + size > method_area_init_size) {
		printf("method area overflow!\n");
		return NULL;
	} else {
		unsigned char *ptr = method_area;
		ptr += method_area_offset;
		method_area_offset += size;
		return ptr;
	}
}

unsigned char* heap_area;
unsigned int heap_area_offset = 0;
unsigned int heap_area_init_size = 10 * 1024 * 1024;

void heap_init() {
	//max new 10 class
	INSTANCES = malloc(sizeof(INSTANCE) * 10);
	heap_area = malloc(heap_area_init_size);
}

void* req_heap_area_memo(unsigned int size) {
	if (heap_area_offset + size > heap_area_init_size) {
		printf("method area overflow!\n");
		return NULL;
	} else {
		unsigned char *ptr = heap_area;
		ptr += heap_area_offset;
		heap_area_offset += size;
		return ptr;
	}
}

unsigned char* stack_area;
unsigned int stack_area_offset = 0;
unsigned int stack_area_init_size = 10 * 1024 * 1024;
STACK_FRAME* current_frame = NULL;

void stack_init() {
	stack_area = malloc(stack_area_init_size);
}

void *req_stack_area_memo(unsigned int size) {

	if (stack_area_offset + size > stack_area_init_size) {
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

void init() {
	method_area_init();
	heap_init();
	stack_init();
}
