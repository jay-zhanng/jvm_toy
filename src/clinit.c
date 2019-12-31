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
#include "jvm_init.h"
#include "operand_stack.h"

extern STACK_FRAME* current_frame;
extern int PRINT_LOG;

/**
 * execute the clinit method,set init value for static field and execute static code block
 */
void init_class(CLASS* class_info) {

	METHOD_INFO *clinit_methods_ptr = class_info->clinit_methods_ptr;
	if (clinit_methods_ptr == NULL) {
		return;
	}
	METHOD_INFO clinit_method = *clinit_methods_ptr;

	STACK_FRAME* current_frame_temp = current_frame;
	current_frame = NULL;

	create_stack_frame(clinit_method);

	while (current_frame->pc < current_frame->code_length) {

		unsigned char byte_code =
				(current_frame->byte_codes)[(current_frame->pc)++];
		if (PRINT_LOG)
			printf("pc #%d: 0X%X	", current_frame->pc - 1, byte_code);
		bytecode_dispatch(byte_code);
	}

	current_frame = current_frame_temp;

	if (PRINT_LOG)
		printf("--------------class (%s) init over--------------\n",
				class_info->this_class_name);

}
