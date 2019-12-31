#include <stdio.h>
#include <stdlib.h>
#include "struct.h"
#include "util.h"
#include "jvm_init.h"
#include "operand_stack.h"
#include "class_loader.h"

/**
 *byte code interpreter
 */

extern STACK_FRAME* current_frame;
extern unsigned int INST_INDEX;
extern INSTANCE* INSTANCES;
extern int NULL_POINT;
extern int ACC_INTERFACE;
extern int PRINT_LOG;

void new() {

	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned short index = byte_codes[current_frame->pc++] << 8
			| byte_codes[current_frame->pc++];
	if (PRINT_LOG)
		printf("new #%d\n", index);

	CONSTANT_Class_Info * cp_ptr =
			((CONSTANT_Class_Info **) current_class_info->cps)[index];

	char* class_name = cp_ptr->class_name;
	if (PRINT_LOG)
		printf("new class name: %s\n", class_name);

	//load and init this class
	CLASS* new_class_info = load_class(class_name);

	//get inst filed num
	unsigned short total_fields_num = get_inst_filed_total_num(new_class_info);
	new_class_info->total_fields_count = total_fields_num;

	//allocate memory
	INSTANCE* inst_ptr = req_inst();
	INST_FIELD_VALUE* fields_ptr = (INST_FIELD_VALUE*) req_heap_area_memo(
			total_fields_num * sizeof(INST_FIELD_VALUE));

	inst_ptr->class_info = new_class_info;
	inst_ptr->inst_field_value_ptr = fields_ptr;

	curr_push(INST_INDEX - 1);

	//set default value for inst field
	init_inst_fields(new_class_info, total_fields_num, fields_ptr);
}

void newarray() {

	CLASS* class_info = current_frame->class_info;
	unsigned char* byte_codes = current_frame->byte_codes;
	unsigned short atype = byte_codes[current_frame->pc++];
	unsigned int item_count = curr_pop();

	//create array CLASS
	char* array_class_name;
	switch (atype) {
	//BOOLEAN
	case 4: {
		array_class_name = "[B";
		break;
	}
		//CHAR
	case 5: {
		array_class_name = "[C";
		break;
	}
		//FLOAT
	case 6: {
		array_class_name = "[F";
		break;
	}
		//DOUBLE
	case 7: {
		array_class_name = "[D";
		break;
	}
		//BYTE
	case 8: {
		array_class_name = "[B";
		break;
	}
		//SHORT
	case 9: {
		array_class_name = "[S";
		break;
	}
		//INT
	case 10: {
		array_class_name = "[I";
		break;
	}
		//LONG
	case 11: {
		array_class_name = "[J";
		break;
	}
	}
	//check if this class has bean loaded
	CLASS* array_class_info = search_class_info(array_class_name);
	if (array_class_info == NULL) {
		array_class_info = req_class();
		array_class_info->total_fields_count = 2;
		array_class_info->super_class_name = "java.lang.Object";
		array_class_info->this_class_name = array_class_name;
	}

	//create array instance
	INSTANCE * array_inst_ptr = req_inst();
	array_inst_ptr->class_info = array_class_info;

	//allocate memory for instance fields
	INST_FIELD_VALUE* fields_ptr = (INST_FIELD_VALUE*) req_heap_area_memo(
			2 * sizeof(INST_FIELD_VALUE));
	array_inst_ptr->inst_field_value_ptr = fields_ptr;

	//set length field
	INST_FIELD_VALUE* length_field = fields_ptr + 0;
	length_field->name = "length";
	length_field->descriptor = "I";
	int* length_value_ptr = (int*) req_heap_area_memo(sizeof(int));
	*length_value_ptr = item_count;
	length_field->field_value_ptr = length_value_ptr;

	//set buffer
	INST_FIELD_VALUE* buffer_field = fields_ptr + 1;
	buffer_field->name = "buffer";
	switch (atype) {
	//BOOLEAN
	case 4: {
		buffer_field->descriptor = "Z";
		int* buffer_value_ptr = (int*) req_heap_area_memo(
				item_count * sizeof(int));
		buffer_field->field_value_ptr = buffer_value_ptr;
		break;
	}
		//CHAR
	case 5: {
		buffer_field->descriptor = "C";
		int* buffer_value_ptr = (int*) req_heap_area_memo(
				item_count * sizeof(int));
		buffer_field->field_value_ptr = buffer_value_ptr;
		break;
	}
		//FLOAT
	case 6: {
		break;
	}
		//DOUBLE
	case 7: {
		break;
	}
		//BYTE
	case 8: {
		buffer_field->descriptor = "B";
		int* buffer_value_ptr = (int*) req_heap_area_memo(
				item_count * sizeof(int));
		buffer_field->field_value_ptr = buffer_value_ptr;
		break;
	}
		//SHORT
	case 9: {
		buffer_field->descriptor = "S";
		int* buffer_value_ptr = (int*) req_heap_area_memo(
				item_count * sizeof(int));
		buffer_field->field_value_ptr = buffer_value_ptr;
		break;
	}
		//INT
	case 10: {
		buffer_field->descriptor = "I";
		int* buffer_value_ptr = (int*) req_heap_area_memo(
				item_count * sizeof(int));
		buffer_field->field_value_ptr = buffer_value_ptr;
		break;
	}
		//LONG
	case 11: {
		buffer_field->descriptor = "J";
		int* buffer_value_ptr = (int*) req_heap_area_memo(
				item_count * sizeof(long));
		buffer_field->field_value_ptr = buffer_value_ptr;
		break;
	}
	}

	if (PRINT_LOG)
		printf("newarray item_count:%d array type:%s\n", item_count,
				array_class_info->this_class_name);

	curr_push(INST_INDEX - 1);
}

void anewarray() {

	CLASS* class_info = current_frame->class_info;
	unsigned char* byte_codes = current_frame->byte_codes;
	unsigned short index = (byte_codes[current_frame->pc++] << 8)
			| byte_codes[current_frame->pc++];
	unsigned int item_count = curr_pop();

	CONSTANT_Class_Info* item_class_info_ptr =
			((CONSTANT_Class_Info**) class_info->cps)[index];

	//get array class name
	char* item_class_name = item_class_info_ptr->class_name;
	unsigned int array_class_name_length = str_len(item_class_name) + 4;
	char* array_class_name = (char*) req_method_area_memo(
			array_class_name_length);
	array_class_name[0] = '[';
	array_class_name[1] = 'L';
	for (int i = 0; i < str_len(item_class_name); i++) {
		char charr = item_class_name[i];
		if (charr == '/') {
			charr = '.';
		}
		array_class_name[i + 2] = charr;
	}
	array_class_name[array_class_name_length - 2] = ';';
	array_class_name[array_class_name_length - 1] = 0;
	if (PRINT_LOG)
		printf("anewarray item class name:%s\n", array_class_name);

	//check if this class has bean loaded
	CLASS* array_class_info = search_class_info(array_class_name);
	if (array_class_info == NULL) {
		array_class_info = req_class();
		array_class_info->super_class_name = "java.lang.Object";
		array_class_info->this_class_name = array_class_name;
	}

	//create array instance
	INSTANCE* inst_ptr = req_inst();
	inst_ptr->class_info = array_class_info;

	//allocate memory for instance fields
	INST_FIELD_VALUE* fields_ptr = (INST_FIELD_VALUE*) req_heap_area_memo(
			2 * sizeof(INST_FIELD_VALUE));
	inst_ptr->inst_field_value_ptr = fields_ptr;

	//set length field
	INST_FIELD_VALUE* length_field = fields_ptr + 0;
	length_field->name = "length";
	length_field->descriptor = "I";
	int* length_value_ptr = (int*) req_heap_area_memo(sizeof(int));
	*length_value_ptr = item_count;
	length_field->field_value_ptr = length_value_ptr;

	//set buffer
	INST_FIELD_VALUE* buffer_field = fields_ptr + 1;
	buffer_field->name = "buffer";
	buffer_field->descriptor = item_class_name;
	int* buffer_value_ptr = (int*) req_heap_area_memo(item_count * sizeof(int));
	buffer_field->field_value_ptr = buffer_value_ptr;

	if (PRINT_LOG)
		printf("anewarray item_count:%d array type:%s\n", item_count,
				array_class_info->this_class_name);

	//push
	curr_push(INST_INDEX - 1);
}

