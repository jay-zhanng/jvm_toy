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
extern int PRINT_LOG;

/**
 *class load -> class prepare -> class init
 */
CLASS* load_class(char* class_name) {

	//check if this class has bean loaded
	CLASS* class_info_ptr = search_class_info(class_name);
	if (class_info_ptr != NULL) {
		return class_info_ptr;
	}

	if (PRINT_LOG)
		printf("--------------class (%s) load start--------------\n",
				class_name);

	//get full class file path
	unsigned total_length = str_len(CLASS_PATH) + str_len(class_name)
			+ str_len(DOT_CLASS) + 1;
	char* full_class_path = (char*) malloc(total_length);
	get_full_class_path(full_class_path, class_name);
	full_class_path[total_length - 1] = 0;
	if (PRINT_LOG)
		printf("full class (%s) path is: %s\n", class_name, full_class_path);

	unsigned int file_size = get_file_size(full_class_path);
	if (PRINT_LOG)
		printf("class file (%s) size:%d\n", full_class_path, file_size);

	unsigned char *class_file_ptr = malloc(file_size);
	read_file(full_class_path, file_size, class_file_ptr);
//	if (PRINT_LOG)printf("class file (%s) content:\n", full_class_path);
//	dump_memory(class_file_ptr, file_size);
	free(full_class_path);

	class_info_ptr = req_class();

	class_info_ptr->class_file_size = file_size;

	//1.Load
	load(class_file_ptr, class_info_ptr);
	free(class_file_ptr);

	//2.Verification(pass)

	//load super class, initialization of a class requires prior initialization of
	//all its superclasses
	if (class_info_ptr->super_class_name != NULL) {
		load_class(class_info_ptr->super_class_name);
	}

	//3.Preparation
	prepare_class(class_info_ptr);

	//4.Resolution(pass)

	//5.Initialization
	init_class(class_info_ptr);

	if (PRINT_LOG)
		printf("--------------class (%s) load over--------------\n",
				class_info_ptr->this_class_name);

	return class_info_ptr;
}
