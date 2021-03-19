#ifndef COMMON_MODELTYPE_H_
#define COMMON_MODELTYPE_H_

#include <string>

struct ModelType
{
	enum {
		TYPE, REF
	} type;
	std::string ref;
	int t;
	ModelType() {
		type = TYPE;
		t = -99;
	};
	~ModelType() {
	};
};

#endif /* COMMON_MODELTYPE_H_ */
