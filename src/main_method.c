#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "bytecode_interpreter.h"
#include "struct.h"
#include "util.h"
#include "class_parser.h"
#include "class_loader.h"
#include "class_preparer.h"
#include "clinit.h"
#include "jvm_init.h"

extern STACK_FRAME* current_frame;
extern int PRINT_LOG;

/**
 * start execute main method byte code
 */
void start(CLASS* main_class_info) {

	METHOD_INFO method = *main_class_info->main_methods_ptr;

	create_stack_frame(method);

	if (PRINT_LOG)
		printf(
				"--------------start to execute main method byte code--------------\n");
	unsigned int* pc_ptr = &(current_frame->pc);
	while (*pc_ptr < current_frame->code_length) {
		unsigned char byte_code = (current_frame->byte_codes)[(*pc_ptr)++];
		if (PRINT_LOG)
			printf("pc #%d: 0X%X ", *pc_ptr - 1, byte_code);
		bytecode_dispatch(byte_code);
	}

	rel_stack_area_memo(current_frame->frame_size);
}
