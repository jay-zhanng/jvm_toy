#include <stdio.h>
#include <stdlib.h>
#include "struct.h"
#include "util.h"
#include "class_parser.h"
#include "class_loader.h"
#include "jvm_init.h"
#include "create_str_cons.h"

extern int PRINT_LOG;

/**
 * parse the content of the class file
 */
void load(unsigned char *ptr, CLASS *class_info) {

	//magic
	class_info->magic = htoni(*(unsigned int *) ptr);
	ptr += sizeof(class_info->magic);
	if (PRINT_LOG)
		printf("magic: %X\n", class_info->magic);

	//minor_version
	class_info->minor_version = htons(*(unsigned short *) ptr);
	ptr += sizeof(class_info->minor_version);
	if (PRINT_LOG)
		printf("minor_version: 0x%X\n", class_info->minor_version);

	//major_version
	class_info->major_version = htons(*(unsigned short *) ptr);
	ptr += sizeof(class_info->major_version);
	if (PRINT_LOG)
		printf("major_version: 0x%X\n", class_info->major_version);

	//constant_pool_count
	class_info->constant_pool_size = htons(*(unsigned short*) ptr);
	ptr += sizeof(class_info->constant_pool_size);
	if (PRINT_LOG)
		printf("constant_pool_count: %d\n", class_info->constant_pool_size);

	//cp_info
	//point array,every array element is the point of constant pool table
	void *cps = req_method_area_memo(
			sizeof(void *) * class_info->constant_pool_size);
	if (PRINT_LOG)
		printf("--------------constant pool info start--------------\n");
	int cp_length = parse_constant_pool(cps, ptr,
			class_info->constant_pool_size);
	ptr += cp_length;
	class_info->cps = cps;

	if (PRINT_LOG)
		printf("constant pool total size: %d\n", cp_length);
	if (PRINT_LOG)
		printf("--------------constant pool info end--------------\n");

	//access_flags
	class_info->access_flags = htons(*(unsigned short *) ptr);
	ptr += sizeof(class_info->access_flags);
	if (PRINT_LOG)
		printf("class_access_flags: %X\n", class_info->access_flags);

	//this class
	unsigned short this_class_index = htons(*(unsigned short *) ptr);
	CONSTANT_Class_Info* this_class_name_info =
			((CONSTANT_Class_Info **) cps)[this_class_index];
	class_info->this_class_name = this_class_name_info->class_name;
	ptr += sizeof(this_class_index);
	if (PRINT_LOG)
		printf("this_class_name: %s\n", class_info->this_class_name);

	//super class
	unsigned short super_class_index = htons(*(unsigned short *) ptr);
	//java.lang.Object do Not hava super class,so skip
	if (super_class_index != 0) {
		CONSTANT_Class_Info* super_class_name_info =
				((CONSTANT_Class_Info **) cps)[super_class_index];
		class_info->super_class_name = super_class_name_info->class_name;
		if (PRINT_LOG)
			printf("super_class_name: %s\n", class_info->super_class_name);
	}
	ptr += sizeof(super_class_index);

	//interface
	class_info->super_interfaces_count = htons(*(unsigned short *) ptr);
	ptr += sizeof(class_info->super_interfaces_count);
	if (PRINT_LOG)
		printf("interfaces_count: %X\n", class_info->super_interfaces_count);

	INTERFACE* interfaces = req_method_area_memo(
			class_info->super_interfaces_count * sizeof(INTERFACE));
	class_info->super_interfaces = interfaces;
	for (unsigned int i = 0; i < class_info->super_interfaces_count; i++) {
		INTERFACE* interface = interfaces + i;
		unsigned short interface_index = htons(*(unsigned short *) ptr);
		ptr += sizeof(interface_index);

		CONSTANT_Class_Info* interface_info =
				((CONSTANT_Class_Info **) cps)[interface_index];

		interface->interface_name = interface_info->class_name;
		if (PRINT_LOG)
			printf("interface name(%d):%s\n", i, interface_info->class_name);
	}

//field
	if (PRINT_LOG)
		printf("--------------field info start--------------\n");
	unsigned int field_total_size = parse_fields(class_info, ptr);
	ptr += field_total_size;
	if (PRINT_LOG)
		printf("--------------field info end--------------\n");

//method
	class_info->methods_count = htons(*(unsigned short *) ptr);
	ptr += sizeof(class_info->methods_count);
	if (PRINT_LOG)
		printf("methods_count: %d\n", class_info->methods_count);

	if (PRINT_LOG)
		printf("--------------method info start--------------\n");
	METHOD_INFO *methods = (METHOD_INFO *) req_method_area_memo(
			sizeof(METHOD_INFO) * class_info->methods_count);
	class_info->all_methods_ptr = methods;
	unsigned int method_total_size = parse_methods(class_info, methods, ptr,
			class_info->methods_count);
	ptr += method_total_size;
	if (PRINT_LOG)
		printf("--------------method info end----------------\n");

//SourceFile
//	unsigned short attribute_name_index = htons(*(unsigned short *) ptr);
//	ptr += sizeof(attribute_name_index);
//	char *attribute_name =
//			((CONSTANT_Utf8_Info **) cps)[attribute_name_index]->chars;
//	if (PRINT_LOG)printf("class attribute name: %s\n", attribute_name);
//
//	unsigned int attribute_length = htons(*(unsigned int*) ptr);
//	ptr += sizeof(attribute_length);
//
//	unsigned short sourcefile_index = htons(*(unsigned short *) ptr);
//	ptr += sizeof(sourcefile_index);
//	char *sourcefile = ((CONSTANT_Utf8_Info **) cps)[sourcefile_index]->chars;
//	if (PRINT_LOG)printf("SourceFile:%s\n", sourcefile);

	//create String instance and add it to string constant pool
	create_str_cons(class_info);

	if (PRINT_LOG)
		printf("--------------class (%s) pares over-------------\n",
				class_info->this_class_name);
}