void arraylength() {
	unsigned int arrayref = curr_pop();

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	unsigned int length = *(field_value->field_value_ptr);
	curr_push(length);

	if (PRINT_LOG)
		printf("arraylength array_type:%s array_length:%d\n",
				inst.class_info->this_class_name, length);
}
void athrow() {
	if (PRINT_LOG)
		printf("athrow(unhandled)\n");
}

void putfield() {

	CLASS* class_info = current_frame->class_info;
	unsigned char* byte_codes = current_frame->byte_codes;
	unsigned short index = (byte_codes[current_frame->pc++] << 8)
			| byte_codes[current_frame->pc++];

	if (PRINT_LOG)
		printf("putfield #%d\n", index);

	CONSTANT_Fieldref_Info* fieldref_ptr =
			((CONSTANT_Fieldref_Info**) class_info->cps)[index];
//	char* class_name = cp_ptr->class_name;
	char* filed_name = fieldref_ptr->filed_name;
	char* filed_descriptor = fieldref_ptr->filed_descriptor;

	unsigned int value = curr_pop();
	int refer_index = curr_pop();

	if (PRINT_LOG)
		printf("put refer_index %d\n", refer_index);
	if (PRINT_LOG)
		printf("put inst field value %d\n", value);

	INSTANCE inst = INSTANCES[refer_index];

	INST_FIELD_VALUE* field = research_inst_field_value(inst, filed_name,
			filed_descriptor);
	*(field->field_value_ptr) = value;
}

void getfield() {
	CLASS* class_info = current_frame->class_info;
	unsigned char* byte_codes = current_frame->byte_codes;
	unsigned short index = (byte_codes[current_frame->pc++] << 8)
			| byte_codes[current_frame->pc++];

	if (PRINT_LOG)
		printf("getfield #%d\n", index);

	CONSTANT_Fieldref_Info* fieldref_ptr =
			((CONSTANT_Fieldref_Info**) class_info->cps)[index];
//	char* class_name = cp_ptr->class_name;
	char* filed_name = fieldref_ptr->filed_name;
	char* filed_descriptor = fieldref_ptr->filed_descriptor;

	int refer_index = curr_pop();
	if (PRINT_LOG)
		printf("get refer_index %d\n", refer_index);
	INSTANCE inst = INSTANCES[refer_index];

	INST_FIELD_VALUE* field = research_inst_field_value(inst, filed_name,
			filed_descriptor);
	unsigned int value = *(field->field_value_ptr);
	curr_push(value);
	if (PRINT_LOG)
		printf("get inst field value %d\n", value);
}

void bipush() {

	unsigned char *byte_codes = current_frame->byte_codes;
	char item = byte_codes[current_frame->pc++];
	if (PRINT_LOG)
		printf("bipush %d\n", item);
	curr_push(signed_byte_int(item));
}

void sipush() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short item = byte_codes[current_frame->pc++] << 8
			| byte_codes[current_frame->pc++];
	if (PRINT_LOG)
		printf("sipush %d\n", item);
	curr_push(signed_short_int(item));
}

