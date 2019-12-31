#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "struct.h"
#include "util.h"
#include "class_parser.h"
#include "class_loader.h"
#include "jvm_init.h"
#include "operand_stack.h"

extern const unsigned int SLOT_SIZE;
extern int PRINT_LOG;

/**
 *prepare class,allocate memory and set default init value for static field
 */
void prepare_class(CLASS* class_info) {

	unsigned short static_fields_count = class_info->static_fields_count;
	if (static_fields_count == 0) {
		return;
	}

	FIELD_INFO * static_fileds_info = class_info->static_fileds_info_ptr;

	STATIC_FIELD_VALUE * static_field_value = req_method_area_memo(
			static_fields_count * sizeof(STATIC_FIELD_VALUE));
	class_info->static_field_value_ptr = static_field_value;

	for (int i = 0; i < static_fields_count; i++) {
		FIELD_INFO *field_info = static_fileds_info + i;

		static_field_value[i].name = field_info->name;

		unsigned short access_flags = field_info->access_flags;
		unsigned char static_flag = access_flags & 0x08;
		unsigned char final_flag = access_flags & 0x10;

		// final and no final filed
		if (static_flag && !final_flag) {
			if (PRINT_LOG)
				printf("static no final field name: %s\n", field_info->name);

			char* filed_descriptor = field_info->descriptor;
			if (PRINT_LOG)
				printf("static filed descriptor: %s\n", filed_descriptor);
			static_field_value[i].descriptor = field_info->descriptor;

			if (str_equal(filed_descriptor, "J")
					|| str_equal(filed_descriptor, "J")) {
				static_field_value[i].field_value_ptr = req_method_area_memo(
						2 * SLOT_SIZE);
			} else {
				static_field_value[i].field_value_ptr = req_method_area_memo(
						SLOT_SIZE);
			}

			if (!str_if_startof(filed_descriptor, "L")
					|| !str_if_startof(filed_descriptor, "[")) {
				// primary type default initial value is 0
				*(static_field_value[i].field_value_ptr) = 0;
			} else {
				// reference type default initial value is null(-1)
				*(static_field_value[i].field_value_ptr) = -1;
			}
		}

		//static and final filed
		else if (static_flag && final_flag) {
			if (PRINT_LOG)
				printf("static final filed name: %s\n", field_info->name);

			char* filed_descriptor = field_info->descriptor;
			static_field_value[i].descriptor = field_info->descriptor;
			if (PRINT_LOG)
				printf("static final filed descriptor: %s\n", filed_descriptor);

			if (str_equal(filed_descriptor, "D")
					|| str_equal(filed_descriptor, "J")) {
				static_field_value[i].field_value_ptr = req_method_area_memo(
						2 * SLOT_SIZE);
			} else {
				static_field_value[i].field_value_ptr = req_method_area_memo(
						SLOT_SIZE);
			}

			if (!str_if_startof(filed_descriptor, "L")
					|| !str_if_startof(filed_descriptor, "[")) {
				//static final field(primary type and String) use attribute ConstantValue to set the initial value(skip)

			} else {
				// reference type default initial value is null(-1)
				*(static_field_value[i].field_value_ptr) = -1;
			}
		}

		if (PRINT_LOG)
			printf("--------------------------------\n");
	}

	if (PRINT_LOG)
		printf("--------------class (%s) prepare over --------------\n",
				class_info->this_class_name);
}
