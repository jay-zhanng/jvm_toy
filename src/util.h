long htonl(long a);

int htoni(int a);

short htons(short a);

int byte_int(char byte);

int short_int(short byte);

unsigned int get_file_size(const char *path);

void int_to_hex(unsigned int d);

void dump_memory(void *ptr, unsigned int byte_num);

void memory_copy(unsigned char *sou, unsigned char *des, unsigned int length);

unsigned int read_file(char *filePath, unsigned int file_size,
		unsigned char* buffer);

unsigned int str_cmp(char *stra, char *strb);

unsigned int str_len(char* str);

METHOD_INFO* research_method_info(CLASS* class_info, char* tar_name,
		char* tar_descriptor);

STATIC_FIELD_VALUE* research_static_field_info(CLASS* class_info,
		char* tar_name, char* tar_descriptor);

INST_FIELD_VALUE research_inst_field_value(CLASS* class_info,
		INST_FIELD_VALUE* INST_FIELD_PPTR, char* tar_name, char* tar_descriptor);

char* find_utf8_info(void *cps, unsigned short index);

void create_stack_frame(METHOD_INFO method_info);

unsigned int prase_method_desc(char* desc);

char* str_sub_indexof(char* sources, unsigned int start_index);

unsigned int str_if_startof(char* sources, const char* targets);

unsigned int str_indexof(char* sources, const char* targets);

void get_full_class_path(char* full_class_path, char* class_name);

CLASS* req_class();

INSTANCE* req_inst();

CLASS* search_class_info(char* class_name);

unsigned int get_inst_filed_total_num(CLASS* class);

void init_inst_fields(CLASS* class_info, unsigned int total_fields_count,
		INST_FIELD_VALUE* fields_ptr);