unsigned int parse_fields(CLASS *class_info, unsigned char *ptr) {

	unsigned int total_num = 0;

	unsigned short static_fields_count = 0;
	unsigned short inst_fields_count = 0;

	unsigned short fields_count = htons(*(unsigned short *) ptr);
	ptr += sizeof(fields_count);
	total_num += sizeof(fields_count);
	if (PRINT_LOG)
		printf("fields_count: %X\n", fields_count);
	if (fields_count == 0) {
		return total_num;
	}

	FIELD_INFO fields[fields_count];
	for (unsigned int i = 0; i < fields_count; i++) {

		if (PRINT_LOG)
			printf("--------------%d--------------\n", i);

		//access_flags
		fields[i].access_flags = htons(*(unsigned short *) ptr);
		ptr += sizeof(fields[i].access_flags);
		total_num += sizeof(fields[i].access_flags);
		if (PRINT_LOG)
			printf("access_flags: %d\n", fields[i].access_flags);

		//static
		if ((fields[i].access_flags & 0x08)) {
			static_fields_count++;
		} else if (!(fields[i].access_flags & 0x08)) {
			inst_fields_count++;
		}

		//name_index
		unsigned short name_index = htons(*(unsigned short *) ptr);
		ptr += sizeof(name_index);
		total_num += sizeof(name_index);
		fields[i].name = find_utf8_info(class_info->cps, name_index);
		if (PRINT_LOG)
			printf("field name: %s\n", fields[i].name);

		//descriptor_index
		unsigned short descriptor_index = htons(*(unsigned short *) ptr);
		ptr += sizeof(descriptor_index);
		total_num += sizeof(descriptor_index);
		fields[i].descriptor = find_utf8_info(class_info->cps,
				descriptor_index);
		if (PRINT_LOG)
			printf("field descriptor: %s\n", fields[i].descriptor);

		char* filed_descriptor = fields[i].descriptor;
		if (str_equal(filed_descriptor, "D") || str_equal(filed_descriptor, "J")
				|| str_equal(filed_descriptor, "L")) {

		} else {

		}

		//attributes
		unsigned short attributes_count = htons(*(unsigned short *) ptr);
		ptr += sizeof(attributes_count);
		total_num += sizeof(attributes_count);
		if (PRINT_LOG)
			printf("attributes_count: %d\n", attributes_count);

		void *attr_infos = req_method_area_memo(
				sizeof(void *) * attributes_count);
		fields[i].attribute_infos = attr_infos;

		for (unsigned int i = 0; i < attributes_count; i++) {

			//attribute_name_index
			unsigned short attribute_name_index = htons(
					*(unsigned short *) ptr);
			ptr += sizeof(attribute_name_index);
			total_num += sizeof(attribute_name_index);
			if (PRINT_LOG)
				printf("attribute_name_index: %d\n", attribute_name_index);

			unsigned char cp_tag =
					((CONSTANT_Common_Info **) class_info->cps)[attribute_name_index]->tag;
			if (cp_tag != 1) {
				if (PRINT_LOG)
					printf(
							"parse  attribute occur error,the tag of attribute name index is NOT Utf8!\n");
				return 0;
			}
			char *attribute_name =
					((CONSTANT_Utf8_Info **) class_info->cps)[attribute_name_index]->chars;
			if (PRINT_LOG)
				printf("attribute_name: %s\n", attribute_name);

			//attribute_length
			unsigned int attribute_length = htoni(*(unsigned int *) ptr);
			ptr += sizeof(attribute_length);
			total_num += sizeof(attribute_length);
			if (PRINT_LOG)
				printf("attribute_length %d\n", attribute_length);

			//handle ConstantValue
			if (str_equal("ConstantValue", attribute_name)) {
				//constantvalue_index
				unsigned short constantvalue_index = htons(
						*(unsigned short*) ptr);
				ptr += sizeof(constantvalue_index);
				total_num += sizeof(constantvalue_index);
				if (PRINT_LOG)
					printf("constantvalue_index %d\n", constantvalue_index);

				unsigned char cps_tag =
						((CONSTANT_Common_Info **) class_info->cps)[constantvalue_index]->tag;
				if (PRINT_LOG)
					printf("constant pool item tag %d\n", cps_tag);

				switch (cps_tag) {
				//CONSTANT_Integer_Info
				case 3: {
					int constant_value =
							((CONSTANT_Integer_Info **) class_info->cps)[constantvalue_index]->value;
					if (PRINT_LOG)
						printf("ConstantValue type is integer,value is %d\n",
								constant_value);
					break;
				}
				}
			}
			// other just skip
			else {
				ptr += attribute_length;
				total_num += attribute_length;
			}

		}
	}
	if (PRINT_LOG)
		printf("------------------------------\n");

	if (PRINT_LOG)
		printf("static field count: %d\n", static_fields_count);
	if (PRINT_LOG)
		printf("inst field count: %d\n", inst_fields_count);
	FIELD_INFO* static_field = req_method_area_memo(
			static_fields_count * sizeof(FIELD_INFO));
	FIELD_INFO* inst_field = req_method_area_memo(
			inst_fields_count * sizeof(FIELD_INFO));

	unsigned int static_index = 0;
	unsigned int inst_index = 0;

	for (unsigned int i = 0; i < fields_count; i++) {

		FIELD_INFO field = fields[i];
		unsigned short access_flags = field.access_flags;
		//static
		if (access_flags & 0x08) {
			static_field[static_index++] = field;
		} else if (!(access_flags & 0x08)) {
			inst_field[inst_index++] = field;
		}
	}

	class_info->static_fields_count = static_fields_count;
	class_info->static_fileds_info_ptr = static_field;

	class_info->inst_fields_count = inst_fields_count;
	class_info->inst_fileds_info_ptr = inst_field;

	return total_num;
}

