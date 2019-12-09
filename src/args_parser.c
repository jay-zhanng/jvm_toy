#include <stdio.h>
#include <stdlib.h>
#include "struct.h"
#include "util.h"

/**
 *parse main method param
 */

const char* CLASSPATH = "class-path";
const char* MAINCLASS = "main-class";
const char* SEPARATOR = ":";

extern char* CLASS_PATH;
extern char* MAIN_CLASS;

void get_class_path(char *argv) {
	unsigned int index = str_indexof(argv, SEPARATOR);
	char* classpath = str_sub_indexof(argv, ++index);

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

	printf("class path is:%s\n", CLASS_PATH);
}

void get_main_class(char *argv) {
	unsigned int index = str_indexof(argv, SEPARATOR);
	MAIN_CLASS = str_sub_indexof(argv, ++index);

	unsigned int length = str_len(MAIN_CLASS);
	for (int i = 0; i < length; i++) {
		if (MAIN_CLASS[i] == '.') {
			MAIN_CLASS[i] = '/';
		}
	}

	printf("main class is:%s\n", MAIN_CLASS);
}

void args_parse(int argc, char *argvs[]) {

	for (int i = 1; i < argc; i++) {
		char* argv = argvs[i];

		if (str_if_startof(argv, CLASSPATH)) {
			get_class_path(argv);
		} else if (str_if_startof(argv, MAINCLASS)) {
			get_main_class(argv);
		}
	}
}

