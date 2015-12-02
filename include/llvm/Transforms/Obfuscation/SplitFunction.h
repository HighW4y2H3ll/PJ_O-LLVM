
#ifndef _FSP_INCLUDES_
#define _FSP_INCLUDES_

#include "llvm/Transforms/IPO.h"

using namespace std;

namespace llvm {
	Pass *createSplitFunction();
	Pass *createSplitFunction(bool flag);
}
#endif

