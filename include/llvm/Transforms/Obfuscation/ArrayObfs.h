
#ifndef _ARRAYOBFS_INCLUDES_
#define _ARRAYOBFS_INCLUDES_


// LLVM include
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/CryptoUtils.h"
#include "llvm/IR/BasicBlock.h"

// Namespace
using namespace std;

namespace llvm {
	Pass *createArrayObfs();
	Pass *createArrayObfs(bool flag);
}

#endif

