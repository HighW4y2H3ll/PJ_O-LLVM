
#ifndef _FDP_INCLUDES_
#define _FDP_INCLUDES_

#include "llvm/Transforms/IPO.h"

using namespace std;

namespace llvm {
	Pass *createDupFunction();
	Pass *createDupFunction(bool flag);
}
#endif

