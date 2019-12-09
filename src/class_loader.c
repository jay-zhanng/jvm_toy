#include <stdio.h>
#include <stdlib.h>
#include "struct.h"
#include "util.h"
#include "class_parser.h"
#include "class_preparer.h"
#include "clinit.h"
#include "jvm_init.h"

extern char* CLASS_PATH;
extern char* DOT_CLASS;

CLASS* load_class(char* class_name) {

	//check  if this class have bean leaded
	CLASS* class_info = search_class_info(class_name);
	if (class_info != NULL) {
		return class_info;
	}

	printf("--------------class (%s) load start--------------\n", class_name);

	unsigned total_length = str_len(CLASS_PATH) + str_len(class_name)
			+ str_len(DOT_CLASS) + 1;
	char* full_class_path = (char*) malloc(total_length);
	get_full_class_path(full_class_path, class_name);
	full_class_path[total_length - 1] = 0;
	printf("full class (%s) path is: %s\n", class_name, full_class_path);

	unsigned int file_size = get_file_size(full_class_path);
	printf("class file (%s) size:%d\n", full_class_path, file_size);

	unsigned char *class_file_ptr = malloc(file_size);
	read_file(full_class_path, file_size, class_file_ptr);
	printf("class file (%s) content:\n", full_class_path);
//	dump_memory(class_file_ptr, file_size);
	free(full_class_path);

	class_info = req_class();
	class_info->clinit_methods_ptr = NULL;
	class_info->static_field_value_ptr = NULL;
	class_info->super_class_full_name = NULL;
	class_info->super_class_info = NULL;
	class_info->inst_fileds_info_ptr = NULL;

	//1.Loading
	parse_class(class_file_ptr, class_info);
	free(class_file_ptr);

	//2.Verification(pass)

	if (class_info->super_class_full_name != NULL) {
		load_class(class_info->super_class_full_name);
	}

	//3.Preparation
	prepare_class(class_info);
	//4.Resolution(pass)
	//5.Initialization,first init super class, java.lang.Object just ignore
	init_class(class_info);

	printf("--------------class (%s) load over--------------\n",
			class_info->this_class_full_name);

	return class_info;
}
