
#ifndef _FDIV_INCLUDES_
#define _FDIV_INCLUDES_

#include "llvm/Transforms/IPO.h"

using namespace std;

namespace llvm {
	Pass *createDivFunction();
	Pass *createDivFunction(bool flag);
}
#endif

