#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Transforms/Obfuscation/DiverseFunction.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/CryptoUtils.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <fstream>
#include <unistd.h>

using namespace llvm;

#define DEBUG_TYPE "divfunc"

STATISTIC(DivFunc, "Functions Diverse Counts");

namespace
{
struct DivFunction : public FunctionPass {
	static char ID;
	bool flag;
	vector<string> KnownFunc;

	DivFunction() : FunctionPass(ID) {}
	DivFunction( bool flag ) : FunctionPass(ID)
	{
		this->flag = flag;
	}


	bool runOnFunction(Function &F);
	void Diverse( Function * );
	void WrLog( string );
	void RdLog( vector<string>* );
	bool ModChanged( Function* );
	bool hasName( vector<string>, string );
	bool Akin( string, string );
};
}

char DivFunction::ID = 0;
static RegisterPass<DivFunction> X("DivFunction", "Diverse Function ");

Pass *llvm::createDivFunction( bool flag )
{ return new DivFunction( flag ); }

void DivFunction::WrLog( string Src )
{
	std::fstream fout;
	fout.open( "KnownFunction.def", std::fstream::out|std::fstream::app );
	fout << Src << "\n";
	fout.close();
}

void DivFunction::RdLog( vector<string> *FuncMap )
{
	std::fstream fin;
	fin.open( "KnownFunction.def", std::fstream::in );
	string fname;
	while ( fin >> fname )
		FuncMap->push_back( fname );
	fin.close();
}

bool DivFunction::ModChanged( Function *F )
{
	if ( access( "PrevMod.def", F_OK ) != -1 )
	{
	std::fstream fin;
	fin.open( "PrevMod.def", std::fstream::in );
	string mod;
	fin >> mod;
	fin.close();
	if ( mod == (string)F->getParent()->getModuleIdentifier() )
		return false;
	} else {
	std::fstream fout;
	fout.open( "PrevMod.def", std::fstream::out );
	fout << (string)F->getParent()->getModuleIdentifier() << "\n";
	fout.close();
	}
	return true;
}

bool DivFunction::hasName( vector<string> FList, string FN )
{
	for ( vector<string>::iterator ff = FList.begin(); ff != FList.end(); ff++ )
		if ( *ff == FN )
			return true;
	return false;
}

bool DivFunction::Akin( string StrC, string StrB )
{
	if ( StrC == StrB )
		return true;
	if ( StrC.length() < StrB.length() )
		return false;
	if ( StrC.substr( 0, StrB.length() ) != StrB )
		return false;
	if ( atoi( StrC.substr( StrB.length()+1 ).c_str() ) )
		return true;
	return false;
}

bool DivFunction::runOnFunction(Function &F)
{
	++DivFunc;
	Function *tmp = &F;

	if (toObfuscate(flag, tmp, "fdiv"))
		Diverse( tmp );
	return false;
}

void DivFunction::Diverse( Function *F )
{
errs() << "Div : " << F->getName() << "\n";

	if ( ModChanged( F ) )
		RdLog( &KnownFunc );

	WrLog( (string)F->getName() );

	//  Iterator through all the Call/Invoke Instructions
	//  Within the Function.
	for ( Function::iterator bb = F->begin(); bb != F->end(); bb++ )
		for ( BasicBlock::iterator inst = bb->begin(); inst != bb->end(); )
		{
			if ( !dyn_cast<CallInst>( &*inst ) && !dyn_cast<InvokeInst>( &*inst ) )
			{
				inst++;
				continue;
			}

			//  Probably vTable
			if( !inst->getOperand( inst->getNumOperands()-1)->hasName() )
			{
				inst++;
				continue;
			}

			Function *fc = dyn_cast<Function>( inst->getOperand( inst->getNumOperands() - 1 ) );

			if ( !fc->getBasicBlockList().empty() )
			{
				inst++;
				continue;
			}

			string base = (string)fc->getName();

			if ( !hasName( KnownFunc, base ) )
			{
				inst++;
				continue;
			}

			// Get SubList of Known Functions
			vector<string> FSubList;
			for ( vector<string>::iterator kf = KnownFunc.begin(); kf != KnownFunc.end(); kf++ )
				if ( Akin( *kf, base ) )
					FSubList.push_back( *kf );

			//  Select a Random Function
			string oFName = FSubList.at( cryptoutils->get_uint64_t() % FSubList.size() );

			//  Create New Declare Statement
			//    Create Params List
			vector<Type *> Params;
			for ( Function::arg_iterator argItr = fc->arg_begin(); argItr != fc->arg_end(); argItr++ )
				Params.push_back( argItr->getType() );

			//    Create new Function
			FunctionType *newFType = FunctionType::get( fc->getReturnType(), Params, fc->getFunctionType()->isVarArg() );
			Function *newFunc = Function::Create( newFType, fc->getLinkage(), oFName );

			//    Insert New Function
			fc->getParent()->getFunctionList().insert( fc, newFunc );

			//  Replace the Current Call/Invoke
			Instruction *newInst = inst->clone();

			newInst->setOperand( newInst->getNumOperands() - 1, newFunc );

			newInst->insertBefore( inst );

			inst->replaceAllUsesWith( newInst );
			newInst->takeName( inst );

			Instruction *DI = inst++;

			DI->eraseFromParent();
		}

	Function *f = F;
	vector<Function *> FList;

	if ( (string)f->getName() == (string)"main" )
		return;

	unsigned i = 0;
	unsigned ONum = ( cryptoutils->get_uint64_t()&0xff ) + 50;
	while ( i++ < ONum )
	{
		//  Create Params List
		vector<Type *> Params;
		for ( Function::arg_iterator argItr = f->arg_begin(); argItr != f->arg_end(); argItr++ )
			Params.push_back( argItr->getType() );

		//  Create new Function
		FunctionType *newFType = FunctionType::get( f->getReturnType(), Params, f->getFunctionType()->isVarArg() );
		Function *newFunc = Function::Create( newFType, f->getLinkage(), f->getName() );

		ValueToValueMapTy Vmap;

		//  Insert New Function
		f->getParent()->getFunctionList().insert( f, newFunc );

		//    Prepare Call Params
		Function::arg_iterator argOld = f->arg_begin();
		for ( Function::arg_iterator argItr = newFunc->arg_begin(); argItr != newFunc->arg_end(); argItr++, argOld++ )
		{
			argItr->setName( argOld->getName() );
			Vmap[argOld] = argItr;
		}

		// Ignore returns cloned.
		SmallVector<ReturnInst*, 8> Returns;
		CloneFunctionInto( newFunc, f, Vmap, false, Returns );

		FList.push_back( newFunc );

		// Write to File
		WrLog( (string)newFunc->getName() );
	}

	//  Iterator through all the Callers
	for ( Value::user_iterator itr = f->user_begin(); itr != f->user_end(); itr++ )
	{
	// If Caller is not Call/Invoke Inst, it's probably vTable
	if ( !dyn_cast<CallInst>(*itr) && !dyn_cast<InvokeInst>(*itr) )
		return;

	//CallSite cs( f->user_back() );
	CallSite cs( *itr );

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

	newInst->setOperand( newInst->getNumOperands() - 1, FList.at( cryptoutils->get_uint64_t()%ONum ) );

	newInst->insertBefore( userInst );

	// Replace the Original Call/Invoke
	userInst->replaceAllUsesWith( newInst );
	newInst->takeName( userInst );

	userInst->eraseFromParent();

	}
}
