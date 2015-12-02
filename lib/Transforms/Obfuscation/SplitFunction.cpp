#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Transforms/Obfuscation/SplitFunction.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/CallSite.h"
#include <fstream>
#include <unistd.h>

using namespace llvm;

#define DEBUG_TYPE "splitfunc"

STATISTIC(SplitFunc, "Functions Split Counts");


#define DEFFILE "LinkFunc.def"

namespace
{
struct SplitFunction : public FunctionPass {
	static char ID;
	bool flag;
	vector<Function*> FuncNodeList;
	vector<string> LinkFuncName;

	SplitFunction() : FunctionPass(ID) {}
	SplitFunction( bool flag ) : FunctionPass(ID)
	{
		this->flag = flag;
	}


	bool runOnFunction(Function &F);
	void Split( Function *, bool );
};
}

char SplitFunction::ID = 0;
static RegisterPass<SplitFunction> X("SplitFunction", "Split Function into Arbitrary Parts");

Pass *llvm::createSplitFunction( bool flag )
{ return new SplitFunction( flag ); }

bool SplitFunction::runOnFunction(Function &F)
{
	++SplitFunc;
	Function *tmp = &F;
errs() << "DEBUG : " << tmp->getParent()->getModuleIdentifier() << "\n";

	if (toObfuscate(flag, tmp, "fsp"))
	{
		bool newModFlag = true;
		string prevModName;
		std::fstream fd;
		if ( access( "ModName.def", F_OK ) == -1 )
		{
			fd.open( "ModName.def", std::fstream::out );
			fd << tmp->getParent()->getModuleIdentifier();
		} else {
			fd.open( "ModName.def", std::fstream::in );
			fd >> prevModName;
			newModFlag = ( prevModName==tmp->getParent()->getModuleIdentifier() );
		}
		fd.close();
		fd.open( "ModName.def", std::fstream::out );
		fd << tmp->getParent()->getModuleIdentifier();
		fd.close();

		Split( tmp, newModFlag );
	}
	return false;
}

