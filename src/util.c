#include <stdio.h>
#include <sys/stat.h>
#include "struct.h"
#include "jvm_init.h"

extern const unsigned int SLOT_SIZE;
extern char* CLASS_PATH;
extern char* DOT_CLASS;
extern CLASS* CLASS_INFOS;
extern int CLASS_INDEX;
extern INSTANCE* INSTANCES;
extern int INST_INDEX;
extern int PRINT_LOG;

STACK_FRAME* current_frame = NULL;

//64 little-endian to big-endian
long htonl(long a) {
	return ((a >> 56) & 0xff) | ((a >> 40) & 0xff00) | ((a >> 24) & 0xff0000)
			| ((a >> 8) & 0xff000000)

			| ((a << 8) & 0x000000ff00000000) | ((a << 24) & 0x0000ff0000000000)
			| ((a << 40) & 0x00ff000000000000)
			| ((a << 56) & 0xff00000000000000);
}

//32 little-endian to big-endian
int htoni(int a) {
	return ((a >> 24) & 0xff) | ((a >> 8) & 0xff00)

	| ((a << 8) & 0x00ff0000) | ((a << 24) & 0xff000000);
}

//16 little-endian to big-endian
short htons(short a) {
	return ((a >> 8) & 0x00ff) | ((a << 8) & 0xff00);
}

/**
 *  byte sign-extended to an int
 */
int signed_byte_int(char byte) {
	int int_value = 0;
	int flag = (byte & 0x80) >> 7;
	if (flag) {
		int_value = 0xFFFFFF00 | byte;
	} else {
		int_value = byte;
	}
	return int_value;
}
/**
 *  short sign-extended to an int
 */
int signed_short_int(short byte) {
	int int_value = 0;
	int flag = (byte & 0x8000) >> 15;
	if (flag) {
		int_value = 0xFFFF0000 | byte;
	} else {
		int_value = byte;
	}
	return int_value;
}

/**
 *  byte zero-extended to an int
 */
unsigned int zero_byte_int(unsigned char byte) {
	unsigned int int_value = byte;
	return int_value;
}
/**
 *  short zero-extended to an int value
 */
unsigned int zero_short_int(unsigned short byte) {
	unsigned int int_value = 0;
	int_value = byte;
	return int_value;
}

unsigned int read_file(char *filePath, unsigned int file_size,
		unsigned char* buffer) {

	FILE *in = fopen(filePath, "rb");
	int num = fread(buffer, sizeof(unsigned char), file_size, in);
	fclose(in);
	if (num == file_size) {
		return 1;
	} else {
		return 0;
	}
}

unsigned int get_file_size(const char *path) {
	unsigned int filesize = -1;
	struct stat statbuff;
	if (stat(path, &statbuff) < 0) {
		return filesize;
	} else {
		filesize = statbuff.st_size;
	}
	return filesize;
}

void int_to_hex(unsigned int d) {
	unsigned int n;
	int c;
	for (c = 28; c >= 0; c -= 4) {
		// get highest tetrad
		n = (d >> c) & 0xF;
		// 0-9 => '0'-'9', 10-15 => 'A'-'F'
		n += n > 9 ? 0x37 : 0x30;
		if (PRINT_LOG)
			printf("%c", n);
	}
}

void dump_memory(void *ptr, unsigned int byte_num) {
	unsigned char c, d;
	unsigned int linenum = 16;

	for (unsigned int index = 0; index < byte_num; index += linenum, ptr +=
			linenum) {

		int_to_hex(index);
		if (PRINT_LOG)
			printf(": ");

		for (unsigned int b = 0; b < linenum; b++) {
			c = *((unsigned char*) (ptr + b));
			d = c;
			d >>= 4;
			d &= 0xF;
			d += d > 9 ? 0x37 : 0x30;
			if (PRINT_LOG)
				printf("%c", d);

			d = c;
			d &= 0xF;
			d += d > 9 ? 0x37 : 0x30;
			if (PRINT_LOG)
				printf("%c", d);

			if (PRINT_LOG)
				printf("  ");

			if (b % 8 == 7)
				if (PRINT_LOG)
					printf("  	");

		}
		for (unsigned int b = 0; b < linenum; b++) {
			c = *((unsigned char*) (ptr + b));
			if (PRINT_LOG)
				printf("%c", c < 32 || c >= 127 ? '.' : c);
		}
		if (PRINT_LOG)
			printf("\n");

	}
}

