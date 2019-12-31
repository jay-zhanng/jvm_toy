#include <stdio.h>
#include "struct.h"
#include "main_method.h"
#include "jvm_init.h"
#include "class_loader.h"
#include "args_parser.h"

int main(int argc, char *argv[]) {

	if (argc != 4) {
		printf("please insert parameters: class-path main-class print_log!\n");
		return 0;
	}

	init();

	char* main_class_name = parse_args(argc, argv);

	CLASS* main_class_info = load_class(main_class_name);

	start(main_class_info);
	printf("--------------all over!--------------\n");
	return 1;
}
