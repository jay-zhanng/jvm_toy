typedef struct {

} INTERFACE;

typedef struct {
	struct CLASS* class_info;

	unsigned short access_flags;

	unsigned short name_index;
	char* name;

	unsigned short descriptor_index;
	char* descriptor;

	unsigned short attributes_count;
	void *attribute_infos;
	void *code_attr_ptr;

} METHOD_INFO;

typedef struct {
	unsigned short access_flags;
	char* name;
	char* descriptor;
	void *attribute_infos;

} FIELD_INFO;

typedef struct {
	unsigned short attribute_name_index;
	unsigned int attribute_length;
	unsigned short max_stack;
	unsigned short max_locals;
	unsigned int code_length;
	unsigned char *bytecode;
	unsigned short exception_table_length;

	void * exception_info;

	unsigned short attributes_count;
	void * attribute_infos;

} COMMON_ATTRIBUTE;

typedef struct {
	unsigned short attribute_name_index;
	unsigned int attribute_length;
	unsigned short max_stack;
	unsigned short max_locals;
	unsigned int code_length;
	unsigned char *bytecode;
	unsigned short exception_table_length;

	void * exception_info;

	unsigned short attributes_count;
	void * attribute_infos;

} CODE_ATTRIBUTE;

typedef struct {
	unsigned char tag;
} CONSTANT_Common_Info;

typedef struct {
	unsigned char tag;
	unsigned short index;
	char* class_name;

	struct CLASS *class;
	unsigned char if_reslution;

} CONSTANT_Class_Info;

typedef struct {
	unsigned char tag;

	unsigned short class_index;
	char* class_name;

	unsigned short name_and_type_index;
	char* filed_name;
	char* filed_descriptor;

	FIELD_INFO *field;
	unsigned char if_reslution;

} CONSTANT_Fieldref_Info;

typedef struct {
	unsigned char tag;

	unsigned short class_index;
	char* class_name;

	unsigned short name_and_type_index;
	char* method_name;
	char* method_descriptor;

	METHOD_INFO *method;
	unsigned char if_reslution;

} CONSTANT_Methodref_Info;

typedef struct {
	unsigned char tag;

	unsigned short class_index;
	char* class_name;

	unsigned short name_and_type_index;
	char* method_name;
	char* method_descriptor;

	METHOD_INFO *method;
	unsigned char if_reslution;

} CONSTANT_InterfaceMethodref_info;

typedef struct {
	unsigned char tag;
	unsigned short name_index;
	char* name;

	unsigned short descriptor_index;
	char* descriptor;

} CONSTANT_NameAndType_Info;

typedef struct {
	unsigned char tag;
	unsigned short index;
	char* value;

} CONSTANT_String_Info;

typedef struct {
	unsigned char tag;
	unsigned short length;
	char* chars;

} CONSTANT_Utf8_Info;

typedef struct {
	unsigned char tag;
	int value;

} CONSTANT_Integer_Info;

typedef struct {
	unsigned char tag;
	float value;

} CONSTANT_Float_Info;

typedef struct {
	unsigned char tag;
	long value;

} CONSTANT_Long_Info;

typedef struct {
	unsigned char tag;
	double value;

} CONSTANT_Double_Info;

typedef struct {
	char* name;
	char* descriptor;
	int *field_value_ptr;
} STATIC_FIELD_VALUE;

typedef struct {
	unsigned int magic;
	unsigned short minor_version;
	unsigned short major_version;

	unsigned short constant_pool_size;
	void *cps; //point array,every array element is the point of constant pool table

	unsigned short access_flags;
	char * this_class_full_name;
	char * super_class_full_name;
	struct CLASS *super_class_info;

	unsigned short interfaces_count;
	INTERFACE *interfaces;

	unsigned short static_fields_count;
	FIELD_INFO *static_fileds_info_ptr;

	unsigned short inst_fields_count; //include this class only
	FIELD_INFO *inst_fileds_info_ptr;

	STATIC_FIELD_VALUE *static_field_value_ptr;

	unsigned short methods_count;
	METHOD_INFO *all_methods_ptr;
	METHOD_INFO *clinit_methods_ptr;
	METHOD_INFO *main_methods_ptr;

	unsigned int method_area_total_size;
	unsigned short total_fields_count; //include this class and all spuer class

} CLASS;

typedef struct {
	char* name;
	char* descriptor;
	int *field_value_ptr;
} INST_FIELD_VALUE;

typedef struct {
	INST_FIELD_VALUE * inst_field_value;
	CLASS * class_info;
} INSTANCE;

typedef struct {
	CLASS *class_info;
	char* method_name;
	unsigned int frame_size;
	unsigned int code_length;
	struct STACK_FRAME* last_frame;
	unsigned int pc;
	unsigned char *byte_codes;
	int* operand_stack_erea;
	int* local_variable_erea;

} STACK_FRAME;