void SplitFunction::Split( Function *F, bool newModFlag )
{
	Function *f = F;
	unsigned skflag = 0;
	unsigned firstRunFlag = 0;

	if ( access( DEFFILE, F_OK ) == -1 )
		firstRunFlag = 1;

	// every new Module will Clear the List
	if ( !firstRunFlag && newModFlag )
	{
		std::fstream fin;
		fin.open( DEFFILE, std::fstream::in );
		string fname;
		while ( fin >> fname )
			LinkFuncName.push_back( fname );
		fin.close();
	}

errs() << "DEBUG BEGIN\n";
	// Linking Modules
	{

	// Append Link Function Name into Local File
	std::fstream fout;
	fout.open( DEFFILE, std::fstream::out|std::fstream::app );
	fout << (string)f->getName() << "\n";
	fout.close();

	unsigned localSkFlag = 0;

	// Transform this current Function
	// TODO: Very Ugly Style, Need to reform

	// Check if Function has already been transformed
	for ( vector<Function*>::iterator itr = FuncNodeList.begin(); itr != FuncNodeList.end(); itr++ )
		if ( *itr == f )
			localSkFlag = 1;

	// Skip Main
	if ( (string)f->getName() == (string)"main" )
		localSkFlag = 1;
	
	// Skip for Variable Argument Function
	if ( f->isVarArg() )
		localSkFlag = 1;
	
	// Skip Functions without Param
	if ( f->getArgumentList().empty() )
		localSkFlag = 1;
errs() << "DEBUG 1\n";

	if ( !localSkFlag )
	{
	// Function Transformation
	//  Prepare new Argument
	vector<Type *> ParamHead;
	for ( Function::arg_iterator argItr = f->arg_begin(); argItr != f->arg_end(); argItr++ )
		ParamHead.push_back( argItr->getType() );
	StructType *Params = StructType::create( ParamHead );
	PointerType *ParamsPtr = Params->getPointerTo();

	//  Create new Function
	FunctionType *newFType = FunctionType::get( f->getReturnType(), ParamsPtr, false );
	Function *newFunc = Function::Create( newFType, f->getLinkage(), f->getName() );

	newFunc->copyAttributesFrom( f );

	// Insert New Function
	f->getParent()->getFunctionList().insert( f, newFunc );
	newFunc->takeName( f );
errs() << "DEBUG 3\n";

	// 
	// Iterator through all the Callers &&
	// Re-arrange Arguments Passed &&
	// Create the corresponding CallInst/InvokeInst

	// ++++++++++++++++++++++++++++++++++++++++++++++++++++
	// + NOTE: Ignore All the Argument Attributes
	// +  Ref: http://llvm.org/docs/doxygen/html/ArgumentPromotion_8cpp_source.html
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++
	unsigned END = f->getNumUses();
	for ( unsigned cnt = 0; cnt < END; cnt++ )
	{
	// If Caller is not Call/Invoke Inst, it's probably vTable
	if ( !dyn_cast<CallInst>(&*f->user_back()) && !dyn_cast<InvokeInst>(&*f->user_back()) )
		return;

	CallSite cs( f->user_back() );

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
errs() << "DEBUG 6 : " << *userInst << " : " << f->hasFnAttribute( Attribute::AttrKind::InlineHint ) << " : " << (f->getLinkage() == GlobalValue::LinkOnceODRLinkage) << "\n";

	Value *ONE = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 1 );
	Value *argSt = new AllocaInst( Params, ONE, "", userInst );

	Instruction *newCall;

	if ( CallInst *callInst = dyn_cast<CallInst>(&*userInst) )
	{
	for ( unsigned argI = 0; argI < callInst->getNumArgOperands(); argI++ )
	{
		Value *argVal = callInst->getArgOperand( argI );
		vector<Value *> idxList;
		Value *ZERO = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 0 );
		Value *IDX = ConstantInt::get( Type::getInt32Ty( f->getContext() ), argI );
		idxList.push_back( ZERO );
		idxList.push_back( IDX );

		Instruction *stPtr = GetElementPtrInst::Create( argSt, idxList, "", userInst );
		new StoreInst( argVal, stPtr, userInst );
	}

	// Create New Call
	newCall = CallInst::Create( newFunc, argSt, "", userInst );

	// Fixing
	cast<CallInst>( newCall )->setCallingConv( cs.getCallingConv() );
	if ( callInst->isTailCall() )
		cast<CallInst>( newCall )->setTailCall();

	}
	else if ( InvokeInst *callInst = dyn_cast<InvokeInst>(&*userInst) )
	{

	for ( unsigned argI = 0; argI < callInst->getNumArgOperands(); argI++ )
	{
		Value *argVal = callInst->getArgOperand( argI );
		vector<Value *> idxList;
		Value *ZERO = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 0 );
		Value *IDX = ConstantInt::get( Type::getInt32Ty( f->getContext() ), argI );
		idxList.push_back( ZERO );
		idxList.push_back( IDX );

		Instruction *stPtr = GetElementPtrInst::Create( argSt, idxList, "", userInst );
		new StoreInst( argVal, stPtr, userInst );
	}
	// Create New Invoke Call
	newCall = InvokeInst::Create( newFunc, callInst->getNormalDest(), callInst->getUnwindDest(), argSt, "", userInst );
	// Fixing
	cast<InvokeInst>( newCall )->setCallingConv( cs.getCallingConv() );

	}
	else
		return;

	// Replace the Original Call/Invoke
	userInst->replaceAllUsesWith( newCall );
	newCall->takeName( userInst );

	userInst->eraseFromParent();

	}

	// Copy Function Body
	// +  Ref: http://llvm.org/docs/doxygen/html/ArgumentPromotion_8cpp_source.html
	newFunc->getBasicBlockList().splice( newFunc->begin(), f->getBasicBlockList() );

	// Mark the Func as Transformed
	FuncNodeList.push_back( newFunc );

	// Re-arrange Parameters Processed
	//  Decode Arg Struct
	Instruction *funcFront = &newFunc->getBasicBlockList().front().front();

	Value *ZERO = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 0 );

	Value *arg = dyn_cast<Value>(&newFunc->getArgumentList().front());

	unsigned argI = 0;
	for ( Function::arg_iterator argItr = f->arg_begin(); argItr != f->arg_end(); argItr++ )
	{
		vector<Value *> idxList;
		Value *IDX = ConstantInt::get( Type::getInt32Ty( f->getContext() ), argI );
		idxList.push_back( ZERO );
		idxList.push_back( IDX );

		Instruction *stPtr = GetElementPtrInst::Create( arg, idxList, "", funcFront );
		Value *newArgVar = new LoadInst( stPtr, "", funcFront );
		newArgVar->takeName( argItr );
		argItr->replaceAllUsesWith( newArgVar );

		argI++;
	}

	//f->eraseFromParent();

	// Finxing f
	f = newFunc;

	}
