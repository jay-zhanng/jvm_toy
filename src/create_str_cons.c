#include <stdio.h>
#include "struct.h"
#include "util.h"
#include "jvm_init.h"

extern int PRINT_LOG;
extern int* STR_CONS_POOL;
extern int STR_CONS_INDEX;
extern INSTANCE* INSTANCES;
extern int INST_INDEX;

void add_str_cons(int str_ref) {
	STR_CONS_POOL[STR_CONS_INDEX++] = str_ref;
}

/**
 * create a String object and add it to string constant pool
 */
unsigned int create_str_obj(char* value, unsigned int length) {

	if (PRINT_LOG) {
		printf("create string object of str: %s\n", value);

	}
	if (str_equal(value, "hello world")) {
		printf("%s\n", value);
	}

	//check if this string has been in pool
	for (unsigned i = 0; i < STR_CONS_INDEX; i++) {

		int str_ref = STR_CONS_POOL[i];
		INSTANCE str = INSTANCES[str_ref];
		INST_FIELD_VALUE* value_field_value = research_inst_field_value(str,
				"value", "[C");
		int str_value = *(value_field_value->field_value_ptr);

		INSTANCE chars = INSTANCES[str_value];

		INST_FIELD_VALUE* length_field_value = chars.inst_field_value_ptr + 0;
		int tar_length = *length_field_value->field_value_ptr;
		INST_FIELD_VALUE* buffer_field_value = chars.inst_field_value_ptr + 1;
		int *buffer = buffer_field_value->field_value_ptr;

		//check strings if equal
		if (tar_length != length) {
			continue;
		}
		int if_equal = 1;
		for (int j = 0; j < length; j++) {
			int a = buffer[j];
			int b = zero_byte_int(value[j]);
			if (a != b) {
				if_equal = 0;
				break;
			}
		}

		if (if_equal) {
			return str_ref;
		}
	}

//  create char array class
	char* char_array_class_name = "[C";
//	check if this class has bean loaded
	CLASS* array_class_info = search_class_info(char_array_class_name);
	if (array_class_info == NULL) {
		array_class_info = req_class();
		array_class_info->total_fields_count = 2;
		array_class_info->super_class_name = "java.lang.Object";
		array_class_info->this_class_name = char_array_class_name;
	}

//create char array object
	INSTANCE * array_inst_ptr = req_inst();
	unsigned int array_inst_ref = INST_INDEX - 1;
	array_inst_ptr->class_info = array_class_info;

	INST_FIELD_VALUE* array_fields_ptr = (INST_FIELD_VALUE*) req_heap_area_memo(
			2 * sizeof(INST_FIELD_VALUE));
	array_inst_ptr->inst_field_value_ptr = array_fields_ptr;

//set length field
	INST_FIELD_VALUE* length_field = array_fields_ptr + 0;
	length_field->name = "length";
	length_field->descriptor = "I";

	int* length_value_ptr = (int*) req_heap_area_memo(sizeof(int));
	*length_value_ptr = length;
	length_field->field_value_ptr = length_value_ptr;

//set buffer field
	INST_FIELD_VALUE* buffer_field = array_fields_ptr + 1;
	buffer_field->name = "buffer";
	buffer_field->descriptor = "C";

	int* buffer_value_ptr = (int*) req_heap_area_memo(length * sizeof(int));
	buffer_field->field_value_ptr = buffer_value_ptr;
	for (unsigned i = 0; i < length; i++) {
		buffer_value_ptr[i] = signed_byte_int(value[i]);
	}

//load String class
	CLASS* str_class_info = load_class("java/lang/String");
//new String object
//get inst filed num
	unsigned short str_total_fields_num = get_inst_filed_total_num(
			str_class_info);
	str_class_info->total_fields_count = str_total_fields_num;

//allocate memory
	INSTANCE* str_inst_ptr = req_inst();
	unsigned int str_inst_ref = INST_INDEX - 1;

	INST_FIELD_VALUE* str_fields_ptr = (INST_FIELD_VALUE*) req_heap_area_memo(
			str_total_fields_num * sizeof(INST_FIELD_VALUE));
	str_inst_ptr->class_info = str_class_info;
	str_inst_ptr->inst_field_value_ptr = str_fields_ptr;
//set default value for inst field
	init_inst_fields(str_class_info, str_total_fields_num, str_fields_ptr);
//set String value
	INST_FIELD_VALUE* value_field = research_inst_field_value(*str_inst_ptr,
			"value", "[C");
	*(value_field->field_value_ptr) = array_inst_ref;

//set String hash
	int hash_code = 0;
	for (int i = 0; i < length; i++) {
		hash_code = 31 * hash_code + value[i];
	}
	INST_FIELD_VALUE* hash_field = research_inst_field_value(*str_inst_ptr,
			"hash", "I");
	*(hash_field->field_value_ptr) = hash_code;

// add string to pool
	add_str_cons(str_inst_ref);
	return str_inst_ref;
}

void create_str_cons(CLASS* class_info_ptr) {

	if (PRINT_LOG)
		printf(
				"--------------class (%s) create string constant pool start--------------\n",
				class_info_ptr->this_class_name);

	unsigned short constant_pool_size = class_info_ptr->constant_pool_size;
	void* cps = class_info_ptr->cps;
	for (unsigned int i = 1; i < constant_pool_size; i++) {

		CONSTANT_Common_Info* cp = ((CONSTANT_Common_Info **) cps)[i];
		if (cp == NULL) {
			continue;
		}

		switch (cp->tag) {
		case 8: {
			CONSTANT_String_Info *str_info = (CONSTANT_String_Info *) cp;
			char* value = str_info->value;
			unsigned int length = str_info->length;

			unsigned int str_obj_ref = create_str_obj(value, length);
			str_info->str_obj_ref = str_obj_ref;
			break;
		}
		}
	}
}

