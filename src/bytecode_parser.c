#include <stdio.h>
#include <stdlib.h>
#include "struct.h"
#include "util.h"
#include "class_parser.h"
#include "jvm_init.h"
#include "class_loader.h"
#include "operand_stack.h"

extern STACK_FRAME* current_frame;
extern unsigned char* heap_area;
extern unsigned int INST_NUM;
extern INSTANCE* INSTANCES;

void new() {

	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned short index = byte_codes[current_frame->pc++] << 8
			| byte_codes[current_frame->pc++];
	printf("new #%d\n", index);

	CONSTANT_Class_Info * cp_ptr =
			((CONSTANT_Class_Info **) current_class_info->cps)[index];

	char* class_name = cp_ptr->class_name;
	printf("new class name: %s\n", class_name);

	//load and init this class
	CLASS* this_class_info = load_class(class_name);

	//get inst filed num
	unsigned short total_fields_num = get_inst_filed_total_num(this_class_info);
	this_class_info->total_fields_count = total_fields_num;

	//allocate memory
	INSTANCE* inst_ptr = req_inst();
	INST_FIELD_VALUE* fields_ptr = (INST_FIELD_VALUE*) req_heap_area_memo(
			total_fields_num * sizeof(INST_FIELD_VALUE));

	inst_ptr->class_info = this_class_info;
	inst_ptr->inst_field_value = fields_ptr;

	//get ref offset
	curr_push(INST_NUM - 1);

	//set default value for inst field
	init_inst_fields(this_class_info, total_fields_num, fields_ptr);
}

void putfield() {

	CLASS* class_info = current_frame->class_info;
	unsigned char* byte_codes = current_frame->byte_codes;
	unsigned short index = (byte_codes[current_frame->pc++] << 8)
			| byte_codes[current_frame->pc++];

	printf("putfield #%d\n", index);

	CONSTANT_Fieldref_Info* cp_ptr =
			((CONSTANT_Fieldref_Info**) class_info->cps)[index];
//	char* class_name = cp_ptr->class_name;
	char* filed_name = cp_ptr->filed_name;
	char* filed_descriptor = cp_ptr->filed_descriptor;

	unsigned int value = curr_pop();
	int refer_index = curr_pop();

	printf("put refer_index %d\n", refer_index);
	printf("put inst field value %d\n", value);

	INSTANCE inst = INSTANCES[refer_index];
	INST_FIELD_VALUE* INST_FIELD_PTR = inst.inst_field_value;

	INST_FIELD_VALUE field = research_inst_field_value(inst.class_info,
			INST_FIELD_PTR, filed_name, filed_descriptor);

	*field.field_value_ptr = value;
}

void getfield() {
	CLASS* class_info = current_frame->class_info;
	unsigned char* byte_codes = current_frame->byte_codes;
	unsigned short index = (byte_codes[current_frame->pc++] << 8)
			| byte_codes[current_frame->pc++];

	printf("getfield #%d\n", index);

	CONSTANT_Fieldref_Info* cp_ptr =
			((CONSTANT_Fieldref_Info**) class_info->cps)[index];
//	char* class_name = cp_ptr->class_name;
	char* filed_name = cp_ptr->filed_name;
	char* filed_descriptor = cp_ptr->filed_descriptor;

	int refer_index = curr_pop();
	printf("get refer_index %d\n", refer_index);
	INSTANCE inst = INSTANCES[refer_index];
	INST_FIELD_VALUE* INST_FIELD_PTR = inst.inst_field_value;

	INST_FIELD_VALUE field = research_inst_field_value(inst.class_info,
			INST_FIELD_PTR, filed_name, filed_descriptor);
	unsigned int value = *field.field_value_ptr;
	curr_push(value);
	printf("get inst field value %d\n", value);
}

void bipush() {
	CLASS* class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	char item = byte_codes[current_frame->pc++];
	printf("bipush %d\n", item);
	curr_push(byte_int(item));
}

void sipush() {
	CLASS* class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	short item = byte_codes[current_frame->pc++] << 8
			| byte_codes[current_frame->pc++];
	printf("sipush %d\n", item);
	curr_push(short_int(item));
}