unsigned int parse_methods(CLASS* class_info, METHOD_INFO *filed_methods,
		unsigned char *ptr, unsigned int methods_count) {

	void *cps = class_info->cps;

	unsigned int total_num = 0;

	for (unsigned int i = 0; i < methods_count; i++) {

		if (PRINT_LOG)
			printf("--------------method of class:%s index:%d--------------\n",
					class_info->this_class_name, i);

		METHOD_INFO *method = (filed_methods + i);

		method->class_info = class_info;

		method->access_flags = htons(*(unsigned short *) ptr);
		ptr += sizeof(method->access_flags);
		total_num += sizeof(method->access_flags);
		if (PRINT_LOG)
			printf("access_flags: %d\n", method->access_flags);

		method->name_index = htons(*(unsigned short *) ptr);
		ptr += sizeof(method->name_index);
		total_num += sizeof(method->name_index);
		method->name = find_utf8_info(cps, method->name_index);
		if (PRINT_LOG)
			printf("method name: %s\n", method->name);

		method->descriptor_index = htons(*(unsigned short *) ptr);
		ptr += sizeof(method->descriptor_index);
		total_num += sizeof(method->descriptor_index);
		method->descriptor = find_utf8_info(cps, method->descriptor_index);
		if (PRINT_LOG)
			printf("method descriptor: %s\n", method->descriptor);

		method->attributes_count = htons(*(unsigned short *) ptr);
		ptr += sizeof(method->attributes_count);
		total_num += sizeof(method->attributes_count);
		if (PRINT_LOG)
			printf("method attributes count: %d\n", method->attributes_count);

		void *attr_infos = req_method_area_memo(
				sizeof(void *) * method->attributes_count);
		method->attribute_infos = attr_infos;

		for (unsigned int i = 0; i < method->attributes_count; i++) {

			if (PRINT_LOG)
				printf(
						"--------------attribute in method:%s of class:%s index:%d--------------\n",
						method->name, class_info->this_class_name, i);

			unsigned short attribute_name_index = htons(
					*(unsigned short *) ptr);
			ptr += sizeof(attribute_name_index);
			total_num += sizeof(attribute_name_index);
			if (PRINT_LOG)
				printf("method attribute name index %d\n",
						attribute_name_index);

			char *attribute_name =
					((CONSTANT_Utf8_Info **) cps)[attribute_name_index]->chars;
			if (PRINT_LOG)
				printf("method attribute name: %s\n", attribute_name);

			//handle Code attribute
			if (str_equal("Code", attribute_name)) {
				CODE_ATTRIBUTE *code_attribute = req_method_area_memo(
						sizeof(CODE_ATTRIBUTE));

				method->code_attr_ptr = code_attribute;

				/*attribute_length,the length in bytes of the Code attribute excluding the initial six
				 bytes that contain the attribute_name_index and attribute_length items.*/
				unsigned int attribute_length = htoni(*(unsigned int *) ptr);
				ptr += sizeof(attribute_length);
				total_num += sizeof(attribute_length);
				if (PRINT_LOG)
					printf("Code attribute length: %d\n", attribute_length);

				//max_stack
				unsigned short max_stack = htons(*(unsigned short *) ptr);
				ptr += sizeof(max_stack);
				total_num += sizeof(max_stack);
				code_attribute->max_stack = max_stack;
				if (PRINT_LOG)
					printf("max_stack %d\n", max_stack);

				//max_locals
				unsigned short max_locals = htons(*(unsigned short *) ptr);
				ptr += sizeof(max_locals);
				total_num += sizeof(max_locals);
				code_attribute->max_locals = max_locals;
				if (PRINT_LOG)
					printf("max_stack %d\n", max_locals);

				//code_length,the length in bytes of the bytecode
				unsigned int code_length = htoni(*(unsigned int *) ptr);
				ptr += sizeof(code_length);
				total_num += sizeof(code_length);
				code_attribute->code_length = code_length;
				if (PRINT_LOG)
					printf("code_length: %d\n", code_length);

				//bytecode
				unsigned char* bytecodes = req_method_area_memo(code_length);
				memory_copy(ptr, bytecodes, code_length);
				ptr += code_length;
				total_num += code_length;
				code_attribute->bytecode = bytecodes;

				//exception table
				unsigned short exception_table_length = htons(
						*(unsigned short *) ptr);
				ptr += sizeof(exception_table_length);
				total_num += sizeof(exception_table_length);
				code_attribute->exception_table_length = exception_table_length;
				if (PRINT_LOG)
					printf("exception table length %d\n",
							exception_table_length);

				Exceptions_ATTRIBUTE* exceptions =
						(Exceptions_ATTRIBUTE*) malloc(
								sizeof(Exceptions_ATTRIBUTE*)
										* exception_table_length);
				code_attribute->exceptions = exceptions;
				for (unsigned int i = 0; i < exception_table_length; i++) {
					Exceptions_ATTRIBUTE* exception = exceptions + i;

					unsigned short start_pc = htons(*(unsigned short *) ptr);
					ptr += sizeof(start_pc);
					total_num += sizeof(start_pc);
					exception->start_pc = start_pc;

					unsigned short end_pc = htons(*(unsigned short *) ptr);
					ptr += sizeof(end_pc);
					total_num += sizeof(end_pc);
					exception->end_pc = end_pc;

					unsigned short handler_pc = htons(*(unsigned short *) ptr);
					ptr += sizeof(handler_pc);
					total_num += sizeof(handler_pc);
					exception->handler_pc = handler_pc;

					unsigned short catch_type_index = htons(
							*(unsigned short *) ptr);
					ptr += sizeof(catch_type_index);
					total_num += sizeof(catch_type_index);
					char *catch_type;
					if (catch_type_index == 0) {
						catch_type = "any";
					} else {
						catch_type =
								((CONSTANT_Utf8_Info **) cps)[catch_type_index]->chars;
						exception->catch_type = catch_type;
					}
					if (PRINT_LOG)
						printf("exception table:\n");
					if (PRINT_LOG)
						printf("from   to   target  type\n");
					if (PRINT_LOG)
						printf("%d      %d   %d	    %s\n", start_pc, end_pc,
								handler_pc, catch_type);

				}

				//attributes_count
				unsigned short attributes_count = htons(
						*(unsigned short *) ptr);
				ptr += sizeof(attributes_count);
				total_num += sizeof(attributes_count);
				if (PRINT_LOG)
					printf("Code attributes count %d\n", attributes_count);

				//handle attribute in Code attribute (all just skip)
				for (unsigned int i = 0; i < attributes_count; i++) {

					unsigned short attribute_name_index = htons(
							*(unsigned short *) ptr);
					ptr += sizeof(attribute_name_index);
					total_num += sizeof(attribute_name_index);
					if (PRINT_LOG)
						printf("attribute in Code name index %d\n",
								attribute_name_index);

					char *attribute_name =
							((CONSTANT_Utf8_Info **) cps)[attribute_name_index]->chars;
					if (PRINT_LOG)
						printf("attribute in Code name: %s\n", attribute_name);
					unsigned int attribute_length = htoni(
							*(unsigned int *) ptr);
					ptr += sizeof(attribute_length);
					total_num += sizeof(attribute_length);
					if (PRINT_LOG)
						printf("attribute in Code length: %d\n",
								attribute_length);

					//just skip
					ptr += attribute_length;
					total_num += attribute_length;
				}
			}

			//others just skip
			else {
				unsigned int attribute_length = htoni(*(unsigned int *) ptr);
				ptr += sizeof(attribute_length);
				total_num += sizeof(attribute_length);
				if (PRINT_LOG)
					printf("just skip this attribute\n");

				//just skip
				ptr += attribute_length;
				total_num += attribute_length;
			}
		}

		if (str_equal(method->name, "<clinit>")) {
			class_info->clinit_methods_ptr = method;
		} else if (str_equal(method->name, "main")) {
			class_info->main_methods_ptr = method;
		}
	}
	return total_num;
}