void putstatic() {
	CLASS* class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned short index = byte_codes[current_frame->pc++] << 8
			| byte_codes[current_frame->pc++];

	if (PRINT_LOG)
		printf("putstatic #%d\n", index);

	CONSTANT_Fieldref_Info * cp_ptr =
			((CONSTANT_Fieldref_Info **) class_info->cps)[index];

	char* class_name = cp_ptr->class_name;
	char* filed_name = cp_ptr->filed_name;
	char* filed_descriptor = cp_ptr->filed_descriptor;

	CLASS* tar_class_info = search_class_info(class_name);
	if (tar_class_info == NULL) {
		tar_class_info = load_class(class_name);
	}

	STATIC_FIELD_VALUE* field_ptr = research_static_field_value(tar_class_info,
			filed_name, filed_descriptor);

	if (str_equal(filed_descriptor, "D") || str_equal(filed_descriptor, "J")
			|| str_equal(filed_descriptor, "L")) {
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

	if (PRINT_LOG)
		printf("getstatic #%d\n", index);

	CONSTANT_Fieldref_Info* cp_ptr =
			((CONSTANT_Fieldref_Info**) class_info->cps)[index];
	char* class_name = cp_ptr->class_name;
	char* filed_name = cp_ptr->filed_name;
	char* filed_descriptor = cp_ptr->filed_descriptor;

	CLASS* tar_class_info = search_class_info(class_name);
	if (tar_class_info == NULL) {
		tar_class_info = load_class(class_name);
	}

	STATIC_FIELD_VALUE* field = research_static_field_value(tar_class_info,
			filed_name, filed_descriptor);

	if (str_equal(filed_descriptor, "D") || str_equal(filed_descriptor, "J")
			|| str_equal(filed_descriptor, "L")) {
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

	if (PRINT_LOG)
		printf("invokespecial #%d\n", index);

	CONSTANT_Methodref_Info * methodref_ptr =
			((CONSTANT_Methodref_Info **) current_class_info->cps)[index];

	char* class_name = methodref_ptr->class_name;
	char* method_name = methodref_ptr->method_name;
	char* descriptor = methodref_ptr->method_descriptor;
	if (PRINT_LOG)
		printf("class name:%s, inst method name:%s, inst method descriptor:%s\n",
				class_name, method_name, descriptor);
	CLASS* tar_class_info = search_class_info(class_name);

//find method info
	METHOD_INFO* method = research_method_info(methodref_ptr, tar_class_info,
			method_name, descriptor);

	create_stack_frame(*method);
// pass parameter,include this param
	unsigned int param_num = get_param_num(descriptor) + 1;
	while (param_num--) {
		int value = last_pop();
		if (PRINT_LOG)
			printf("param %d: %d\n", param_num, value);
		(current_frame->local_variable_erea)[param_num] = value;
	}

	while (current_frame->pc < current_frame->code_length) {
		unsigned char byte_code =
				(current_frame->byte_codes)[(current_frame->pc)++];
		if (PRINT_LOG)
			printf("pc #%d: 0X%X	", current_frame->pc - 1, byte_code);
		bytecode_dispatch(byte_code);
	}
}

void invokevirtual() {
	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;

	unsigned short index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	if (PRINT_LOG)
		printf("invokevirtual #%d\n", index);

	CONSTANT_Methodref_Info * methodref_ptr =
			((CONSTANT_Methodref_Info **) current_class_info->cps)[index];

	char* class_name = methodref_ptr->class_name;
	char* method_name = methodref_ptr->method_name;
	if (PRINT_LOG)
		printf("class name:%s, Inst method name:%s\n", class_name, method_name);

	char* descriptor = methodref_ptr->method_descriptor;
//include this param
	unsigned int param_num = get_param_num(descriptor) + 1;
	int params_temp[param_num];
	int param_num_temp = param_num;
	while (param_num_temp--) {
		int value = curr_pop();
		if (PRINT_LOG)
			printf("param %d: %d\n", param_num_temp, value);
		params_temp[param_num_temp] = value;
	}

//find method info
	int refer_index = params_temp[0];
	if (PRINT_LOG)
		printf("get refer_index %d\n", refer_index);

	INSTANCE* inst_ptr = INSTANCES + refer_index;
	CLASS* tar_class_info = inst_ptr->class_info;

//	char* name = cp_ptr->class_name;
	METHOD_INFO *method = research_method_info(methodref_ptr, tar_class_info,
			method_name, descriptor);

	create_stack_frame(*method);
	param_num_temp = param_num;
	while (param_num_temp--) {
		(current_frame->local_variable_erea)[param_num_temp] =
				params_temp[param_num_temp];
	}

	while (current_frame->pc < current_frame->code_length) {
		unsigned char byte_code =
				(current_frame->byte_codes)[(current_frame->pc)++];
		if (PRINT_LOG)
			printf("pc #%d: 0X%X	", current_frame->pc - 1, byte_code);
		bytecode_dispatch(byte_code);
	}
}

void invokestatic() {
	CLASS* class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned short index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	if (PRINT_LOG)
		printf("invokestatic #%d\n", index);

	CONSTANT_Methodref_Info * methodref_ptr =
			((CONSTANT_Methodref_Info **) class_info->cps)[index];

	char* class_name = methodref_ptr->class_name;
	char* method_name = methodref_ptr->method_name;
	if (PRINT_LOG)
		printf("class name:%s,static method name:%s\n", class_name,
				method_name);
	CLASS* tar_class_info = search_class_info(class_name);

	char* descriptor = methodref_ptr->method_descriptor;

	METHOD_INFO* method = research_method_info(methodref_ptr, tar_class_info,
			method_name, descriptor);

	unsigned short access_flags = method->access_flags;
	if (access_flags & 0x0100) {

		if (PRINT_LOG)
			printf("native method name: %s\n", method_name);
		if (str_equal("println", method_name)
				&& str_equal("(I)V", descriptor)) {

			if (PRINT_LOG)
				printf("print final int value: %d\n", curr_pop());

		} else if (str_equal("println", method_name)
				&& str_equal("(J)V", descriptor)) {
			unsigned long high_32 = curr_pop();
			unsigned int low_32 = curr_pop();

			long value = high_32 << 32 | low_32;
			if (PRINT_LOG)
				printf("print final long value: %ld\n", value);
		} else if (str_equal("println", method_name)
				&& str_equal("(Z)V", descriptor)) {
			unsigned int value = curr_pop();

			if (value) {
				if (PRINT_LOG)
					printf("print boolean value: true\n");
			} else {
				if (PRINT_LOG)
					printf("print boolean value: false\n");
			}

		} else if (str_equal("println", method_name)
				&& str_equal("(C)V", descriptor)) {
			char value = curr_pop();
			if (PRINT_LOG)
				printf("print char value: %c\n", value);

		} else if (str_equal("println", method_name)
				&& str_equal("(Ljava/lang/String;)V", descriptor)) {
			unsigned int value = curr_pop();

			INSTANCE str = INSTANCES[value];
			INST_FIELD_VALUE* value_field_value = research_inst_field_value(str,
					"value", "[C");

			int str_value = *(value_field_value->field_value_ptr);

			INSTANCE chars = INSTANCES[str_value];
			INST_FIELD_VALUE* length_field_value = chars.inst_field_value_ptr
					+ 0;
			int length = *(length_field_value->field_value_ptr);

			INST_FIELD_VALUE* buffer_field_value = chars.inst_field_value_ptr
					+ 1;
			int *buffer = buffer_field_value->field_value_ptr;
			if (PRINT_LOG)
				printf("print string value: ");
			for (unsigned int i = 0; i < length; i++) {
				char byte = (char) buffer[i];
				if (PRINT_LOG)
					printf("%c", byte);
			}
			if (PRINT_LOG)
				printf("\n");

		} else {

			//to do
		}

	} else {
		create_stack_frame(*method);

		unsigned int param_num = get_param_num(descriptor);
		while (param_num--) {
			int value = last_pop();
			if (PRINT_LOG)
				printf("param:%d\n", value);
			(current_frame->local_variable_erea)[param_num] = value;
		}

		while (current_frame->pc < current_frame->code_length) {
			unsigned char byte_code =
					(current_frame->byte_codes)[(current_frame->pc)++];
			if (PRINT_LOG)
				printf("pc #%d: 0X%X	", current_frame->pc, byte_code);
			bytecode_dispatch(byte_code);
		}
	}
}

void invokeinterface() {
	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;

	unsigned short index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	if (PRINT_LOG)
		printf("invokeinterface #%d\n", index);

	//The count the number of argument values,the redundancy is historical
	unsigned char count = byte_codes[(current_frame->pc)++];
	//The fourth operand byte exists to reserve space for an additional
	//operand used in certain of Oracle's Java Virtual Machine implementations
	unsigned char zero = byte_codes[(current_frame->pc)++];

	CONSTANT_Methodref_Info * methodref_ptr =
			((CONSTANT_Methodref_Info **) current_class_info->cps)[index];

	char* class_name = methodref_ptr->class_name;
	char* method_name = methodref_ptr->method_name;
	if (PRINT_LOG)
		printf("Interface name:%s, interface method name:%s\n", class_name,
				method_name);

	char* descriptor = methodref_ptr->method_descriptor;
//include this param
	unsigned int param_num = get_param_num(descriptor) + 1;
	int params_temp[param_num];
	int param_num_temp = param_num;
	while (param_num_temp--) {
		int value = curr_pop();
		if (PRINT_LOG)
			printf("param %d: %d\n", param_num_temp, value);
		params_temp[param_num_temp] = value;
	}

//find method info
	int refer_index = params_temp[0];
	if (PRINT_LOG)
		printf("get refer_index %d\n", refer_index);

	INSTANCE* inst_ptr = INSTANCES + refer_index;
	CLASS*tar_class_info = inst_ptr->class_info;

//	char* name = cp_ptr->class_name;
	METHOD_INFO* method = research_method_info(methodref_ptr, tar_class_info,
			method_name, descriptor);

	create_stack_frame(*method);
	param_num_temp = param_num;
	while (param_num_temp--) {
		(current_frame->local_variable_erea)[param_num_temp] =
				params_temp[param_num_temp];
	}

	while (current_frame->pc < current_frame->code_length) {
		unsigned char byte_code =
				(current_frame->byte_codes)[(current_frame->pc)++];
		if (PRINT_LOG)
			printf("pc #%d: 0X%X	", current_frame->pc - 1, byte_code);
		bytecode_dispatch(byte_code);
	}

}

void invokedynamic() {
	if (PRINT_LOG)
		printf("invokedynamic(unhandled)\n");
}

void istore() {
	unsigned char *byte_codes = current_frame->byte_codes;

	unsigned char index = byte_codes[(current_frame->pc)++];

	int value = curr_pop();
	if (PRINT_LOG)
		printf("istore %d\n", value);
	(current_frame->local_variable_erea)[index] = value;
}

void istore_0() {
	int value = curr_pop();
	if (PRINT_LOG)
		printf("istore_0 %d\n", value);
	(current_frame->local_variable_erea)[0] = value;
}

void istore_1() {
	int value = curr_pop();
	if (PRINT_LOG)
		printf("istore_1 %d\n", value);
	(current_frame->local_variable_erea)[1] = value;
}

void istore_2() {
	int value = curr_pop();
	(current_frame->local_variable_erea)[2] = value;
	if (PRINT_LOG)
		printf("istore_2 %d\n", value);
}

void istore_3() {
	int value = curr_pop();
	(current_frame->local_variable_erea)[3] = value;
	if (PRINT_LOG)
		printf("istore_3 %d\n", value);
}

void dup_jvm() {
	int value = curr_pop();
	curr_push(value);
	curr_push(value);

	if (PRINT_LOG)
		printf("dup %d\n", value);
}

void dup_x1() {

	int value1 = curr_pop();
	int value2 = curr_pop();

	curr_push(value1);
	curr_push(value2);
	curr_push(value1);

	if (PRINT_LOG)
		printf("dup_x1 value1(%d) value2(%d)\n", value1, value2);
}
void dup_x2() {
	int value1 = curr_pop();
	int value2 = curr_pop();
	int value3 = curr_pop();

	curr_push(value1);
	curr_push(value3);
	curr_push(value2);
	curr_push(value1);

	if (PRINT_LOG)
		printf("dup_x2 value1(%d) value2(%d) value3(%d)\n", value1, value2,
				value3);
}
void dup2_jvm() {
	int value1 = curr_pop();
	int value2 = curr_pop();

	curr_push(value2);
	curr_push(value1);
	curr_push(value2);
	curr_push(value1);

	if (PRINT_LOG)
		printf("dup2 value1(%d) value2(%d)\n", value1, value2);
}

void dup2_x1() {

	int value1 = curr_pop();
	int value2 = curr_pop();
	int value3 = curr_pop();

	curr_push(value2);
	curr_push(value1);
	curr_push(value3);
	curr_push(value2);
	curr_push(value1);

	if (PRINT_LOG)
		printf("dup2_x1 value1(%d) value2(%d) value3(%d)\n", value1, value2,
				value3);
}

void dup2_x2() {

	int value1 = curr_pop();
	int value2 = curr_pop();
	int value3 = curr_pop();
	int value4 = curr_pop();

	curr_push(value2);
	curr_push(value1);
	curr_push(value4);
	curr_push(value3);
	curr_push(value2);
	curr_push(value1);

	if (PRINT_LOG)
		printf("dup2_x2 value1(%d) value2(%d) value3(%d) value4(%d)\n", value1,
				value2, value3, value4);
}

void swap() {
	int value1 = curr_pop();
	int value2 = curr_pop();

	curr_push(value1);
	curr_push(value2);

	if (PRINT_LOG)
		printf("swap value1(%d) value2(%d)\n", value1, value2);
}

void iload() {
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	int value = (current_frame->local_variable_erea)[index];
	curr_push(value);

	if (PRINT_LOG)
		printf("iload %d\n", index);
}

void lload() {
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	unsigned int low_32 = (current_frame->local_variable_erea)[index];
	unsigned int high_32 = (current_frame->local_variable_erea)[index + 1];

	curr_push(low_32);
	curr_push(high_32);

	if (PRINT_LOG)
		printf("lload\n");
}

void fload() {
}

void dload() {
}

void iload_0() {
	int value = (current_frame->local_variable_erea)[0];
	curr_push(value);

	if (PRINT_LOG)
		printf("iload_0 %d\n", value);
}

void iload_1() {
	int value = (current_frame->local_variable_erea)[1];
	curr_push(value);

	if (PRINT_LOG)
		printf("iload_1 %d\n", value);
}

void iload_2() {
	int value = (current_frame->local_variable_erea)[2];
	curr_push(value);

	if (PRINT_LOG)
		printf("iload_2 %d\n", value);
}

void iload_3() {
	int value = (current_frame->local_variable_erea)[3];
	curr_push(value);

	if (PRINT_LOG)
		printf("iload_3 %d\n", value);
}

void lload_0() {
	unsigned int low_32 = (current_frame->local_variable_erea)[0];
	unsigned int high_32 = (current_frame->local_variable_erea)[1];

	curr_push(low_32);
	curr_push(high_32);

	if (PRINT_LOG)
		printf("lload_0\n");
}

void lload_1() {
	unsigned int low_32 = (current_frame->local_variable_erea)[1];
	unsigned int high_32 = (current_frame->local_variable_erea)[2];

	curr_push(low_32);
	curr_push(high_32);

	if (PRINT_LOG)
		printf("lload_1\n");
}

void lload_2() {
	unsigned int low_32 = (current_frame->local_variable_erea)[2];
	unsigned int high_32 = (current_frame->local_variable_erea)[3];

	curr_push(low_32);
	curr_push(high_32);

	if (PRINT_LOG)
		printf("lload_2\n");
}

void lload_3() {
	unsigned int low_32 = (current_frame->local_variable_erea)[3];
	unsigned int high_32 = (current_frame->local_variable_erea)[4];

	curr_push(low_32);
	curr_push(high_32);

	if (PRINT_LOG)
		printf("lload_3\n");
}

void fload_0() {
}

void fload_1() {
}

void fload_2() {
}

void fload_3() {
}

void dload_0() {
}

void dload_1() {
}

void dload_2() {
}

void dload_3() {
}

void iadd() {
	int value0 = curr_pop();
	int value1 = curr_pop();
	curr_push(value0 + value1);

	if (PRINT_LOG)
		printf("iadd %d + %d \n", value0, value1);
}

void isub() {
	int value0 = curr_pop();
	int value1 = curr_pop();
	curr_push(value1 - value0);

	if (PRINT_LOG)
		printf("isub %d - %d \n", value1, value0);
}

void imul() {
	int value0 = curr_pop();
	int value1 = curr_pop();
	curr_push(value1 * value0);

	if (PRINT_LOG)
		printf("imul %d * %d \n", value0, value1);
}

void idiv() {
	int value0 = curr_pop();
	int value1 = curr_pop();

	if (PRINT_LOG)
		printf("idiv %d / %d \n", value1, value0);

	curr_push(value1 / value0);
}

void return_void() {
	if (PRINT_LOG)
		printf("class(%s) method(%s) return_void.\n",
				current_frame->class_info->this_class_name,
				current_frame->method_name);

	unsigned int frame_size = current_frame->frame_size;
	if (current_frame->last_frame != NULL)
		current_frame = (STACK_FRAME*) current_frame->last_frame;

	rel_stack_area_memo(frame_size);
}

void ireturn() {
	if (PRINT_LOG)
		printf("class(%s) method(%s)  ireturn\n",
				current_frame->class_info->this_class_name,
				current_frame->method_name);

	int return_value = curr_pop();
	if (current_frame->last_frame != NULL) {
		current_frame = (STACK_FRAME*) current_frame->last_frame;
		curr_push(return_value);
	}

	unsigned int frame_size = current_frame->frame_size;
	rel_stack_area_memo(frame_size);
}

void lreturn() {
	if (PRINT_LOG)
		printf("class(%s) method(%s)  lreturn\n",
				current_frame->class_info->this_class_name,
				current_frame->method_name);

	//high 32 in stack top,low 32 in stack bottom
	int high_32 = curr_pop();
	int low_32 = curr_pop();

	if (current_frame->last_frame != NULL) {
		current_frame = (STACK_FRAME*) current_frame->last_frame;
		curr_push(low_32);
		curr_push(high_32);
	}

	unsigned int frame_size = current_frame->frame_size;
	rel_stack_area_memo(frame_size);
}
void freturn() {
	if (PRINT_LOG)
		printf("class(%s) method(%s)  freturn\n",
				current_frame->class_info->this_class_name,
				current_frame->method_name);

	//high 32 in stack top,low 32 in stack bottom
	int high_32 = curr_pop();
	int low_32 = curr_pop();

	if (current_frame->last_frame != NULL) {
		current_frame = (STACK_FRAME*) current_frame->last_frame;
		curr_push(low_32);
		curr_push(high_32);
	}

	unsigned int frame_size = current_frame->frame_size;
	rel_stack_area_memo(frame_size);
}

void dreturn() {
	if (PRINT_LOG)
		printf("class(%s) method(%s)  dreturn\n",
				current_frame->class_info->this_class_name,
				current_frame->method_name);

	//high 32 in stack top,low 32 in stack bottom
	int high_32 = curr_pop();
	int low_32 = curr_pop();

	if (current_frame->last_frame != NULL) {
		current_frame = (STACK_FRAME*) current_frame->last_frame;
		curr_push(low_32);
		curr_push(high_32);
	}

	unsigned int frame_size = current_frame->frame_size;
	rel_stack_area_memo(frame_size);
}

void areturn() {
	if (PRINT_LOG)
		printf("class(%s) method(%s)  areturn\n",
				current_frame->class_info->this_class_name,
				current_frame->method_name);

	int return_value = curr_pop();
	if (current_frame->last_frame != NULL) {
		current_frame = (STACK_FRAME*) current_frame->last_frame;
		curr_push(return_value);
	}

	unsigned int frame_size = current_frame->frame_size;
	rel_stack_area_memo(frame_size);
}

void aload() {
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	int value = (current_frame->local_variable_erea)[index];
	curr_push(value);

	if (PRINT_LOG)
		printf("aload #%d\n", index);
}

void aload_0() {
	int value = (current_frame->local_variable_erea)[0];
	curr_push(value);
	if (PRINT_LOG)
		printf("aload_0 %d\n", value);
}

void aload_1() {
	int value = (current_frame->local_variable_erea)[1];
	curr_push(value);
	if (PRINT_LOG)
		printf("aload_1 %d\n", value);
}

void aload_2() {
	int value = (current_frame->local_variable_erea)[2];
	curr_push(value);
	if (PRINT_LOG)
		printf("aload_2 %d\n", value);
}

void aload_3() {
	int value = (current_frame->local_variable_erea)[3];
	curr_push(value);
	if (PRINT_LOG)
		printf("aload_3 %d\n", value);
}

void iaload() {
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	if (PRINT_LOG)
		printf("iaload index:%d\n", index);

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	int value = field_value_ptr[index];
	curr_push(value);

}

void laload() {
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	if (PRINT_LOG)
		printf("laload index:%d\n", index);

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	long value = *((long*) field_value_ptr + index);

	//high 32 in stack top,low 32 in stack bottom
	unsigned int low_32 = (value & 0x00000000ffffffff);
	curr_push(low_32);

	unsigned int high_32 = (value & 0xffffffff00000000) >> 32;
	curr_push(high_32);
}

void faload() {
}

void daload() {
}
void aaload() {
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	if (PRINT_LOG)
		printf("aaload index:%d\n", index);

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	int value = field_value_ptr[index];
	curr_push(value);
}

void caload() {
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	if (PRINT_LOG)
		printf("caload index:%d\n", index);

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	int value = field_value_ptr[index];
	curr_push(value);
}

void saload() {
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	if (PRINT_LOG)
		printf("saload index:%d\n", index);

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	int value = field_value_ptr[index];
	curr_push(value);
}

void astore() {
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	int value = curr_pop();
	(current_frame->local_variable_erea)[index] = value;

	if (PRINT_LOG)
		printf("astore #%d\n", index);
}

void astore_0() {
	int value = curr_pop();
	(current_frame->local_variable_erea)[0] = value;
	if (PRINT_LOG)
		printf("astore_0 %d\n", value);
}

void astore_1() {
	int value = curr_pop();
	(current_frame->local_variable_erea)[1] = value;
	if (PRINT_LOG)
		printf("astore_1 %d\n", value);
}

void astore_2() {
	int value = curr_pop();
	(current_frame->local_variable_erea)[2] = value;
	if (PRINT_LOG)
		printf("astore_2 %d\n", value);
}

void astore_3() {
	int value = curr_pop();
	(current_frame->local_variable_erea)[3] = value;
	if (PRINT_LOG)
		printf("astore_3 %d\n", value);
}

void iastore() {
	unsigned int value = curr_pop();
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	field_value_ptr[index] = value;

	if (PRINT_LOG)
		printf("iastore index:%d value:%d\n", index, value);

}

void fastore() {

}
void dastore() {

}

void aastore() {
	unsigned int value = curr_pop();
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	field_value_ptr[index] = value;

	if (PRINT_LOG)
		printf("aastore index:%d value:%d\n", index, value);

}

void bastore() {

	unsigned int value = curr_pop();
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	field_value_ptr[index] = value;

	if (PRINT_LOG)
		printf("bastore index:%d value:%d\n", index, value);
}

void castore() {
	unsigned int value = curr_pop();
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	field_value_ptr[index] = value;

	if (PRINT_LOG)
		printf("castore index:%d value:%d\n", index, value);

}

void sastore() {
	unsigned int value = curr_pop();
	unsigned int index = curr_pop();
	unsigned int arrayref = curr_pop();

	INSTANCE inst = INSTANCES[arrayref];
	INST_FIELD_VALUE* field_value = inst.inst_field_value_ptr;

	int array_length = *(field_value[0].field_value_ptr);
	if (index + 1 > array_length) {
		if (PRINT_LOG)
			printf("	ArrayIndexOutOfBoundsException\n");
		return;
	}

	int* field_value_ptr = field_value[1].field_value_ptr;
	field_value_ptr[index] = value;

	if (PRINT_LOG)
		printf("sastore index:%d value:%d\n", index, value);
}

void pop() {
	curr_pop();
}

void pop2() {
	curr_pop();
	curr_pop();
}

void iconst_m1() {
	curr_push(-1);
	if (PRINT_LOG)
		printf("iconst_m1(-1)\n");
}
void iconst_0() {
	curr_push(0);
	if (PRINT_LOG)
		printf("iconst_0\n");
}
void iconst_1() {
	curr_push(1);
	if (PRINT_LOG)
		printf("iconst_1\n");
}
void iconst_2() {
	curr_push(2);
	if (PRINT_LOG)
		printf("iconst_2\n");
}
void iconst_3() {
	curr_push(3);
	if (PRINT_LOG)
		printf("iconst_3\n");
}
void iconst_4() {
	curr_push(4);
	if (PRINT_LOG)
		printf("iconst_4\n");
}
void iconst_5() {
	curr_push(5);
	if (PRINT_LOG)
		printf("iconst_5\n");
}
void lconst_0() {
	curr_push(0);
	curr_push(0);
	if (PRINT_LOG)
		printf("lconst_0\n");
}
void lconst_1() {
	curr_push(1);
	curr_push(0);
	if (PRINT_LOG)
		printf("lconst_1\n");
}

void fconst_0() {
}

void fconst_1() {
}

void fconst_2() {
}

void dconst_0() {
}

void dconst_1() {
}

void if_icmpeq() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value2 = curr_pop();
	int value1 = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("if_icmpeq: value1(%d), value2(%d),target_pc(%d) \n", value1,
				value2, target_pc);

	if (value1 == value2) {
		current_frame->pc = target_pc;
	}
	return;
}

void if_icmpne() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value2 = curr_pop();
	int value1 = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("if_icmpne: value1(%d), value2(%d),target_pc(%d) \n", value1,
				value2, target_pc);

	if (value1 != value2) {
		current_frame->pc = target_pc;
	}
	return;
}

