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
#include "jvm_init.h"
#include "operand_stack.h"

extern STACK_FRAME* current_frame;

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
		printf("pc #%d: 0X%X	", current_frame->pc - 1, byte_code);
		dispatch_bytecode(byte_code);
	}

	current_frame = current_frame_temp;

	printf("--------------class (%s) init over--------------\n",
			class_info->this_class_full_name);

}
