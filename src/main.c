#include <stdio.h>
#include "struct.h"
#include "main_method.h"
#include "jvm_init.h"
#include "class_loader.h"
#include "args_parser.h"

extern char* MAIN_CLASS;

int main(int argc, char *argv[]) {

	if (argc != 3) {
		printf("Please insert param: class-path and main-class !\n");
		return 0;
	}

	args_parse(argc, argv);

	init();

	load_class(MAIN_CLASS);

	main_method();

	printf("--------------all over!--------------\n");
	return 1;
}
