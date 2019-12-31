#include <stdio.h>
#include <stdlib.h>
#include "struct.h"
#include "util.h"

extern char* CLASSPATH;
extern char* MAINCLASS;
extern char* COLON;
extern char* PRINT_LOG_STR;

char* CLASS_PATH;
int PRINT_LOG;

void get_class_path(char *argv) {
	int index = str_find_index(argv, COLON);
	char* classpath = str_sub(argv, ++index);

	int str_length = str_len(classpath);
	if (classpath[str_length - 1] != '/') {
		char* bytes = (char*) malloc(str_length + 2);
		for (int i = 0; i < str_length; i++) {
			bytes[i] = classpath[i];
		}
		bytes[str_length] = '/';
		bytes[str_length + 1] = 0;
		CLASS_PATH = bytes;
	} else {
		CLASS_PATH = classpath;
	}

	if (PRINT_LOG)
		printf("class path is:%s\n", CLASS_PATH);
}

char* get_main_class(char *argv) {
	unsigned int index = str_find_index(argv, COLON);
	char* main_class = str_sub(argv, ++index);

	unsigned int length = str_len(main_class);
	//replace . to / (com.Main -> com/Main)
	for (int i = 0; i < length; i++) {
		if (main_class[i] == '.') {
			main_class[i] = '/';
		}
	}

	if (PRINT_LOG)printf("main class is:%s\n", main_class);

	return main_class;
}
void get_print_log(char *argv) {
	unsigned int index = str_find_index(argv, COLON);
	char* print_log_str = str_sub(argv, ++index);

	if (str_equal(print_log_str, "1")) {
		PRINT_LOG = 1;
	} else {
		PRINT_LOG = 0;
	}

	if (PRINT_LOG)printf("print_log:%d\n", PRINT_LOG);
}

/**
 *parse method parameter,get class_path and main_class
 */
char* parse_args(int argc, char *argvs[]) {

	char* main_class_name;

	for (int i = 1; i < argc; i++) {
		char* argv = argvs[i];

		if (str_if_startof(argv, CLASSPATH)) {
			get_class_path(argv);
		} else if (str_if_startof(argv, MAINCLASS)) {
			main_class_name = get_main_class(argv);
		} else if (str_if_startof(argv, PRINT_LOG_STR)) {
			get_print_log(argv);
		}
	}

	return main_class_name;
}