void putstatic() {
	CLASS* class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned short index = byte_codes[current_frame->pc++] << 8
			| byte_codes[current_frame->pc++];

	printf("putstatic #%d\n", index);

	CONSTANT_Fieldref_Info * cp_ptr =
			((CONSTANT_Fieldref_Info **) class_info->cps)[index];

	char* class_name = cp_ptr->class_name;
	char* filed_name = cp_ptr->filed_name;
	char* filed_descriptor = cp_ptr->filed_descriptor;

	CLASS* tar_class_info = search_class_info(class_name);
	STATIC_FIELD_VALUE* field_ptr = research_static_field_info(tar_class_info,
			filed_name, filed_descriptor);

	if (str_cmp(filed_descriptor, "D") || str_cmp(filed_descriptor, "J")
			|| str_cmp(filed_descriptor, "L")) {
		(field_ptr->field_value_ptr)[0] = curr_pop();
		(field_ptr->field_value_ptr)[1] = curr_pop();

	} else {
		*(field_ptr->field_value_ptr) = curr_pop();
	}
}

void getstatic() {
	CLASS* class_info = current_frame->class_info;
	unsigned char* byte_codes = current_frame->byte_codes;
	unsigned short index = (byte_codes[current_frame->pc++] << 8)
			| byte_codes[current_frame->pc++];

	printf("getstatic #%d\n", index);

	CONSTANT_Fieldref_Info* cp_ptr =
			((CONSTANT_Fieldref_Info**) class_info->cps)[index];
	char* class_name = cp_ptr->class_name;
	char* filed_name = cp_ptr->filed_name;
	char* filed_descriptor = cp_ptr->filed_descriptor;

	CLASS* tar_class_info = search_class_info(class_name);
	STATIC_FIELD_VALUE* field = research_static_field_info(tar_class_info,
			filed_name, filed_descriptor);

	if (str_cmp(filed_descriptor, "D") || str_cmp(filed_descriptor, "J")
			|| str_cmp(filed_descriptor, "L")) {
		curr_push(field->field_value_ptr[0]);
		curr_push(field->field_value_ptr[1]);
	} else {
		curr_push(*field->field_value_ptr);
	}
}

// static invoke
void invokespecial() {
	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;

	unsigned short index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	printf("invokespecial #%d\n", index);

	CONSTANT_Methodref_Info * current_cp_ptr =
			((CONSTANT_Methodref_Info **) current_class_info->cps)[index];

	char* class_name = current_cp_ptr->class_name;
	char* name = current_cp_ptr->method_name;
	printf("Inst object name:%s, Inst method name:%s\n", class_name, name);

	char* descriptor = current_cp_ptr->method_descriptor;

	CLASS* tar_class_info = search_class_info(class_name);
	//find method info
	METHOD_INFO* method;
	if (!current_cp_ptr->if_reslution) {

		method = research_method_info(tar_class_info, name, descriptor);
		if (method == NULL) {
			printf("do not find the method: %s\n", name);
			return;
		}
		current_cp_ptr->method = method;
		current_cp_ptr->if_reslution = 1;
	} else {
		method = current_cp_ptr->method;
	}

	create_stack_frame(*method);
	// pass parameter,include this param
	unsigned int param_num = prase_method_desc(descriptor) + 1;
	while (param_num--) {
		int value = last_pop();
		printf("param %d: %d\n", param_num, value);
		(current_frame->local_variable_erea)[param_num] = value;
	}

	while (current_frame->pc < current_frame->code_length) {
		unsigned char byte_code =
				(current_frame->byte_codes)[(current_frame->pc)++];
		printf("pc #%d: 0X%X	", current_frame->pc - 1, byte_code);
		dispatch_bytecode(byte_code, current_class_info);
	}
}