void memory_copy(char *sou, char *des, unsigned int length) {
	for (int i = 0; i < length; i++) {
		des[i] = sou[i];
	}
}

/*
 *equal return 1,else return 0
 */
unsigned int str_equal(char *stra, char *strb) {
	int i = 0;
	while (1) {
		char a = stra[i];
		char b = strb[i];
		if (a != b) {
			return 0;
		} else {
			if (a == '\0' && b == '\0') {
				return 1;
			} else {
				i++;
			}
		}
	}
}

INST_FIELD_VALUE* research_inst_field_value(INSTANCE inst, char* tar_name,
		char* tar_descriptor) {
	INST_FIELD_VALUE* field = NULL;

	INST_FIELD_VALUE* INST_FIELD_PTR = inst.inst_field_value_ptr;
	CLASS* class_info = inst.class_info;

	unsigned short fields_count = class_info->total_fields_count;
	for (unsigned short i = 0; i < fields_count; i++) {
		field = INST_FIELD_PTR + i;
		char *name = field->name;
		char *descriptor = field->descriptor;
		if (str_equal(tar_name, name)
				&& str_equal(tar_descriptor, descriptor)) {
			break;
		}
	}

	if (field == NULL) {
		if (PRINT_LOG)
			printf("do not find the field: %s\n", tar_name);
		return field;
	}

	return field;
}

char* find_utf8_info(void *cps, unsigned short index) {

	CONSTANT_Utf8_Info** utf8_info = (CONSTANT_Utf8_Info **) cps;
	return utf8_info[index]->chars;
}

unsigned int str_len(char* str) {
	unsigned int len = 0;
	while (1) {
		if (!str[len]) {
			break;
		}
		len++;
	}

	return len;
}

void create_stack_frame(METHOD_INFO method_info) {

	CLASS *class_info = method_info.class_info;

	CODE_ATTRIBUTE* code = (CODE_ATTRIBUTE*) method_info.code_attr_ptr;
	unsigned short max_locals = code->max_locals;
	unsigned short max_stack = code->max_stack;
	unsigned int code_length = code->code_length;
	unsigned char *byte_codes = code->bytecode;

	if (PRINT_LOG)
		printf("--------------class(%s) method(%s) bytecode --------------\n",
				class_info->this_class_name, method_info.name);
	dump_memory(byte_codes, code_length);

	unsigned int stack_frame_size = sizeof(STACK_FRAME)
			+ (max_stack + max_locals) * SLOT_SIZE;

	unsigned char* stack = (unsigned char*) req_stack_area_memo(
			stack_frame_size);
	unsigned char* local_variable_erea = stack + sizeof(STACK_FRAME);
	unsigned char* operand_stack_erea = stack + sizeof(STACK_FRAME)
			+ max_locals * SLOT_SIZE;

	STACK_FRAME* current_frame_temp = (STACK_FRAME *) stack;
	current_frame_temp->frame_size = stack_frame_size;
	current_frame_temp->code_length = code_length;
	current_frame_temp->local_variable_erea = (int*) local_variable_erea;
	current_frame_temp->operand_stack_erea = (int*) operand_stack_erea;
	current_frame_temp->pc = 0;
	current_frame_temp->byte_codes = byte_codes;
	current_frame_temp->class_info = class_info;
	current_frame_temp->method_name = method_info.name;

	current_frame_temp->last_frame = current_frame;

	current_frame = current_frame_temp;
}

