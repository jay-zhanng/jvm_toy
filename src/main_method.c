#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "struct.h"
#include "util.h"
#include "class_parser.h"
#include "class_loader.h"
#include "class_preparer.h"
#include "clinit.h"
#include "bytecode_parser.h"
#include "jvm_init.h"

extern STACK_FRAME* current_frame;
extern char* MAIN_CLASS;
void main_method() {

	CLASS *main_class = search_class_info(MAIN_CLASS);

	METHOD_INFO method = *main_class->main_methods_ptr;

	create_stack_frame(method);

	printf(
			"--------------start execute main method byte code \n--------------");
	unsigned int* pc_ptr = &(current_frame->pc);
	while (*pc_ptr < current_frame->code_length) {
		unsigned char byte_code = (current_frame->byte_codes)[(*pc_ptr)++];
		printf("pc #%d: 0X%X ", *pc_ptr - 1, byte_code);
		dispatch_bytecode(byte_code);
	}

	rel_stack_area_memo(current_frame->frame_size);
}