void invokevirtual() {
	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;

	unsigned short index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	printf("invokevirtual #%d\n", index);

	CONSTANT_Methodref_Info * current_cp_ptr =
			((CONSTANT_Methodref_Info **) current_class_info->cps)[index];

	char* class_name = current_cp_ptr->class_name;
	char* name = current_cp_ptr->method_name;
	printf("Inst object name:%s, Inst method name:%s\n", class_name, name);

	char* descriptor = current_cp_ptr->method_descriptor;
	//include this param
	unsigned int param_num = prase_method_desc(descriptor) + 1;
	int params_temp[param_num];
	int param_num_temp = param_num;
	while (param_num_temp--) {
		int value = curr_pop();
		printf("param %d: %d\n", param_num_temp, value);
		params_temp[param_num_temp] = value;
	}

	//find method info
	METHOD_INFO* method;
	CLASS* this_class_info;
	if (!current_cp_ptr->if_reslution) {

		int refer_index = params_temp[0];
		printf("get refer_index %d\n", refer_index);

		INSTANCE* inst_ptr = INSTANCES + refer_index;
		this_class_info = inst_ptr->class_info;

		//	char* name = cp_ptr->class_name;
		method = research_method_info(this_class_info, name, descriptor);
		if (method == NULL) {
			printf("do not find the method: %s\n", name);
			return;
		}
		current_cp_ptr->method = method;
		current_cp_ptr->if_reslution = 1;
	} else {
		method = current_cp_ptr->method;
	}

	create_stack_frame(*method);
	param_num_temp = param_num;
	while (param_num_temp--) {
		(current_frame->local_variable_erea)[param_num_temp] =
				params_temp[param_num_temp];
	}

	while (current_frame->pc < current_frame->code_length) {
		unsigned char byte_code =
				(current_frame->byte_codes)[(current_frame->pc)++];
		printf("pc #%d: 0X%X	", current_frame->pc - 1, byte_code);
		dispatch_bytecode(byte_code, current_class_info);
	}
}

void invokestatic() {

	CLASS* class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned short index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	printf("invokestatic #%d\n", index);

	CONSTANT_Methodref_Info * cp_ptr =
			((CONSTANT_Methodref_Info **) class_info->cps)[index];

	char* name = cp_ptr->method_name;
	char* descriptor = cp_ptr->method_descriptor;
	METHOD_INFO* method;
	if (!cp_ptr->if_reslution) {
		//	char* name = cp_ptr->class_name;
		method = research_method_info(class_info, name, descriptor);
		if (method == NULL) {
			printf("do not find the method: %s\n", name);
			return;
		}

		cp_ptr->method = method;
		cp_ptr->if_reslution = 1;
	} else {
		method = cp_ptr->method;
	}

	unsigned short access_flags = method->access_flags;
	if (access_flags & 0x0100) {
		printf("native method name: %s\n", method->name);
		printf("print final value: %d\n", curr_pop());

	} else {
		create_stack_frame(*method);

		unsigned int param_num = prase_method_desc(descriptor);
		while (param_num--) {
			int value = last_pop();
			printf("param:%d\n", value);
			(current_frame->local_variable_erea)[param_num] = value;
		}

		while (current_frame->pc < current_frame->code_length) {
			unsigned char byte_code =
					(current_frame->byte_codes)[(current_frame->pc)++];
			printf("pc #%d: 0X%X	", current_frame->pc, byte_code);
			dispatch_bytecode(byte_code, class_info);
		}
	}
}

void invokeinterface() {

	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;

	unsigned short index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	printf("invokeinterface #%d\n", index);

	unsigned char count = byte_codes[(current_frame->pc)++];
	unsigned char zero = byte_codes[(current_frame->pc)++];

	CONSTANT_Methodref_Info * current_cp_ptr =
			((CONSTANT_Methodref_Info **) current_class_info->cps)[index];

	char* class_name = current_cp_ptr->class_name;
	char* name = current_cp_ptr->method_name;
	printf("Inst object name:%s, Inst method name:%s\n", class_name, name);

	char* descriptor = current_cp_ptr->method_descriptor;
	//include this param
	unsigned int param_num = prase_method_desc(descriptor) + 1;
	int params_temp[param_num];
	int param_num_temp = param_num;
	while (param_num_temp--) {
		int value = curr_pop();
		printf("param %d: %d\n", param_num_temp, value);
		params_temp[param_num_temp] = value;
	}

	//find method info
	METHOD_INFO* method;
	CLASS* this_class_info;
	if (!current_cp_ptr->if_reslution) {

		int refer_index = params_temp[0];
		printf("get refer_index %d\n", refer_index);

		INSTANCE* inst_ptr = INSTANCES + refer_index;
		this_class_info = inst_ptr->class_info;

		//	char* name = cp_ptr->class_name;
		method = research_method_info(this_class_info, name, descriptor);
		if (method == NULL) {
			printf("do not find the method: %s\n", name);
			return;
		}
		current_cp_ptr->method = method;
		current_cp_ptr->if_reslution = 1;
	} else {
		method = current_cp_ptr->method;
	}

	create_stack_frame(*method);
	param_num_temp = param_num;
	while (param_num_temp--) {
		(current_frame->local_variable_erea)[param_num_temp] =
				params_temp[param_num_temp];
	}

	while (current_frame->pc < current_frame->code_length) {
		unsigned char byte_code =
				(current_frame->byte_codes)[(current_frame->pc)++];
		printf("pc #%d: 0X%X	", current_frame->pc - 1, byte_code);
		dispatch_bytecode(byte_code, current_class_info);
	}

}

