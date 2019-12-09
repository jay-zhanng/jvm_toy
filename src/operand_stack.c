#include "jvm_init.h"
#include "struct.h"

extern STACK_FRAME* current_frame;

void curr_push(int item) {
	*(current_frame->operand_stack_erea) = item;
	(current_frame->operand_stack_erea)++;
}

int curr_pop() {
	(current_frame->operand_stack_erea)--;
	return *(current_frame->operand_stack_erea);
}

void last_push(int item) {

	*(((STACK_FRAME*) current_frame->last_frame)->operand_stack_erea) =
			item;

	(((STACK_FRAME*) current_frame->last_frame)->operand_stack_erea)++;
}

int last_pop() {

	(((STACK_FRAME*) current_frame->last_frame)->operand_stack_erea)--;
	return *(((STACK_FRAME*) current_frame->last_frame)->operand_stack_erea);
}