errs() << "DEBUG 2\n";
	}

	for ( Function::iterator bb = f->begin(); bb != f->end(); bb++ )
	{
		for ( BasicBlock::iterator inst = bb->begin(); inst != bb->end() ; )
		{
			// Only Cares about Call & Invoke Instruction
			if ( dyn_cast<CallInst>(&*inst) || dyn_cast<InvokeInst>(&*inst) )
			{
				// Case : (1) system API ( e.g. glibc )
				//	  (2) virtual function ( e.g. vtable )
				//	  (3) direct call & Inter-Module Call
				//	  (4) direct call & the defination code is available
				// Skip Case(2)
				if ( Function *targetFunc = dyn_cast<Function>( inst->getOperand( inst->getNumOperands() - 1 ) ) )
				{
					// Skip Intrinsic Function
					if (  targetFunc->isIntrinsic() )
					{	inst++;
						continue;	}

					// Check if Function has already been transformed
					for ( vector<Function*>::iterator itr = FuncNodeList.begin(); itr != FuncNodeList.end(); itr++ )
						if ( *itr == targetFunc )
							skflag = 1;
					if ( skflag )
					{	inst++;
						continue;	}

					// Skip for Variable Argument Function
					if ( targetFunc->isVarArg() )
					{	inst++;
						continue;	}

					// Skip Functions without Param
					if ( targetFunc->getArgumentList().empty() )
					{	inst++;
						continue;	}

					// Handle Inter-Module Calls
					// Check FirstRun
					if ( targetFunc->getBasicBlockList().empty() )
					{
					if ( firstRunFlag )
					{	inst++;
						continue;
					} else {
						// Check Previous Link Func List
						unsigned lkFuncFlag = 0;
						for ( vector<string>::iterator lfnItr = LinkFuncName.begin(); lfnItr != LinkFuncName.end(); lfnItr++ )
							if ( *lfnItr == (string)targetFunc->getName() )
							{
							/*
							//
							// Found Extern Linking Function
							//  Since We know targetFunction has already modified
							//  Only Modify Callers & this Declare Inst
							// TODO: Again, Horrible Style
							//
							// Function Transformation
							//  Prepare new Argument
							vector<Type *> ParamHead;
							for ( Function::arg_iterator argItr = targetFunc->arg_begin(); argItr != targetFunc->arg_end(); argItr++ )
								ParamHead.push_back( argItr->getType() );
							StructType *Params = StructType::create( ParamHead );
							PointerType *ParamsPtr = Params->getPointerTo();

							//  Create new Function
							FunctionType *newFType = FunctionType::get( targetFunc->getReturnType(), ParamsPtr, false );
							Function *newFunc = Function::Create( newFType, targetFunc->getLinkage(), targetFunc->getName() );

							newFunc->copyAttributesFrom( targetFunc );

							// Insert New Function
							targetFunc->getParent()->getFunctionList().insert( targetFunc, newFunc );
							newFunc->takeName( targetFunc );

							// 
							// Iterator through all the Callers &&
							// Re-arrange Arguments Passed &&
							// Create the corresponding CallInst/InvokeInst

							// ++++++++++++++++++++++++++++++++++++++++++++++++++++
							// + NOTE: Ignore All the Argument Attributes
							// +  Ref: http://llvm.org/docs/doxygen/html/ArgumentPromotion_8cpp_source.html
							// ++++++++++++++++++++++++++++++++++++++++++++++++++++
							unsigned vTabFlag = 0;
							unsigned END = targetFunc->getNumUses();
							for ( unsigned cnt = 0; cnt < END; cnt++ )
							{
							CallSite cs( targetFunc->user_back() );

							// Skip Indirect Call, Probably vTable
							if ( !cs.getCalledFunction() )
							{	vTabFlag = 1;
								break;		}

							// Replace old call inst

							Instruction *userInst = cs.getInstruction();

							Value *ONE = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 1 );
							Value *argSt = new AllocaInst( Params, ONE, "", userInst );

							Instruction *newCall;

							if ( CallInst *callInst = dyn_cast<CallInst>(&*userInst) )
							{
							for ( unsigned argI = 0; argI < callInst->getNumArgOperands(); argI++ )
							{
								Value *argVal = callInst->getArgOperand( argI );
								vector<Value *> idxList;
								Value *ZERO = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 0 );
								Value *IDX = ConstantInt::get( Type::getInt32Ty( f->getContext() ), argI );
								idxList.push_back( ZERO );
								idxList.push_back( IDX );

								Instruction *stPtr = GetElementPtrInst::Create( argSt, idxList, "", userInst );
								new StoreInst( argVal, stPtr, userInst );
							}

							// Create New Call
							newCall = CallInst::Create( newFunc, argSt, "", userInst );

							// Fixing
							cast<CallInst>( newCall )->setCallingConv( cs.getCallingConv() );
							if ( callInst->isTailCall() )
								cast<CallInst>( newCall )->setTailCall();

							}
							else if ( InvokeInst *callInst = dyn_cast<InvokeInst>(&*userInst) )
							{

							for ( unsigned argI = 0; argI < callInst->getNumArgOperands(); argI++ )
							{
								Value *argVal = callInst->getArgOperand( argI );
								vector<Value *> idxList;
								Value *ZERO = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 0 );
								Value *IDX = ConstantInt::get( Type::getInt32Ty( f->getContext() ), argI );
								idxList.push_back( ZERO );
								idxList.push_back( IDX );

								Instruction *stPtr = GetElementPtrInst::Create( argSt, idxList, "", userInst );
								new StoreInst( argVal, stPtr, userInst );
							}
							// Create New Invoke Call
							newCall = InvokeInst::Create( newFunc, callInst->getNormalDest(), callInst->getUnwindDest(), argSt, "", userInst );
							// Fixing cast<InvokeInst>( newCall )->setCallingConv( cs.getCallingConv() );

							}
							else
								continue;

							// Replace the Original Call/Invoke
							userInst->replaceAllUsesWith( newCall );
							newCall->takeName( userInst );

							// Trick
							if ( inst->isIdenticalTo( userInst ) )
								inst++;
							userInst->eraseFromParent();

							}

							// Hack lkFuncFlag: Skip vTable
							if ( vTabFlag )
							{	lkFuncFlag = 0;
								break;		}

							// Mark the Func as Transformed
							FuncNodeList.push_back( newFunc );

							targetFunc->eraseFromParent();
							*/

							lkFuncFlag = 1;

							break;
							}
						if ( !lkFuncFlag )
						{	inst++;
							continue;	}
					}
					}

					// Function Transformation
					//  Prepare new Argument
					vector<Type *> ParamHead;
					for ( Function::arg_iterator argItr = targetFunc->arg_begin(); argItr != targetFunc->arg_end(); argItr++ )
						ParamHead.push_back( argItr->getType() );
					StructType *Params = StructType::create( ParamHead );
					PointerType *ParamsPtr = Params->getPointerTo();

					//  Create new Function
					FunctionType *newFType = FunctionType::get( targetFunc->getReturnType(), ParamsPtr, false );
					Function *newFunc = Function::Create( newFType, targetFunc->getLinkage(), targetFunc->getName() );
					//Function *newFunc = Function::Create( newFType, targetFunc->getLinkage(), "" );

					newFunc->copyAttributesFrom( targetFunc );

					// Insert New Function
					targetFunc->getParent()->getFunctionList().insert( targetFunc, newFunc );
					newFunc->takeName( targetFunc );

					// 
					// Iterator through all the Callers &&
					// Re-arrange Arguments Passed &&
					// Create the corresponding CallInst/InvokeInst

					// ++++++++++++++++++++++++++++++++++++++++++++++++++++
					// + NOTE: Ignore All the Argument Attributes
					// +  Ref: http://llvm.org/docs/doxygen/html/ArgumentPromotion_8cpp_source.html
					// ++++++++++++++++++++++++++++++++++++++++++++++++++++
					unsigned vTabFlag = 0;
					unsigned END = targetFunc->getNumUses();
					for ( unsigned cnt = 0; cnt < END; cnt++ )
					{
					// If Caller is not Call/Invoke Inst, it's probably vTable
					if ( !dyn_cast<CallInst>(&*targetFunc->user_back()) && !dyn_cast<InvokeInst>(&*targetFunc->user_back()) )
						return;

					CallSite cs( targetFunc->user_back() );

					// Handle Special Case: ios_base
					// Attribute::AttrKind::InLineHint ?
					if ( 	( targetFunc->getLinkage() == GlobalValue::LinkOnceODRLinkage ) &&
						( targetFunc->hasFnAttribute( Attribute::AttrKind::Dereferenceable ) ))
						return;

					// Skip Indirect Call, Probably vTable
					if ( !cs.getCalledFunction() )
					{	vTabFlag = 1;
						break;		}

					// Replace old call inst

					Instruction *userInst = cs.getInstruction();

					Value *ONE = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 1 );
					Value *argSt = new AllocaInst( Params, ONE, "", userInst );

					Instruction *newCall;

					if ( CallInst *callInst = dyn_cast<CallInst>(&*userInst) )
					{
					for ( unsigned argI = 0; argI < callInst->getNumArgOperands(); argI++ )
					{
						Value *argVal = callInst->getArgOperand( argI );
						vector<Value *> idxList;
						Value *ZERO = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 0 );
						Value *IDX = ConstantInt::get( Type::getInt32Ty( f->getContext() ), argI );
						idxList.push_back( ZERO );
						idxList.push_back( IDX );

						Instruction *stPtr = GetElementPtrInst::Create( argSt, idxList, "", userInst );
						new StoreInst( argVal, stPtr, userInst );
					}

					// Create New Call
					newCall = CallInst::Create( newFunc, argSt, "", userInst );

					// Fixing
					cast<CallInst>( newCall )->setCallingConv( cs.getCallingConv() );
					if ( callInst->isTailCall() )
						cast<CallInst>( newCall )->setTailCall();

					}
					else if ( InvokeInst *callInst = dyn_cast<InvokeInst>(&*userInst) )
					{

					for ( unsigned argI = 0; argI < callInst->getNumArgOperands(); argI++ )
					{
						Value *argVal = callInst->getArgOperand( argI );
						vector<Value *> idxList;
						Value *ZERO = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 0 );
						Value *IDX = ConstantInt::get( Type::getInt32Ty( f->getContext() ), argI );
						idxList.push_back( ZERO );
						idxList.push_back( IDX );

						Instruction *stPtr = GetElementPtrInst::Create( argSt, idxList, "", userInst );
						new StoreInst( argVal, stPtr, userInst );
					}
					// Create New Invoke Call
					newCall = InvokeInst::Create( newFunc, callInst->getNormalDest(), callInst->getUnwindDest(), argSt, "", userInst );
					// Fixing
					cast<InvokeInst>( newCall )->setCallingConv( cs.getCallingConv() );

					}
					else
						continue;

					// Replace the Original Call/Invoke
					userInst->replaceAllUsesWith( newCall );
					newCall->takeName( userInst );

					// Trick
					if ( inst->isIdenticalTo( userInst ) )
						inst++;
					userInst->eraseFromParent();

					}
					if ( vTabFlag )
					{	inst++;
						continue;	}
						

					// Copy Function Body
					// +  Ref: http://llvm.org/docs/doxygen/html/ArgumentPromotion_8cpp_source.html
					newFunc->getBasicBlockList().splice( newFunc->begin(), targetFunc->getBasicBlockList() );

					// Mark the Func as Transformed
					FuncNodeList.push_back( newFunc );

					// Re-arrange Parameters Processed
					//  Decode Arg Struct
					Instruction *funcFront = &newFunc->getBasicBlockList().front().front();

					Value *ZERO = ConstantInt::get( Type::getInt32Ty( f->getContext() ), 0 );

					Value *arg = dyn_cast<Value>(&newFunc->getArgumentList().front());

					unsigned argI = 0;
					for ( Function::arg_iterator argItr = targetFunc->arg_begin(); argItr != targetFunc->arg_end(); argItr++ )
					{
						vector<Value *> idxList;
						Value *IDX = ConstantInt::get( Type::getInt32Ty( f->getContext() ), argI );
						idxList.push_back( ZERO );
						idxList.push_back( IDX );

						Instruction *stPtr = GetElementPtrInst::Create( arg, idxList, "", funcFront );
						Value *newArgVar = new LoadInst( stPtr, "", funcFront );
						newArgVar->takeName( argItr );
						argItr->replaceAllUsesWith( newArgVar );

						argI++;
					}

					targetFunc->eraseFromParent();
/*
					// Split BasicBlock? or Split BasicBlock-s?
					// If there's plenty of BBs, the latter;
					// If not enough, ( i.e. One BB in a Function ), the former.

					//    Get list of BB & Check its length
					unsigned bbListLen = targetFunc->getBasicBlockList().size();

					if ( bbListLen > 1 )
					//    Split Function
					{
					//    Create two Bins
					//Function *FuncPart_1 = 
					//    Add BBs
					for ( Function::iterator tbb = targetFunc->begin(); tbb != targetFunc->end(); tbb++ )
					break;
					//    Replace orig Func
					errs() << inst->getOpcodeName() << " : " << *targetFunc << "\n";

					} else if ( bbListLen == 1 ) {

					//    Split the only BB into 2 Funcs
					errs() << bbListLen << " : " << targetFunc->front() << "\n";
					for ( BasicBlock::iterator bbInst = targetFunc->front().begin(); bbInst != targetFunc->front().end(); bbInst++ )
					{
						// Filter out Instructions without Left-Value

						///////////// ABORTED ////////////////////
						// Terminator Instructions , Except: 	invoke, catchpad
						// Memory Access Operation : 		store, fence
						//if ( !bbInst->isTerminator() && 

						// Call,Invoke,Catchpad Inst ----> check Return Value Type
						// Cmpxchg ----> metadata ?
						//////////////////////////////////////////

						// Easily Dealed with Value->getType() == void Check
						Value *instVal = dyn_cast<Value>(&*bbInst);
						errs() << *instVal << " : " << **bbInst->getOperand( 0 )->use_begin() << "\n" ;

						Module *M = f->getParent();
						if ( instVal->getType() != Type::getVoidTy( M->getContext() ) )
						{
							// Create a Global Variable for each local Variable
							// Special Case for alloca Inst
							Constant *initGV = ConstantInt::get( instVal->getType(), 0 );
							GlobalVariable *gv = new GlobalVariable( *M, instVal->getType(), false, GlobalValue::CommonLinkage, initGV );

							for ( Value::use_iterator xref = instVal->use_begin(); xref != instVal->use_end(); xref++)
							{
								Instruction *tmp = dyn_cast<Instruction>(*xref);
								//errs() << *bbInst << " : " << *tmp << "\n";

							}
						}						
					}

					} else { continue; }
*/
				} else { inst++; }
			} else { inst++; }
		}
	}
errs() << "DEBUG ENDIGN\n";
}