void if_icmplt() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value2 = curr_pop();
	int value1 = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("if_icmplt: value1(%d), value2(%d),target_pc(%d) \n", value1,
				value2, target_pc);

	if (value1 < value2) {
		current_frame->pc = target_pc;
	}
	return;
}

void if_icmpge() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value2 = curr_pop();
	int value1 = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("if_icmpge: value1(%d), value2(%d),target_pc(%d) \n", value1,
				value2, target_pc);

	if (value1 >= value2) {
		current_frame->pc = target_pc;
	}
	return;
}

void if_icmpgt() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value2 = curr_pop();
	int value1 = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("if_icmpgt: value1(%d), value2(%d),target_pc(%d) \n", value1,
				value2, target_pc);

	if (value1 > value2) {
		current_frame->pc = target_pc;
	}
	return;
}

void if_icmple() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value2 = curr_pop();
	int value1 = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("if_icmple: value1(%d), value2(%d),target_pc(%d) \n", value1,
				value2, target_pc);

	if (value1 <= value2) {
		current_frame->pc = target_pc;
	}
	return;
}

void if_acmpeq() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value2 = curr_pop();
	int value1 = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("if_acmpeq: value1(%d), value2(%d),target_pc(%d) \n", value1,
				value2, target_pc);

	if (value1 == value2) {
		current_frame->pc = target_pc;
	}
	return;
}

