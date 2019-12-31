long htonl(long a);

int htoni(int a);

short htons(short a);

int signed_byte_int(char byte);

int signed_short_int(short byte);

unsigned int zero_byte_int(unsigned char byte);

unsigned int zero_short_int(unsigned short byte);

unsigned int get_file_size(const char *path);

void int_to_hex(unsigned int d);

void dump_memory(void *ptr, unsigned int byte_num);

void memory_copy(unsigned char *sou, unsigned char *des, unsigned int length);

unsigned int read_file(char *filePath, unsigned int file_size,
		unsigned char* buffer);

unsigned int str_equal(char *stra, char *strb);

unsigned int str_len(char* str);

METHOD_INFO* research_method_info(CONSTANT_Methodref_Info* methodref_ptr,
		CLASS* class_info, char* tar_name, char* tar_descriptor);

STATIC_FIELD_VALUE* research_static_field_value(CLASS* class_info,
		char* tar_name, char* tar_descriptor);

INST_FIELD_VALUE* research_inst_field_value(INSTANCE inst, char* tar_name,
		char* tar_descriptor);

char* find_utf8_info(void *cps, unsigned short index);

void create_stack_frame(METHOD_INFO method_info);

unsigned int get_param_num(char* desc);

char* str_sub(char* sources, unsigned int start_index);

unsigned int str_if_startof(char* sources, const char* targets);

int str_find_index(char* sources, const char* targets);

void get_full_class_path(char* full_class_path, char* class_name);

CLASS* req_class();

INSTANCE* req_inst();

CLASS* search_class_info(char* class_name);

CLASS* get_super_class(CLASS* class_info);

CLASS* get_interface_info(INTERFACE* interface);

unsigned int get_inst_filed_total_num(CLASS* class);

void init_inst_fields(CLASS* class_info, unsigned int total_fields_count,
		INST_FIELD_VALUE* fields_ptr);