void istore() {
	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;

	unsigned char index = byte_codes[(current_frame->pc)++];

	int value = curr_pop();
	printf("istore %d\n", value);
	(current_frame->local_variable_erea)[index] = value;
}

void istore_0() {
	CLASS* class_info = current_frame->class_info;
	int value = curr_pop();
	printf("istore_0 %d\n", value);
	(current_frame->local_variable_erea)[0] = value;
}

void istore_1() {
	CLASS* class_info = current_frame->class_info;
	int value = curr_pop();
	printf("istore_1 %d\n", value);
	(current_frame->local_variable_erea)[1] = value;
}

void istore_2() {
	CLASS* class_info = current_frame->class_info;
	int value = curr_pop();
	(current_frame->local_variable_erea)[2] = value;
	printf("istore_2 %d\n", value);
}

void istore_3() {
	CLASS* class_info = current_frame->class_info;
	int value = curr_pop();
	(current_frame->local_variable_erea)[3] = value;
	printf("istore_3 %d\n", value);
}

void dup_jvm() {
	int value = curr_pop();
	curr_push(value);
	curr_push(value);

	printf("dup %d\n", value);
}

void iload() {
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	int value = (current_frame->local_variable_erea)[index];
	curr_push(value);

	printf("iload %d\n", index);
}

void iload_0() {
	int value = (current_frame->local_variable_erea)[0];
	curr_push(value);

	printf("iload_0 %d\n", value);
}

void iload_1() {
	int value = (current_frame->local_variable_erea)[1];
	curr_push(value);

	printf("iload_1 %d\n", value);
}

void iload_2() {
	int value = (current_frame->local_variable_erea)[2];
	curr_push(value);

	printf("iload_2 %d\n", value);
}

void iload_3() {
	int value = (current_frame->local_variable_erea)[3];
	curr_push(value);

	printf("iload_3 %d\n", value);
}

void iadd() {
	int value0 = curr_pop();
	int value1 = curr_pop();
	curr_push(value0 + value1);

	printf("iadd %d + %d \n", value0, value1);
}

void isub() {
	int value0 = curr_pop();
	int value1 = curr_pop();
	curr_push(value1 - value0);

	printf("isub %d - %d \n", value1, value0);
}

void imul() {
	int value0 = curr_pop();
	int value1 = curr_pop();
	curr_push(value1 * value0);

	printf("imul %d * %d \n", value0, value1);
}

void idiv() {
	int value0 = curr_pop();
	int value1 = curr_pop();
	curr_push(value1 / value0);

	printf("imul %d / %d \n", value1, value0);
}

void return_ins() {
	printf("method(%s) return.\n", current_frame->method_name);

	unsigned int frame_size = current_frame->frame_size;
	if (current_frame->last_frame != NULL)
		current_frame = current_frame->last_frame;
	rel_stack_area_memo(frame_size);
}

void ireturn() {
	printf("method(%s)  ireturn\n", current_frame->method_name);

	int return_value = curr_pop();
	unsigned int frame_size = current_frame->frame_size;

	if (current_frame->last_frame != NULL)
		current_frame = current_frame->last_frame;

	curr_push(return_value);
	rel_stack_area_memo(frame_size);
}