void if_acmpne() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value2 = curr_pop();
	int value1 = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("if_acmpne: value1(%d), value2(%d),target_pc(%d) \n", value1,
				value2, target_pc);

	if (value1 != value2) {
		current_frame->pc = target_pc;
	}
	return;
}

void go_to() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("goto: target_pc(%d)\n", target_pc);
	current_frame->pc = target_pc;
	return;
}

void jsr() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short address = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	curr_push(signed_short_int(address));
	if (PRINT_LOG)
		printf("jsr: address(%d)\n", address);
}

void jsr_w() {
	unsigned char *byte_codes = current_frame->byte_codes;
	int address = (byte_codes[(current_frame->pc)++] << 24)
			| (byte_codes[(current_frame->pc)++] << 16)
			| (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	curr_push(address);
	if (PRINT_LOG)
		printf("jsr_w: address(%d)\n", address);
}

void ret() {

	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	int address = current_frame->local_variable_erea[index];
	current_frame->pc = address;

	if (PRINT_LOG)
		printf("ret: address(%d)\n", address);
}

void tableswitch() {
	if (PRINT_LOG)
		printf("tableswitch(unhandeled)\n");
}

void lookupswitch() {
	if (PRINT_LOG)
		printf("lookupswitch(unhandeled)\n");
}

void iinc() {
	unsigned char *byte_codes = current_frame->byte_codes;

	unsigned char index = byte_codes[(current_frame->pc)++];
	unsigned char constant = byte_codes[(current_frame->pc)++];
	unsigned int constant_int = signed_byte_int(constant);

	current_frame->local_variable_erea[index] = constant_int
			+ current_frame->local_variable_erea[index];

	if (PRINT_LOG)
		printf("iinc:  index(%d) constant(%d)\n", index, constant);
}

void lcmp() {

	if (PRINT_LOG)
		printf("lcmp(unhandled)\n");
}

void fcmpl() {

	if (PRINT_LOG)
		printf("fcmpl(unhandled)\n");
}

void fcmpg() {

	if (PRINT_LOG)
		printf("fcmpg(unhandled)\n");
}

void dcmpl() {

	if (PRINT_LOG)
		printf("dcmpl(unhandled)\n");
}

void dcmpg() {

	if (PRINT_LOG)
		printf("dcmpg(unhandled)\n");
}

void ifeq() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("ifeq: if value(%d) equal 0, target_pc(%d) \n", value,
				target_pc);

	if (value == 0) {
		current_frame->pc = target_pc;
	}
	return;
}

