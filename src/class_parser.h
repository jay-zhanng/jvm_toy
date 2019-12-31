unsigned int parse_methods(CLASS* class_info, METHOD_INFO *filed_methods,
		unsigned char *ptr, unsigned int methods_count);

unsigned int parse_attribute(void *cps, void *attribute_info,
		unsigned char *ptr, unsigned int attributes_count);

unsigned int parse_constant_pool(void *cps, unsigned char * ptr,
		unsigned int constant_pool_size);

void load(unsigned char *ptr, CLASS *class_info);

unsigned int parse_fields(CLASS *class_info, unsigned char *ptr);