void aload() {
	CLASS* class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	int value = (current_frame->local_variable_erea)[index];
	curr_push(value);

	printf("aload #%d\n", index);
}

void aload_0() {
	CLASS* class_info = current_frame->class_info;
	int value = (current_frame->local_variable_erea)[0];
	curr_push(value);
	printf("aload_0 %d\n", value);
}

void aload_1() {
	CLASS* class_info = current_frame->class_info;
	int value = (current_frame->local_variable_erea)[1];
	curr_push(value);
	printf("aload_1 %d\n", value);
}

void aload_2() {
	CLASS* class_info = current_frame->class_info;
	int value = (current_frame->local_variable_erea)[2];
	curr_push(value);
	printf("aload_2 %d\n", value);
}

void aload_3() {
	CLASS* class_info = current_frame->class_info;
	int value = (current_frame->local_variable_erea)[3];
	curr_push(value);
	printf("aload_3 %d\n", value);
}

void astore() {
	CLASS* class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	int value = curr_pop();
	(current_frame->local_variable_erea)[index] = value;

	printf("astore #%d\n", index);
}

void astore_0() {
	CLASS* class_info = current_frame->class_info;
	int value = curr_pop();
	(current_frame->local_variable_erea)[0] = value;
	printf("astore_0 %d\n", value);
}

void astore_1() {
	CLASS* class_info = current_frame->class_info;
	int value = curr_pop();
	(current_frame->local_variable_erea)[1] = value;
	printf("astore_1 %d\n", value);
}

void astore_2() {
	CLASS* class_info = current_frame->class_info;
	int value = curr_pop();
	(current_frame->local_variable_erea)[2] = value;
	printf("astore_2 %d\n", value);
}

void astore_3() {
	CLASS* class_info = current_frame->class_info;
	int value = curr_pop();
	(current_frame->local_variable_erea)[3] = value;
	printf("astore_3 %d\n", value);
}

void dispatch_bytecode(unsigned char byte_code) {

	switch (byte_code) {
	case 0x10: {
		bipush();
		break;
	}
	case 0x11: {
		sipush();
		break;
	}
	case 0x15: {
		iload();
		break;
	}
	case 0x19: {
		aload();
		break;
	}

	case 0x1a: {
		iload_0();
		break;
	}
	case 0x1b: {
		iload_1();
		break;
	}
	case 0x1c: {
		iload_2();
		break;
	}
	case 0x1d: {
		iload_3();
		break;
	}
	case 0x2a: {
		aload_0();
		break;
	}
	case 0x2b: {
		aload_1();
		break;
	}
	case 0x2c: {
		aload_2();
		break;
	}
	case 0x2d: {
		aload_3();
		break;
	}
	case 0x3a: {
		astore();
		break;
	}
	case 0x3b: {
		istore_0();
		break;
	}
	case 0x36: {
		istore();
		break;
	}

	case 0x3c: {
		istore_1();
		break;
	}
	case 0x3d: {
		istore_2();
		break;
	}
	case 0x3e: {
		istore_3();
		break;
	}

	case 0x4b: {
		astore_0();
		break;
	}
	case 0x4c: {
		astore_1();
		break;
	}
	case 0x4d: {
		astore_2();
		break;
	}
	case 0x4e: {
		astore_3();
		break;
	}
	case 0x59: {
		dup_jvm();
		break;
	}
	case 0x60: {
		iadd();
		break;
	}

	case 0x64: {
		isub();
		break;
	}
	case 0x68: {
		imul();
		break;
	}
	case 0x6c: {
		idiv();
		break;
	}

	case 0xac: {
		ireturn();
		break;
	}

	case 0xb2: {
		getstatic();
		break;
	}

	case 0xb3: {
		putstatic();
		break;
	}

	case 0xb4: {
		getfield();
		break;
	}

	case 0xb5: {
		putfield();
		break;
	}
	case 0xb6: {
		invokevirtual();
		break;
	}
	case 0xb7: {
		invokespecial();
		break;
	}
	case 0xb8: {
		invokestatic();
		break;
	}

	case 0xb9: {
		invokeinterface();
		break;
	}

	case 0xbb: {
		new();
		return;
	}

	case 0xb1: {
		return_ins();
		return;
	}
	}
}