void ifne() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("ifne: if value(%d) not equal 0, target_pc(%d) \n", value,
				target_pc);

	if (value != 0) {
		current_frame->pc = target_pc;
	}
	return;
}

void iflt() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("iflt: value(%d), target_pc(%d) \n", value, target_pc);

	if (value < 0) {
		current_frame->pc = target_pc;
	}
	return;
}

void ifge() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("ifge: value(%d), target_pc(%d) \n", value, target_pc);

	if (value >= 0) {
		current_frame->pc = target_pc;
	}
	return;
}

void ifgt() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("ifgt: value(%d), target_pc(%d) \n", value, target_pc);

	if (value > 0) {
		current_frame->pc = target_pc;
	}
	return;
}

void ifle() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	int value = curr_pop();

	unsigned int target_pc = current_frame->pc - 3 + offset;

	if (PRINT_LOG)
		printf("ifle: value(%d), target_pc(%d) \n", value, target_pc);

	if (value <= 0) {
		current_frame->pc = target_pc;
	}
	return;
}

void ldc() {
	CLASS* current_class_info = current_frame->class_info;

	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	CONSTANT_Common_Info* common_ptr =
			((CONSTANT_Common_Info **) current_class_info->cps)[index];

	if (common_ptr->tag == 3) {
		CONSTANT_Integer_Info* intref_ptr = (CONSTANT_Integer_Info*) common_ptr;
		curr_push(intref_ptr->value);
		if (PRINT_LOG)
			printf("ldc:  index(%d) int value(%d)\n", index, intref_ptr->value);
	} else if (common_ptr->tag == 8) {
		CONSTANT_String_Info* intref_ptr = (CONSTANT_String_Info*) common_ptr;

		curr_push(intref_ptr->str_obj_ref);
		if (PRINT_LOG)
			printf("ldc:  index(%d) String refer(%d) value(%s)\n", index,
					intref_ptr->str_obj_ref, intref_ptr->value);

	}

	else {
		if (PRINT_LOG)
			printf("ldc:  index(%d) unhandled tag(%d)\n", index,
					common_ptr->tag);
	}
}

void ldc_w() {
	CLASS* current_class_info = current_frame->class_info;

	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	CONSTANT_Common_Info* common_ptr =
			((CONSTANT_Common_Info **) current_class_info->cps)[index];

	if (common_ptr->tag == 3) {
		CONSTANT_Integer_Info* intref_ptr = (CONSTANT_Integer_Info*) common_ptr;
		curr_push(intref_ptr->value);
		if (PRINT_LOG)
			printf("ldc_w:  index(%d) int value(%d)\n", index,
					intref_ptr->value);
	} else if (common_ptr->tag == 8) {
		CONSTANT_String_Info* intref_ptr = (CONSTANT_String_Info*) common_ptr;

		curr_push(intref_ptr->str_obj_ref);
		if (PRINT_LOG)
			printf("ldc_w:  index(%d) String value%s)\n", index,
					intref_ptr->value);
	}

	else {
		if (PRINT_LOG)
			printf("ldc_w:  index(%d) unhandled tag(%d)\n", index,
					common_ptr->tag);
	}
}

void ldc2_w() {
	CLASS* current_class_info = current_frame->class_info;

	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	CONSTANT_Common_Info* common_ptr =
			((CONSTANT_Common_Info **) current_class_info->cps)[index];

	if (common_ptr->tag == 5) {
		CONSTANT_Long_Info* longref_ptr = (CONSTANT_Long_Info*) common_ptr;
		long value = longref_ptr->value;

		//high 32 in stack top,low 32 in stack bottom
		unsigned int low_32 = (value & 0x00000000ffffffff);
		curr_push(low_32);

		unsigned int high_32 = (value & 0xffffffff00000000) >> 32;
		curr_push(high_32);

		if (PRINT_LOG)
			printf("ldc2_w:  index(%d) long value(%ld)\n", index, value);
	} else {

		if (PRINT_LOG)
			printf("ldc2_w:  index(%d) unhandled tag(%d)\n", index,
					common_ptr->tag);
	}
}
void lstore() {
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = byte_codes[(current_frame->pc)++];

	unsigned int high_32 = curr_pop();
	unsigned int low_32 = curr_pop();

//high 32 bit in high index,low 32 bit in low index
	current_frame->local_variable_erea[index] = low_32;
	current_frame->local_variable_erea[index + 1] = high_32;
	if (PRINT_LOG)
		printf("lstore\n");
}