unsigned int get_param_num(char* desc) {
	unsigned int param_num = 0;
	for (unsigned int i = 1; i < str_len(desc); i++) {

		if (desc[i] == ')') {
			break;
		} else if (desc[i] == '[') {
			continue;
		} else if (desc[i] == 'L') {
			param_num++;
			while (1) {
				if (desc[++i] == ';') {
					break;
				}
			}
		} else {
			param_num++;
		}
	}

	return param_num;
}

/**
 * find start index of sub string
 */
int str_find_index(char* sources, char* targets) {

	int start_index = -1;

	int sou_index = 0;

	while (1) {
		char sou = sources[sou_index++];
		if (sou == 0) {
			return -1;
		}

		char tar = targets[0];
		if (sou == tar) {
			start_index = sou_index - 1;
			int tar_index = 1;

			while (1) {
				tar = targets[tar_index++];
				if (tar == 0) {
					return start_index;
				}

				sou = sources[sou_index];
				if (sou == 0) {
					return -1;
				}

				if (tar != sou) {
					break;
				} else {
					sou_index++;
				}
			}
		}
	}
}

/**
 * check if source str start with target str
 */
unsigned int str_if_startof(char* sources, char* targets) {

	int result = 1;

	if (str_len(targets) > str_len(sources)) {
		return 0;
	}

	int index = 0;
	while (1) {

		char sou = sources[index];
		char tar = targets[index];

		if (tar == 0) {
			break;
		}

		if (sou != tar) {
			result = 0;
			break;
		}

		index++;
	}

	return result;
}

/**
 *get sub string from start index(include) to end
 */
char* str_sub(char* sources, unsigned int start_index) {

	unsigned int str_length = str_len(sources);
	if (start_index + 1 > str_length) {
		return NULL;
	}

	return sources + start_index;
}

/**
 * get full class path:classpath + class_name + .class
 */
void get_full_class_path(char* full_class_path, char* class_name) {

	unsigned int path_index = 0;

	for (int i = 0; i < str_len(CLASS_PATH); i++) {
		full_class_path[path_index++] = CLASS_PATH[i];
	}

	for (int i = 0; i < str_len(class_name); i++) {
		full_class_path[path_index++] = class_name[i];
	}

	for (int i = 0; i < str_len(DOT_CLASS); i++) {
		full_class_path[path_index++] = DOT_CLASS[i];
	}
}

CLASS* req_class() {
	CLASS* class = CLASS_INFOS + CLASS_INDEX;
	CLASS_INDEX++;
	return class;
}

INSTANCE* req_inst() {
	INSTANCE* inst = INSTANCES + INST_INDEX;
	INST_INDEX++;
	return inst;
}

CLASS* search_class_info(char* class_name) {
	CLASS *tar_class = NULL;

	for (int i = 0; i < CLASS_INDEX; i++) {
		CLASS * class_temp = CLASS_INFOS + i;

		if (str_equal(class_temp->this_class_name, class_name)) {
			tar_class = class_temp;
			break;
		}

	}

	return tar_class;
}

CLASS* get_super_class(CLASS* curr_class_info) {

	CLASS* super_class = curr_class_info->super_class_info;
	if (super_class == NULL) {
		super_class = search_class_info(curr_class_info->super_class_name);
		curr_class_info->super_class_info = super_class;
	}

	return super_class;
}

CLASS* get_interface_info(INTERFACE* interface) {

	CLASS* interface_info = interface->interface_info;
	if (interface_info == NULL) {
		char* interface_name = interface->interface_name;
		interface_info = load_class(interface_name);
		interface->interface_info = interface_info;
	}

	return interface_info;
}

