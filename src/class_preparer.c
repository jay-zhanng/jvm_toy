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
		printf("static filed name: %s\n", field_info->name);

		unsigned short access_flags = field_info->access_flags;
		unsigned char static_flag = access_flags & 0x08;
		unsigned char final_flag = access_flags & 0x10;

		// final and no final filed
		if (static_flag && !final_flag) {

			char* filed_descriptor = field_info->descriptor;
			printf("static filed descriptor: %s\n", filed_descriptor);
			static_field_value[i].descriptor = field_info->descriptor;

			if (str_cmp(filed_descriptor, "D")
					|| str_cmp(filed_descriptor, "J")) {
				static_field_value[i].field_value_ptr = req_method_area_memo(
						2 * SLOT_SIZE);
			} else {
				static_field_value[i].field_value_ptr = req_method_area_memo(
						SLOT_SIZE);
			}

			if (!str_cmp(filed_descriptor, "L")) {
				// primary type default initial value is 0
				*(static_field_value[i].field_value_ptr) = 0;
			} else {
				// reference type default initial value is null
				*(static_field_value[i].field_value_ptr) =
				NULL;
			}

		}
		//static and final filed
		else if (static_flag && final_flag) {
			printf("static final filed name: %s\n", field_info->name);

			char* filed_descriptor = field_info->descriptor;
			printf("static final filed descriptor: %s\n", filed_descriptor);

			if (str_cmp(filed_descriptor, "D")
					|| str_cmp(filed_descriptor, "J")) {
				static_field_value[i].field_value_ptr = req_method_area_memo(
						2 * SLOT_SIZE);
			} else {
				static_field_value[i].field_value_ptr = req_method_area_memo(
						SLOT_SIZE);
			}

			//use attribute ConstantValue to set the initial value(skip)

		}

		printf("--------------------------------\n");
	}

	printf("--------------class (%s) prepare over --------------\n",
			class_info->this_class_full_name);
}
