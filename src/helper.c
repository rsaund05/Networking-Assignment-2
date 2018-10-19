#include "helper.h"

int isInt(char* str) {
	int len = strlen(str);

	if (len == 0 || str == NULL) {
		return 0;
	}

	for(int i = 0; i < len; i++) {
		if (!isdigit(str[i]))
			return 0;
	}
	return 1;
}