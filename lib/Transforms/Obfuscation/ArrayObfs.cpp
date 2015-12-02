#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Transforms/Obfuscation/ArrayObfs.h"
#include "llvm/Transforms/Obfuscation/Utils.h"
#include "llvm/IR/Constants.h"
#include "llvm/ADT/ArrayRef.h"

using namespace llvm;

#define DEBUG_TYPE "array"

STATISTIC(ArrayMod, "Counts number of Arrays Modified");

namespace
{
struct ArrayObfs : public FunctionPass
{
	static char ID;
	bool flag;
	
	ArrayObfs() : FunctionPass(ID) {}
	ArrayObfs( bool flag ) : FunctionPass(ID)
	{
		this->flag = flag;
	}
	
	bool runOnFunction(Function &F);
	void ArrObfuscate( Function *F );
};
}

char ArrayObfs::ID = 0;
static RegisterPass<ArrayObfs> X("array_obfuscation", "Add Bogus Array Indexing");
Pass *llvm::createArrayObfs(bool flag) { return new ArrayObfs(flag); }

bool ArrayObfs::runOnFunction(Function &F)
{
	Function *tmp = &F;
	if (toObfuscate(flag, tmp, "arr"))
		ArrObfuscate( tmp );
	return false;
}

void ArrayObfs::ArrObfuscate ( Function *F )
{

	// Iterate the whole Function
	Function *f = F;
	for ( Function::iterator bb = f->begin(); bb != f->end(); ++bb )
	{
		for ( BasicBlock::iterator inst = bb->begin(); inst != bb->end(); )
		{
			if ( inst->getOpcode() == 29 )		// getelementptr
			{
				//errs() << "INST : " << *inst << "\n";

				GetElementPtrInst *Ary = dyn_cast<GetElementPtrInst>(&*inst);
				Value *ptrVal = Ary->getOperand(0);
				Type *type = ptrVal->getType();

				unsigned numOfOprand = Ary->getNumOperands();
				unsigned lastOprand = numOfOprand - 1;

				// Check Type Array

				if ( PointerType *ptrType = dyn_cast<PointerType>( type ) )
				{
					Type *elementType = ptrType->getElementType();
					if ( elementType->isArrayTy() )
					{
						// Skip if Index is a Variable
						if ( dyn_cast<ConstantInt>( Ary->getOperand( lastOprand ) ) )
						{

				//////////////////////////////////////////////////////////////////////////////

				// Do Real Stuff
				Value *oprand = Ary->getOperand( lastOprand );
				Value *basePtr = Ary->getOperand( 0 );
				APInt offset = dyn_cast<ConstantInt>(oprand)->getValue();
				Value *prevPtr = basePtr;

				// Enter a Loop to Perform Random Obfuscation
				unsigned cnt = 100;

				// Prelog : Clone the Original Inst
				unsigned ObfsIdx =  cryptoutils->get_uint64_t() & 0xffff;
				Value *newOprand = ConstantInt::get( oprand->getType(), ObfsIdx );
				Instruction *gep = inst->clone();
				gep->setOperand( lastOprand, newOprand );
				gep->setOperand( 0, prevPtr );
				gep->insertBefore( inst );
				prevPtr = gep;
				offset = offset - ObfsIdx;

				// Create a Global Variable to Avoid Optimization
				Module *M = f->getParent();
				Constant *initGV = ConstantInt::get( prevPtr->getType(), 0 );
				GlobalVariable *gv = new GlobalVariable( *M, prevPtr->getType(), false, GlobalValue::CommonLinkage, initGV );

				while ( cnt-- )
				{
					// Iteratively Generate Obfuscated Code
					switch( cryptoutils->get_uint64_t() & 7 )
					{
					// Random Indexing Obfuscation
					case 0 :
					case 1 :
					case 2 :
						{
						//errs() << "=> Random Index \n";

						// Create New Instruction
						//   Create Obfuscated New Oprand in ConstantInt Type
						unsigned ObfsIdx =  cryptoutils->get_uint64_t() & 0xffff;
						Value *newOprand = ConstantInt::get( oprand->getType(), ObfsIdx );

						//   Create GetElementPtrInst Instruction
						GetElementPtrInst *gep = GetElementPtrInst::Create( prevPtr, newOprand, "", inst );

						//Set prevPtr
						prevPtr = gep;

						//errs() << "Created : " << *prevPtr << "\n";

						offset = offset - ObfsIdx;
						break;
						}

					// Ptr Dereference
					case 3 :
					case 4 :
						{
						//errs() << "=> Ptr Dereference \n";

						Module *M = f->getParent();
						Value *ONE = ConstantInt::get( Type::getInt32Ty( M->getContext() ), 1 );
						Value *tmp = new AllocaInst( prevPtr->getType(), ONE, "", inst );

						new StoreInst( prevPtr, tmp, inst );

						prevPtr = new LoadInst( tmp, "", inst );

						break;
						}

					// Ptr Value Transform
					case 5 :
					case 6 :
					case 7 :
						{
						//errs() << "=> Ptr Value Trans \n";

						unsigned RandNum =  cryptoutils->get_uint64_t();
						Value *ObfsVal = ConstantInt::get( prevPtr->getType(), RandNum );

						BinaryOperator *op = BinaryOperator::Create( Instruction::FAdd, prevPtr, ObfsVal, "", inst );
						new StoreInst( prevPtr, gv, inst );
						BinaryOperator::Create( Instruction::FSub, gv, ObfsVal, "", inst );
						prevPtr = new LoadInst( gv, "", inst );

						break;
						}
					}
				}

				// Postlog : Fix the Original Indexing
				{
				Value *fixOprand = ConstantInt::get( oprand->getType(), offset );
				// Refine the Last Instruction
				GetElementPtrInst *gep = GetElementPtrInst::Create( prevPtr, fixOprand, "", inst );

				// Fix the Relationship
				inst->replaceAllUsesWith( gep );

				// Finally : Unlink This Instruction From Parent
				Instruction *DI = inst++;
				//errs() << "user_back : " << *(DI->user_back()) << "\n";
				DI->removeFromParent();
				}

				//////////////////////////////////////////////////////////////////////////////

						// End : Variable Index
						} else { inst++; }
					// End : Check Array Type
					} else { inst++; }
				// End : Check Pointer Type
				} else { inst++; }
			// End : Check Opcode GetElementPtr
			} else { inst++; }
		}
	}
	++ArrayMod;
}