void fstore() {
	if (PRINT_LOG)
		printf("fstore\n");
}

void dstore() {
	if (PRINT_LOG)
		printf("dstore\n");
}
void lstore_0() {
	unsigned int high_32 = curr_pop();
	unsigned int low_32 = curr_pop();

//high 32 bit in high index,low 32 bit in low index
	current_frame->local_variable_erea[0] = low_32;
	current_frame->local_variable_erea[1] = high_32;
	if (PRINT_LOG)
		printf("lstore_0\n");
}
void lstore_1() {
	unsigned int high_32 = curr_pop();
	unsigned int low_32 = curr_pop();

//high 32 bit in high index,low 32 bit in low index
	current_frame->local_variable_erea[1] = low_32;
	current_frame->local_variable_erea[2] = high_32;
	if (PRINT_LOG)
		printf("lstore_1\n");
}
void lstore_2() {
	unsigned int high_32 = curr_pop();
	unsigned int low_32 = curr_pop();

//high 32 bit in high index,low 32 bit in low index
	current_frame->local_variable_erea[2] = low_32;
	current_frame->local_variable_erea[3] = high_32;
	if (PRINT_LOG)
		printf("lstore_2\n");
}
void lstore_3() {
	unsigned int high_32 = curr_pop();
	unsigned int low_32 = curr_pop();

//high 32 bit in high index,low 32 bit in low index
	current_frame->local_variable_erea[3] = low_32;
	current_frame->local_variable_erea[4] = high_32;
	if (PRINT_LOG)
		printf("lstore_3\n");
}

void fstore_0() {
}
void fstore_1() {
}
void fstore_2() {
}
void fstore_3() {
}
void dstore_0() {
}
void dstore_1() {
}
void dstore_2() {
}
void dstore_3() {
}

void aconst_null() {
//null is -1
	curr_push(NULL_POINT);
	if (PRINT_LOG)
		printf("aconst_null\n");
}

void nop() {
	return;
}

unsigned int instanceof_interfaces(char* tar_class_name, CLASS* class_info) {

	unsigned int interfaces_count = class_info->super_interfaces_count;
	if (interfaces_count == 0) {
		return 0;
	} else {
		INTERFACE* interfaces = class_info->super_interfaces;
		for (int i = 0; i < interfaces_count; i++) {

			INTERFACE* interface = interfaces + i;
			CLASS* interface_info = get_interface_info(interface);

			unsigned int super_result = instanceof_interfaces(tar_class_name,
					interface_info);

			//if super interface match the target,just return the result
			if (super_result == 1) {
				return 1;
			} else {
				//if current interface if match
				if (str_equal(tar_class_name, interface->interface_name)) {
					return 1;
				}
			}
		}

		return 0;
	}

}

unsigned int do_instanceof(char* tar_class_name, CLASS* class_info) {

	int result = 0;

	while (1) {
		//if current class match the target
		if (str_equal(class_info->this_class_name, tar_class_name)) {
			result = 1;
			break;
		}
		//if interfaces match the target
		else if (instanceof_interfaces(tar_class_name, class_info)) {
			result = 1;
			break;
		}

		//if super class match the target
		char* super_class_name = class_info->super_class_name;
		if (super_class_name != NULL) {
			class_info = get_super_class(class_info);
		} else {
			break;
		}
	}

	return result;
}

void checkcast() {

	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	CONSTANT_Class_Info* cp_ptr =
			((CONSTANT_Class_Info **) current_class_info->cps)[index];
	char* tar_class_name = cp_ptr->class_name;

	int refer_index = curr_pop();
//If objectref is null, then the operand stack is unchanged
	if (refer_index == NULL_POINT) {
		curr_push(refer_index);
	}

	INSTANCE inst = INSTANCES[refer_index];
	CLASS* sou_class = inst.class_info;

	if (PRINT_LOG)
		printf("checkcast: (%s) -----> (%s)\n", sou_class->this_class_name,
				tar_class_name);

	unsigned int result;
//interface(to do)
	if (sou_class->access_flags & ACC_INTERFACE) {
		if (PRINT_LOG)
			printf("checkcast: source class is interface\n");
		result = instanceof_interfaces(tar_class_name, sou_class);
	}
//array class(to do)
	else if (str_if_startof(sou_class->this_class_name, "[")) {
		if (PRINT_LOG)
			printf("checkcast: source class is array class\n");
	}
//ordinary (nonarray) class
	else {
		if (PRINT_LOG)
			printf("checkcast: source class ordinary (nonarray) class\n");
		result = do_instanceof(tar_class_name, sou_class);
	}

	if (result) {
		curr_push(refer_index);

	} else {
		printf("-------------------ClassCastException----------------------\n");
	}
}

void instanceof() {

	CLASS* current_class_info = current_frame->class_info;
	unsigned char *byte_codes = current_frame->byte_codes;
	unsigned char index = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	CONSTANT_Class_Info* cp_ptr =
			((CONSTANT_Class_Info **) current_class_info->cps)[index];
	char* tar_class_name = cp_ptr->class_name;

	int refer_index = curr_pop();
//If objectref is null,pushes an int result of 0 as an int on the operand stack.
	if (refer_index == NULL_POINT) {
		curr_push(0);
		return;
	}

	INSTANCE inst = INSTANCES[refer_index];
	CLASS* sou_class = inst.class_info;

	if (PRINT_LOG)
		printf("instanceof: (%s) instanceof (%s)\n", sou_class->this_class_name,
				tar_class_name);

	unsigned int result;
//interface(to do)
	if (sou_class->access_flags & ACC_INTERFACE) {
		if (PRINT_LOG)
			printf("instanceof: source class is interface\n");
		result = instanceof_interfaces(tar_class_name, sou_class);
	}
//array class(to do)
	else if (str_if_startof(sou_class->this_class_name, "[")) {
		if (PRINT_LOG)
			printf("instanceof: source class is array class\n");
	}
//ordinary (nonarray) class
	else {
		if (PRINT_LOG)
			printf("instanceof: source class ordinary (nonarray) class\n");
		result = do_instanceof(tar_class_name, sou_class);
	}
	curr_push(result);
}

void monitorenter() {

}

void monitorexit() {

}

void ifnull() {
	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	unsigned int target_pc = current_frame->pc - 3 + offset;

	int refer_index = curr_pop();

	if (PRINT_LOG)
		printf("ifnull target_pc(%d) refer_index(%d)\n", target_pc,
				refer_index);

	if (refer_index == NULL_POINT) {
		current_frame->pc = target_pc;
		return;
	}

}

void ifnonnull() {

	unsigned char *byte_codes = current_frame->byte_codes;
	short offset = (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];
	unsigned int target_pc = current_frame->pc - 3 + offset;

	int refer_index = curr_pop();

	if (PRINT_LOG)
		printf("ifnonnull target_pc(%d) refer_index(%d)\n", target_pc,
				refer_index);

	if (refer_index != NULL_POINT) {
		current_frame->pc = target_pc;
		return;
	}
}