unsigned int parse_constant_pool(void *cps, unsigned char * ptr,
		unsigned int constant_pool_size) {

	unsigned int cps_total_byte_num = 0;

	for (unsigned int i = 1; i < constant_pool_size; i++) {
		unsigned char tag = *(unsigned char *) ptr;
		ptr += sizeof(tag);
		cps_total_byte_num += sizeof(tag);

		switch (tag) {
		//CONSTANT_Utf8
		case 1: {
			//tag
			CONSTANT_Utf8_Info *utf8_info =
					(CONSTANT_Utf8_Info *) req_method_area_memo(
							sizeof(CONSTANT_Utf8_Info));
			utf8_info->tag = tag;

			//length
			utf8_info->length = htons(*(unsigned short *) ptr);
			ptr += sizeof(utf8_info->length);
			cps_total_byte_num += sizeof(utf8_info->length);

			//content
			char *chars = (char *) req_method_area_memo(utf8_info->length + 1);
			memory_copy(ptr, (unsigned char *) chars, utf8_info->length);
			ptr += utf8_info->length;
			cps_total_byte_num += utf8_info->length;

			//add string end flag
			chars[utf8_info->length] = 0;
			utf8_info->chars = chars;
			((CONSTANT_Utf8_Info **) cps)[i] = utf8_info;

			if (PRINT_LOG)
				printf("#%d Utf8: %s\n", i,
						((CONSTANT_Utf8_Info **) cps)[i]->chars);

			break;
		}

			//CONSTANT_Integer
		case 3: {
			//tag
			CONSTANT_Integer_Info *int_info =
					(CONSTANT_Integer_Info *) req_method_area_memo(
							sizeof(CONSTANT_Integer_Info));
			int_info->tag = tag;

			int_info->value = htoni(*(int *) ptr);
			ptr += sizeof(int_info->value);
			cps_total_byte_num += sizeof(int_info->value);

			((CONSTANT_Integer_Info **) cps)[i] = int_info;
			if (PRINT_LOG)
				printf("#%d int: %d\n", i,
						((CONSTANT_Integer_Info **) cps)[i]->value);

			break;
		}
			//CONSTANT_CONSTANT_Float
		case 4: {
			//tag
			CONSTANT_Float_Info *float_info =
					(CONSTANT_Float_Info *) req_method_area_memo(
							sizeof(CONSTANT_Float_Info));
			float_info->tag = tag;

			//just read four byte
			float_info->value = *(float *) ptr;
			ptr += sizeof(float_info->value);
			cps_total_byte_num += sizeof(float_info->value);

			((CONSTANT_Float_Info **) cps)[i] = float_info;
			if (PRINT_LOG)
				printf("#%d float: #%f\n", i,
						((CONSTANT_Float_Info **) cps)[i]->value);

			break;
		}

			//CONSTANT_Double
		case 6: {
			//tag
			CONSTANT_Double_Info *double_info =
					(CONSTANT_Double_Info *) req_method_area_memo(
							sizeof(CONSTANT_Double_Info));
			double_info->tag = tag;

			//just read eight byte
			double_info->value = *(double *) ptr;
			ptr += sizeof(double_info->value);
			cps_total_byte_num += sizeof(double_info->value);

			((CONSTANT_Double_Info **) cps)[i] = double_info;
			if (PRINT_LOG)
				printf("#%d int: #%f\n", i,
						((CONSTANT_Double_Info **) cps)[i]->value);

			break;
		}

			//CONSTANT_Long
		case 5: {
			//tag
			CONSTANT_Long_Info *long_info =
					(CONSTANT_Long_Info *) req_method_area_memo(
							sizeof(CONSTANT_Long_Info));
			long_info->tag = tag;

			long_info->value = htonl(*(long *) ptr);
			ptr += sizeof(long_info->value);
			cps_total_byte_num += sizeof(long_info->value);

			((CONSTANT_Long_Info **) cps)[i] = long_info;
			if (PRINT_LOG)
				printf("#%d long: %ld\n", i,
						((CONSTANT_Long_Info **) cps)[i]->value);

			//there will skip next item for CONSTANT_Long,why?
			i++;
			break;
		}

			//CONSTANT_Class
		case 7: {
			CONSTANT_Class_Info *class_info =
					(CONSTANT_Class_Info *) req_method_area_memo(
							sizeof(CONSTANT_Class_Info));

			class_info->tag = tag;

			class_info->index = htons(*(unsigned short *) ptr);
			ptr += sizeof(class_info->index);
			cps_total_byte_num += sizeof(class_info->index);

			((CONSTANT_Class_Info **) cps)[i] = class_info;
			if (PRINT_LOG)
				printf("#%d Class: #%d\n", i,
						((CONSTANT_Class_Info **) cps)[i]->index);
			break;
		}
			//CONSTANT_String
		case 8: {
			CONSTANT_String_Info *str_info =
					(CONSTANT_String_Info *) req_method_area_memo(
							sizeof(CONSTANT_String_Info));

			str_info->tag = tag;

			str_info->index = htons(*(unsigned short *) ptr);
			ptr += sizeof(str_info->index);
			cps_total_byte_num += sizeof(str_info->index);
			((CONSTANT_String_Info **) cps)[i] = str_info;
			if (PRINT_LOG)
				printf("#%d String: #%d\n", i,
						((CONSTANT_String_Info **) cps)[i]->index);
			break;
		}
			//CONSTANT_Fieldref
		case 9: {
			CONSTANT_Fieldref_Info *fieldref_info =
					(CONSTANT_Fieldref_Info *) req_method_area_memo(
							sizeof(CONSTANT_Fieldref_Info));

			fieldref_info->tag = tag;

			fieldref_info->class_index = htons(*(unsigned short *) ptr);
			ptr += sizeof(fieldref_info->class_index);
			cps_total_byte_num += sizeof(fieldref_info->class_index);

			fieldref_info->name_and_type_index = htons(*(unsigned short *) ptr);
			ptr += sizeof(fieldref_info->name_and_type_index);
			cps_total_byte_num += sizeof(fieldref_info->name_and_type_index);
			((CONSTANT_Fieldref_Info **) cps)[i] = fieldref_info;
			if (PRINT_LOG)
				printf("#%d Fieldref: #%d.#%d\n", i,
						((CONSTANT_Fieldref_Info **) cps)[i]->class_index,
						((CONSTANT_Fieldref_Info **) cps)[i]->name_and_type_index);
			break;
		}

			//CONSTANT_Methodref
		case 10: {
			CONSTANT_Methodref_Info *methoddref_info =
					(CONSTANT_Methodref_Info *) req_method_area_memo(
							sizeof(CONSTANT_Methodref_Info));

			methoddref_info->tag = tag;

			methoddref_info->class_index = htons(*(unsigned short *) ptr);
			ptr += sizeof(methoddref_info->class_index);
			cps_total_byte_num += sizeof(methoddref_info->class_index);

			methoddref_info->name_and_type_index = htons(
					*(unsigned short *) ptr);
			ptr += sizeof(methoddref_info->name_and_type_index);
			cps_total_byte_num += sizeof(methoddref_info->name_and_type_index);
			((CONSTANT_Methodref_Info **) cps)[i] = methoddref_info;

			if (PRINT_LOG)
				printf("#%d Methodref: #%d.#%d\n", i,
						((CONSTANT_Methodref_Info **) cps)[i]->class_index,
						((CONSTANT_Methodref_Info **) cps)[i]->name_and_type_index);
			break;
		}

			//CONSTANT_InterfaceMethodref_info
		case 11: {
			CONSTANT_InterfaceMethodref_info *methoddref_info =
					(CONSTANT_InterfaceMethodref_info *) req_method_area_memo(
							sizeof(CONSTANT_InterfaceMethodref_info));

			methoddref_info->tag = tag;

			methoddref_info->class_index = htons(*(unsigned short *) ptr);
			ptr += sizeof(methoddref_info->class_index);
			cps_total_byte_num += sizeof(methoddref_info->class_index);

			methoddref_info->name_and_type_index = htons(
					*(unsigned short *) ptr);
			ptr += sizeof(methoddref_info->name_and_type_index);
			cps_total_byte_num += sizeof(methoddref_info->name_and_type_index);
			((CONSTANT_InterfaceMethodref_info **) cps)[i] = methoddref_info;

			if (PRINT_LOG)
				printf("#%d InterfaceMethodref: #%d.#%d\n", i,
						((CONSTANT_InterfaceMethodref_info **) cps)[i]->class_index,
						((CONSTANT_InterfaceMethodref_info **) cps)[i]->name_and_type_index);
			break;
		}

			//CONSTANT_NameAndType
		case 12: {
			CONSTANT_NameAndType_Info *name_and_type_info =
					(CONSTANT_NameAndType_Info *) req_method_area_memo(
							sizeof(CONSTANT_Fieldref_Info));

			name_and_type_info->tag = tag;

			name_and_type_info->name_index = htons(*(unsigned short *) ptr);
			ptr += sizeof(name_and_type_info->name_index);
			cps_total_byte_num += sizeof(name_and_type_info->name_index);

			name_and_type_info->descriptor_index = htons(
					*(unsigned short *) ptr);
			ptr += sizeof(name_and_type_info->descriptor_index);
			cps_total_byte_num += sizeof(name_and_type_info->descriptor_index);
			((CONSTANT_NameAndType_Info **) cps)[i] = name_and_type_info;

			if (PRINT_LOG)
				printf("#%d NameAndType: #%d.#%d\n", i,
						((CONSTANT_NameAndType_Info **) cps)[i]->name_index,
						((CONSTANT_NameAndType_Info **) cps)[i]->descriptor_index);

			break;
		}
		}
	}

	for (unsigned int i = 1; i < constant_pool_size; i++) {

		CONSTANT_Common_Info* cp = ((CONSTANT_Common_Info **) cps)[i];
		if (cp == NULL) {
			continue;
		}

		unsigned char tag = cp->tag;

		switch (tag) {

		case 7: {
			CONSTANT_Class_Info *class_info = (CONSTANT_Class_Info *) cp;
			class_info->class_name =
					((CONSTANT_Utf8_Info **) cps)[class_info->index]->chars;
			break;
		}
		case 8: {
			CONSTANT_String_Info *str_info = (CONSTANT_String_Info *) cp;
			char* value = ((CONSTANT_Utf8_Info **) cps)[str_info->index]->chars;
			unsigned int length =
					((CONSTANT_Utf8_Info **) cps)[str_info->index]->length;

			str_info->value = value;
			str_info->length = length;

			if (PRINT_LOG)
				printf("#%d String is: %s\n", i, str_info->value);
			break;
		}
		case 9: {
			CONSTANT_Fieldref_Info *fieldref_info =
					(CONSTANT_Fieldref_Info *) cp;

			unsigned short class_index = fieldref_info->class_index;
			CONSTANT_Class_Info* class_info =
					((CONSTANT_Class_Info**) cps)[class_index];
			fieldref_info->class_name =
					((CONSTANT_Utf8_Info **) cps)[class_info->index]->chars;

			unsigned short name_and_type_index =
					fieldref_info->name_and_type_index;

			CONSTANT_NameAndType_Info * name_type_info =
					((CONSTANT_NameAndType_Info **) cps)[name_and_type_index];

			fieldref_info->filed_name =
					((CONSTANT_Utf8_Info **) cps)[name_type_info->name_index]->chars;

			fieldref_info->filed_descriptor =
					((CONSTANT_Utf8_Info **) cps)[name_type_info->descriptor_index]->chars;

			break;
		}

		case 10: {
			CONSTANT_Methodref_Info *methoddref_info =
					(CONSTANT_Methodref_Info *) cp;

			unsigned short class_index = methoddref_info->class_index;
			CONSTANT_Class_Info* class_info =
					((CONSTANT_Class_Info**) cps)[class_index];
			methoddref_info->class_name =
					((CONSTANT_Utf8_Info **) cps)[class_info->index]->chars;

			unsigned short name_and_type_index =
					methoddref_info->name_and_type_index;

			CONSTANT_NameAndType_Info * name_type_info =
					((CONSTANT_NameAndType_Info **) cps)[name_and_type_index];

			methoddref_info->method_name =
					((CONSTANT_Utf8_Info **) cps)[name_type_info->name_index]->chars;

			methoddref_info->method_descriptor =
					((CONSTANT_Utf8_Info **) cps)[name_type_info->descriptor_index]->chars;

			break;
		}
		case 11: {
			CONSTANT_InterfaceMethodref_info *methoddref_info =
					(CONSTANT_InterfaceMethodref_info *) cp;

			unsigned short class_index = methoddref_info->class_index;
			CONSTANT_Class_Info* class_info =
					((CONSTANT_Class_Info**) cps)[class_index];
			methoddref_info->class_name =
					((CONSTANT_Utf8_Info **) cps)[class_info->index]->chars;

			unsigned short name_and_type_index =
					methoddref_info->name_and_type_index;

			CONSTANT_NameAndType_Info * name_type_info =
					((CONSTANT_NameAndType_Info **) cps)[name_and_type_index];

			methoddref_info->method_name =
					((CONSTANT_Utf8_Info **) cps)[name_type_info->name_index]->chars;

			methoddref_info->method_descriptor =
					((CONSTANT_Utf8_Info **) cps)[name_type_info->descriptor_index]->chars;

			break;
		}

		case 12: {
			CONSTANT_NameAndType_Info *name_and_type_info =
					(CONSTANT_NameAndType_Info *) cp;

			name_and_type_info->name =
					((CONSTANT_Utf8_Info **) cps)[name_and_type_info->name_index]->chars;

			name_and_type_info->descriptor =
					((CONSTANT_Utf8_Info **) cps)[name_and_type_info->descriptor_index]->chars;

			break;
		}
		}
	}

	return cps_total_byte_num;
}