STATIC_FIELD_VALUE* research_static_field_value(CLASS* class_info,
		char* tar_name, char* tar_descriptor) {

	STATIC_FIELD_VALUE* tar_filed_value = NULL;

	while (class_info != NULL) {
		unsigned short fields_count = class_info->static_fields_count;
		for (unsigned int i = 0; i < fields_count; i++) {
			STATIC_FIELD_VALUE* tar_filed_value_temp =
					class_info->static_field_value_ptr + i;

			char *name = tar_filed_value_temp->name;
			char *descriptor = tar_filed_value_temp->descriptor;

			if (str_equal(tar_name, name)
					&& str_equal(tar_descriptor, descriptor)) {
				tar_filed_value = tar_filed_value_temp;
				break;
			}
		}

		if (tar_filed_value != NULL) {
			break;
		}

		char* super_class_name = class_info->super_class_name;
		if (super_class_name != NULL) {
			class_info = get_super_class(class_info);
		} else {
			class_info = NULL;
		}
	}

	return tar_filed_value;
}

METHOD_INFO* research_method_info(CONSTANT_Methodref_Info* methodref_ptr,
		CLASS* class_info, char* tar_name, char* tar_descriptor) {

	METHOD_INFO* tar_method = NULL;

	if (methodref_ptr->method != NULL) {
		return methodref_ptr->method;
	}

	while (1) {
		unsigned short methods_count = class_info->methods_count;
		for (unsigned int i = 0; i < methods_count; i++) {
			METHOD_INFO* method = class_info->all_methods_ptr + i;
			char *name = method->name;
			char *descriptor = method->descriptor;

			if (str_equal(tar_name, name)
					&& str_equal(tar_descriptor, descriptor)) {
				tar_method = method;
				break;
			}
		}

		if (tar_method != NULL) {
			break;
		}

		char* super_class_name = class_info->super_class_name;
		if (super_class_name != NULL) {
			class_info = get_super_class(class_info);
		} else {
			break;
		}
	}

	if (tar_method == NULL) {
		if (PRINT_LOG)
			printf("do not find the method: %s\n", tar_name);
		return tar_method;
	} else {
		methodref_ptr->method = tar_method;
	}

	return tar_method;
}

/*
 * get total inst fields num,include all super class
 */
unsigned int get_inst_filed_total_num(CLASS* class_info) {

	unsigned int total_num = 0;
	while (class_info != NULL) {
		total_num += class_info->inst_fields_count;

		char* super_class_name = class_info->super_class_name;
		if (super_class_name != NULL) {
			class_info = get_super_class(class_info);
		} else {
			class_info = NULL;
		}
	}

	return total_num;
}

/**
 * allocate memory and init value for instance field
 */
void init_inst_fields(CLASS* class_info, unsigned int total_fields_count,
		INST_FIELD_VALUE* field_value_ptr) {

	unsigned int current_fields_num = class_info->inst_fields_count;
	FIELD_INFO * current_fields_info = class_info->inst_fileds_info_ptr;

	unsigned int i = 0;
	while (1) {

		for (unsigned short j = 0; j < current_fields_num; j++, i++) {

			FIELD_INFO field_info = current_fields_info[j];
			char* filed_descriptor = field_info.descriptor;

			field_value_ptr[i].name = field_info.name;
			field_value_ptr[i].descriptor = filed_descriptor;

			void* ptr;
			if (str_equal(filed_descriptor, "D")
					|| str_equal(filed_descriptor, "J")) {
				ptr = req_heap_area_memo(2 * SLOT_SIZE);
			} else {
				ptr = req_heap_area_memo(SLOT_SIZE);
			}

			field_value_ptr[i].field_value_ptr = ptr;

			if (!str_if_startof(filed_descriptor, "L")
					|| !str_if_startof(filed_descriptor, "[")) {
				// primary type default initial value is 0
				*field_value_ptr[i].field_value_ptr = 0;
			} else {
				// reference type default initial value is null(-1)
				*field_value_ptr[i].field_value_ptr = -1;
			}
		}

		class_info = class_info->super_class_info;
		if (class_info == NULL) {
			break;
		}

		current_fields_num = class_info->inst_fields_count;
		current_fields_info = class_info->inst_fileds_info_ptr;
	}

}