void goto_w() {

	unsigned char *byte_codes = current_frame->byte_codes;
	int offset = (byte_codes[(current_frame->pc)++] << 24)
			| (byte_codes[(current_frame->pc)++] << 16)
			| (byte_codes[(current_frame->pc)++] << 8)
			| byte_codes[(current_frame->pc)++];

	int target_pc = current_frame->pc - 5 + offset;

	if (PRINT_LOG)
		printf("goto_w: target_pc(%d)\n", target_pc);
	current_frame->pc = target_pc;
	return;
}

/**
 * dispatch byte_code to corresponding interpreting method
 */
void bytecode_dispatch(unsigned char byte_code) {

	switch (byte_code) {
	case 0x00: {
		nop();
		break;
	}

	case 0x01: {
		aconst_null();
		break;
	}
	case 0x02: {
		iconst_m1();
		break;
	}
	case 0x03: {
		iconst_0();
		break;
	}
	case 0x04: {
		iconst_1();
		break;
	}
	case 0x05: {
		iconst_2();
		break;
	}
	case 0x06: {
		iconst_3();
		break;
	}
	case 0x07: {
		iconst_4();
		break;
	}
	case 0x08: {
		iconst_5();
		break;
	}
	case 0x09: {
		lconst_0();
		break;
	}
	case 0x0a: {
		lconst_1();
		break;
	}
	case 0x0b: {
		fconst_0();
		break;
	}
	case 0x0c: {
		fconst_1();
		break;
	}
	case 0x0d: {
		fconst_2();
		break;
	}
	case 0x0e: {
		dconst_0();
		break;
	}
	case 0x0f: {
		dconst_1();
		break;
	}
	case 0x10: {
		bipush();
		break;
	}
	case 0x11: {
		sipush();
		break;
	}
	case 0x12: {
		ldc();
		break;
	}
	case 0x13: {
		ldc_w();
		break;
	}
	case 0x14: {
		ldc2_w();
		break;
	}
	case 0x15: {
		iload();
		break;
	}
	case 0x16: {
		lload();
		break;
	}
	case 0x17: {
		fload();
		break;
	}
	case 0x18: {
		dload();
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
	case 0x1e: {
		lload_0();
		break;
	}
	case 0x1f: {
		lload_1();
		break;
	}
	case 0x20: {
		lload_2();
		break;
	}
	case 0x21: {
		lload_3();
		break;
	}

	case 0x22: {
		fload_0();
		break;
	}
	case 0x23: {
		fload_1();
		break;
	}
	case 0x24: {
		fload_2();
		break;
	}
	case 0x25: {
		fload_3();
		break;
	}
	case 0x26: {
		dload_0();
		break;
	}
	case 0x27: {
		dload_1();
		break;
	}
	case 0x28: {
		dload_2();
		break;
	}
	case 0x29: {
		dload_3();
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
	case 0x2e: {
		iaload();
		break;
	}
	case 0x2f: {
		laload();
		break;
	}
	case 0x30: {
		faload();
		break;
	}
	case 0x31: {
		daload();
		break;
	}
	case 0x32: {
		aaload();
		break;
	}
	case 0x34: {
		caload();
		break;
	}
	case 0x36: {
		istore();
		break;
	}
	case 0x37: {
		lstore();
		break;
	}
	case 0x38: {
		fstore();
		break;
	}
	case 0x39: {
		dstore();
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
	case 0x3f: {
		lstore_0();
		break;
	}
	case 0x40: {
		lstore_1();
		break;
	}
	case 0x41: {
		lstore_2();
		break;
	}
	case 0x42: {
		lstore_3();
		break;
	}

	case 0x43: {
		fstore_0();
		break;
	}
	case 0x44: {
		fstore_1();
		break;
	}
	case 0x45: {
		fstore_2();
		break;
	}
	case 0x46: {
		fstore_3();
		break;
	}
	case 0x47: {
		dstore_0();
		break;
	}
	case 0x48: {
		dstore_1();
		break;
	}
	case 0x49: {
		dstore_2();
		break;
	}
	case 0x4a: {
		dstore_3();
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
	case 0x4f: {
		iastore();
		break;
	}

	case 0x51: {
		fastore();
		break;
	}

	case 0x52: {
		dastore();
		break;
	}

	case 0x53: {
		aastore();
		break;
	}

	case 0x54: {
		bastore();
		break;
	}

	case 0x55: {
		castore();
		break;
	}
	case 0x56: {
		sastore();
		break;
	}

		//Stack
	case 0x57: {
		pop();
		break;
	}
	case 0x58: {
		pop2();
		break;
	}
	case 0x59: {
		dup_jvm();
		break;
	}
	case 0x5a: {
		dup_x1();
		break;
	}
	case 0x5b: {
		dup_x2();
		break;
	}
	case 0x5c: {
		dup2_jvm();
		break;
	}

	case 0x5d: {
		dup2_x1();
		break;
	}

	case 0x5e: {
		dup2_x2();
		break;
	}

	case 0x5f: {
		swap();
		break;
	}

		//Math
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
	case 0x84: {
		iinc();
		break;
	}

		//Comparisons
	case 0x94: {
		lcmp();
		break;
	}
	case 0x95: {
		fcmpl();
		break;
	}
	case 0x96: {
		fcmpg();
		break;
	}
	case 0x97: {
		dcmpl();
		break;
	}
	case 0x98: {
		dcmpg();
		break;
	}

	case 0x99: {
		ifeq();
		break;
	}

	case 0x9a: {
		ifne();
		break;
	}
	case 0x9b: {
		iflt();
		break;
	}

	case 0x9c: {
		ifge();
		break;
	}
	case 0x9d: {
		ifgt();
		break;
	}

	case 0x9e: {
		ifle();
		break;
	}
	case 0x9f: {
		if_icmpeq();
		break;
	}

	case 0xa0: {
		if_icmpne();
		break;
	}

	case 0xa1: {
		if_icmplt();
		break;
	}

	case 0xa2: {
		if_icmpge();
		break;
	}

	case 0xa3: {
		if_icmpgt();
		break;
	}

	case 0xa4: {
		if_icmple();
		break;
	}

	case 0xa5: {
		if_acmpeq();
		break;
	}

	case 0xa6: {
		if_acmpne();
		break;
	}

		//Control
	case 0xa7: {
		go_to();
		break;
	}

	case 0xa8: {
		jsr();
		break;
	}

	case 0xa9: {
		ret();
		break;
	}

	case 0xaa: {
		tableswitch();
		break;
	}

	case 0xab: {
		lookupswitch();
		break;
	}

	case 0xac: {
		ireturn();
		break;
	}

	case 0xad: {
		lreturn();
		break;
	}

	case 0xae: {
		freturn();
		break;
	}

	case 0xaf: {
		dreturn();
		break;
	}

	case 0xb0: {
		areturn();
		break;
	}

	case 0xb1: {
		return_void();
		break;
	}

		//References
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
	case 0xba: {
		invokedynamic();
		break;
	}
	case 0xbb: {
		new();
		break;
	}
	case 0xbc: {
		newarray();
		break;
	}
	case 0xbd: {
		anewarray();
		break;
	}
	case 0xbe: {
		arraylength();
		break;
	}
	case 0xbf: {
		athrow();
		break;
	}
	case 0xc0: {
		checkcast();
		break;
	}
	case 0xc1: {
		instanceof();
		break;
	}

	case 0xc2: {
		monitorenter();
		break;
	}
	case 0xc3: {
		monitorexit();
		break;
	}

	case 0xc6: {
		ifnull();
		break;
	}
	case 0xc7: {
		ifnonnull();
		break;
	}

	case 0xc8: {
		goto_w();
		break;
	}
	case 0xc9: {
		jsr_w();
		break;
	}
	default: {
		if (PRINT_LOG)
			printf("	unhandled code byte:%d\n", byte_code);
		break;
	}

	}
}

