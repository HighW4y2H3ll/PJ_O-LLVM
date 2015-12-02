#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Transforms/Obfuscation/DupFunction.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/CryptoUtils.h"
#include "llvm/IR/CallSite.h"

using namespace llvm;

#define DEBUG_TYPE "dupfunc"

STATISTIC(DupFunc, "Functions Dup Counts");

namespace
{
struct DupFunction : public FunctionPass {
	static char ID;
	bool flag;

	DupFunction() : FunctionPass(ID) {}
	DupFunction( bool flag ) : FunctionPass(ID)
	{
		this->flag = flag;
	}


	bool runOnFunction(Function &F);
	void Dup( Function * );
};
}

char DupFunction::ID = 0;
static RegisterPass<DupFunction> X("DupFunction", "Duplicate Function ");

Pass *llvm::createDupFunction( bool flag )
{ return new DupFunction( flag ); }

bool DupFunction::runOnFunction(Function &F)
{
	++DupFunc;
	Function *tmp = &F;

	if (toObfuscate(flag, tmp, "fdp"))
		Dup( tmp );
	return false;
}

void DupFunction::Dup( Function *F )
{
errs() << " Dup \n";
	Function *f = F;
	vector<Function *> FList;

	if ( (string)f->getName() == (string)"main" )
		return;

	unsigned i = 0;
	unsigned ONum = cryptoutils->get_uint64_t()&0xff;
	while ( i++ < ONum )
	{
		//  Create Params List
		vector<Type *> Params;
		for ( Function::arg_iterator argItr = f->arg_begin(); argItr != f->arg_end(); argItr++ )
			Params.push_back( argItr->getType() );

		//  Create new Function
		FunctionType *newFType = FunctionType::get( f->getReturnType(), Params, false );
		Function *newFunc = Function::Create( newFType, f->getLinkage(), f->getName() );

		newFunc->copyAttributesFrom( f );

		//  Insert New Function
		f->getParent()->getFunctionList().insert( f, newFunc );

		//  Create Func Body
		BasicBlock *bb = BasicBlock::Create( newFunc->getContext(), "", newFunc );

		//    Prepare Call Params
		vector<Value *> fakeArg;
		for ( Function::arg_iterator argItr = newFunc->arg_begin(); argItr != newFunc->arg_end(); argItr++ )
		{
			Value *arg = dyn_cast<Value>( &*argItr );
			arg->setName("a");
			fakeArg.push_back( arg );
		}
		//    Create Call & Ret
		Instruction *inst = CallInst::Create( f, fakeArg, "", bb );
		Instruction *end = ReturnInst::Create( newFunc->getContext(), inst, bb );
		FList.push_back( newFunc );
	}

	//  Iterator through all the Callers
	for ( Value::user_iterator itr = f->user_begin(); itr != f->user_end(); itr++ )
	{
	// If Caller is not Call/Invoke Inst, it's probably vTable
	if ( !dyn_cast<CallInst>(*itr) && !dyn_cast<InvokeInst>(*itr) )
		return;

	//CallSite cs( f->user_back() );
	CallSite cs( *itr );

	// ID Original Call Funcs and Obfuscation Call Funcs
	// Set Different Obfuscation Ratio for each Case
	//
	// This ORatio should not be Set to 100,
	// To Avoid Loop
	unsigned ORatio = 100;
	for ( vector<Function *>::iterator UFItr = FList.begin(); UFItr != FList.end(); UFItr++ )
		if ( *UFItr == cs.getInstruction()->getParent()->getParent() )
			ORatio = 50;

errs() << cs.getInstruction()->getParent()->getParent()->getName() << "\n";

	// Handle Special Case: ios_base
	// Attribute::AttrKind::Dereferenceable ?
	if ( 	( f->getLinkage() == GlobalValue::LinkOnceODRLinkage ) &&
		( f->hasFnAttribute( Attribute::AttrKind::InlineHint ) ))
		return;

	// Skip Indirect Call, Probably vTable
	if ( !cs.getCalledFunction() )
		return;

	// Replace old call inst
	Instruction *userInst = cs.getInstruction();

	Instruction *newInst = userInst->clone();
	unsigned t = cryptoutils->get_uint64_t()%100;

errs() << "Rand : " << t  << " / " << ORatio << "\n";

	if ( ( t ) < ORatio )
		newInst->setOperand( newInst->getNumOperands() - 1, FList.at( cryptoutils->get_uint64_t()%ONum ) );

	newInst->insertBefore( userInst );

	// Replace the Original Call/Invoke
	userInst->replaceAllUsesWith( newInst );
	newInst->takeName( userInst );

	userInst->eraseFromParent();

	}
}
