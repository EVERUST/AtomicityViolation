
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"

#include "llvm/IR/Metadata.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/AsmParser/LLToken.h"

//#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

namespace {

void initialization(Function &F);
void storeLog(Instruction &I);
void loadLog(BasicBlock &B, Instruction &I);
void lockLog(InvokeInst *inv);
void unlockLog(InvokeInst *inv);
void rwlockReadLog(InvokeInst *inv, StringRef op);
void rwlockWriteLog(InvokeInst *inv, StringRef op);

struct HelloWorld : PassInfoMixin<HelloWorld> {

	bool initial = false;

	Type *intTy, *ptrTy, *voidTy, *boolTy;
	string FName;

	FunctionCallee p_init;
	FunctionCallee p_probe;
	FunctionCallee p_probe_lock;

	// Main entry point, takes IR unit to run the pass on (&F) and the
	// corresponding pass manager (to be queried if need be)
	PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

		string moduleName = F.getParent()->getName().str();
		string filename = moduleName.substr(0, moduleName.find(".ll"));

		FName = F.getName().str();
		//if(FName.find("core") != string::npos || FName.find("std") != string::npos) {
		if(FName.find(filename) == string::npos || (FName.find("core") != string::npos || FName.find("std") != string::npos)) {
			return PreservedAnalyses::all();
		}

		errs() << "function: " << FName << "\n";

		if(!initial) {
			initialization(F);
			initial = true;
		}

		for(auto& B : F) {
			for(auto& I : B) {
				if(I.getOpcode() == Instruction::Store) { // store
					//errs() << "store!\n";
					storeLog(I);

					//if(I.getOperand(0)->getType() == intTy) { // int
					//}

				} else if(I.getOpcode() == Instruction::Load) { // load
					//errs() << "load!\n";
					loadLog(B, I);

					//if(I.getType() == intTy) { // int
					//}
				}
				else if(I.getOpcode() == Instruction::Invoke) {
					
					InvokeInst * inv = dyn_cast<InvokeInst>(&I);
					if(inv->getDebugLoc().get() != NULL) {
						string funcName = inv->getCalledFunction()->getName().str();
						
						if(funcName.find("_ZN3std4sync5mutex14Mutex$LT$T$GT$4lock") != std::string::npos) { // lock
							//errs() << "lock\n";

							lockLog(inv);

						} else if(funcName.find("_ZN4core3ptr60drop_in_place$LT$std..sync..mutex..MutexGuard") != std::string::npos) { // unlock
							//errs() << "unlock\n";
							
							int loc = inv->getDebugLoc().getLine();
							Value * var = inv->getArgOperand(0);

							IRBuilder<> builder(inv);

							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();

							Value * MutexGuard = builder.CreateLoad(var->getType()->getPointerElementType(), var);
							Value * lockPtr = builder.CreateStructGEP(MutexGuard->getType(), var, 0);
							Value * lock = builder.CreateLoad(lockPtr->getType()->getPointerElementType(), lockPtr);

							Value * varAddr = builder.CreateBitCast(lock, ptrTy);

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(FName, ""), // variable name
								builder.CreateGlobalStringPtr("unlock", ""), // operation
								varAddr
								//lock
							};
							builder.CreateCall(p_probe_lock, args);
						} else if(funcName.find("_ZN3std4sync6rwlock15RwLock$LT$T$GT$4read") != std::string::npos) { // rwlock read lock
							rwlockReadLog(inv);
						} else if(funcName.find("_ZN4core3ptr66drop_in_place$LT$std..sync..rwlock..RwLockReadGuard") != std::string::npos) { // rwlock read unlock
							int loc = inv->getDebugLoc().getLine();
							Value * var = inv->getArgOperand(0);

							IRBuilder<> builder(inv);

							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();

							Value * MutexGuard = builder.CreateLoad(var->getType()->getPointerElementType(), var);
							//MutexGuard->getType()->dump();

							Value * varAddr = builder.CreateBitCast(MutexGuard, ptrTy);

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(FName, ""), // variable name
								builder.CreateGlobalStringPtr("rwlock read unlock", ""), // operation
								varAddr
							};
							builder.CreateCall(p_probe_lock, args);
						} else if(funcName.find("_ZN3std4sync6rwlock15RwLock$LT$T$GT$5write") != std::string::npos) { // rwlock write lock
							rwlockWriteLog(inv);
						} else if(funcName.find("_ZN4core3ptr67drop_in_place$LT$std..sync..rwlock..RwLockWriteGuard") != std::string::npos) { // rwlock write unlock
							int loc = inv->getDebugLoc().getLine();
							Value * var = inv->getArgOperand(0);

							IRBuilder<> builder(inv);

							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();

							Value * MutexGuard = builder.CreateLoad(var->getType()->getPointerElementType(), var);
							Value * lockPtr = builder.CreateStructGEP(MutexGuard->getType(), var, 0);
							Value * lock = builder.CreateLoad(lockPtr->getType()->getPointerElementType(), lockPtr);

							Value * varAddr = builder.CreateBitCast(lock, ptrTy);

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(FName, ""), // variable name
								builder.CreateGlobalStringPtr("rwlock write unlock", ""), // operation
								varAddr
							};
							builder.CreateCall(p_probe_lock, args);
						}
					}
				}
			}
		}
		return PreservedAnalyses::none();
	}

	// Without isRequired returning true, this pass will be skipped for functions
	// decorated with the optnone LLVM attribute. Note that clang -O0 decorates
	// all functions with optnone.
	static bool isRequired() { return true; }

	void initialization(Function &F) {
		intTy = Type::getInt32Ty(F.getContext());
		ptrTy = Type::getInt8PtrTy(F.getContext());
		voidTy = Type::getVoidTy(F.getContext());
		boolTy = Type::getInt1Ty(F.getContext());

		LLVMContext &Ctx = F.getContext();

		FunctionType * fty = FunctionType::get(voidTy, false);
		p_init = F.getParent()->getOrInsertFunction("_init_", fty);

		vector<Type*> paramTypes = {intTy, ptrTy, ptrTy, ptrTy};
		fty = FunctionType::get(voidTy, paramTypes, false);
		p_probe = F.getParent()->getOrInsertFunction("_probe_", fty);

		paramTypes = {intTy, ptrTy, ptrTy, ptrTy};
		fty = FunctionType::get(voidTy, paramTypes, false);
		p_probe_lock = F.getParent()->getOrInsertFunction("_probe_lock_", fty);
		
		Function * mainFunc = F.getParent()->getFunction(StringRef("main"));
		if(mainFunc != NULL) {
			IRBuilder<> builder(mainFunc->getEntryBlock().getFirstNonPHI());
			vector<Value *> ArgsV;
			builder.CreateCall(p_init, ArgsV, "");
		}
		errs() << "init!\n";
	}

	void storeLog(Instruction &I) {
		StoreInst * st = dyn_cast<StoreInst>(&I);

		if(st->getDebugLoc().get() != NULL) {
			int loc = st->getDebugLoc().getLine();
			Value * var = st->getPointerOperand();
			Value * val = st->getOperand(0);

			IRBuilder<> builder(st);
			Value * varAddr = builder.CreateBitCast(st->getOperand(1), ptrTy);

			Value* args[] = {
				ConstantInt::get(intTy, loc, false), // line number
				builder.CreateGlobalStringPtr(FName, ""), // variable name
				builder.CreateGlobalStringPtr(st->getOpcodeName(), ""), // store
				//builder.CreateGlobalStringPtr(st->getOpcodeName(), ""),
				varAddr
//					val // value being stored
			};

			builder.CreateCall(p_probe, args);
		}
	}

	void loadLog(BasicBlock &B, Instruction &I) {
		LoadInst * ld = dyn_cast<LoadInst>(&I);
		if(ld->getDebugLoc().get() != NULL) {
			int loc = ld->getDebugLoc().getLine() ;
			Value * var = ld->getPointerOperand() ;
			Value * val = ld->getOperand(0) ;
			string s;

			//MDNode * md = ld->getMetadata(llvm::lltok::Kind::LocalVar);

			//if(val->getType() != intTy) errs() << "not int!\n";
			//errs() << val->getType()->(llvm::raw_stdout_ostream, true, false) << "\n";
			//std::string type_str;
			//llvm::raw_string_ostream rso(type_str);
			//val->getType()->print(rso);
			//errs()<<rso.str() << "\n";

			IRBuilder<> builder(ld);
			builder.SetInsertPoint(&B, ++builder.GetInsertPoint());

			Value * varAddr = builder.CreateBitCast(val, ptrTy);

			Value* args[] = {
				ConstantInt::get(intTy, loc, false),
				builder.CreateGlobalStringPtr(FName, ""),
				builder.CreateGlobalStringPtr(ld->getOpcodeName(), ""),
				varAddr
//				ld // value being loaded
			};

			builder.CreateCall(p_probe, args);
		}
	}

	void lockLog(InvokeInst *inv) {
		int loc = inv->getDebugLoc().getLine();
		Value * var = inv->getArgOperand(1); 
		StringRef argName = "_noName_";
		if(var->hasName())
			argName = var->getName();

		IRBuilder<> builder(inv);

		Value * varAddr = builder.CreateBitCast(var, ptrTy);

		Value* args[] = {
			ConstantInt::get(intTy, loc, false), // line number
			builder.CreateGlobalStringPtr(FName, ""), // variable name
			builder.CreateGlobalStringPtr("lock", ""), // operation
			varAddr
		};

		builder.CreateCall(p_probe_lock, args);
	}

	void unlockLog(InvokeInst *inv) {
	}

	void rwlockReadLog(InvokeInst *inv) {
		int loc = inv->getDebugLoc().getLine();
		Value * var = inv->getArgOperand(0);

		StringRef argName = "_noName_";
		if(var->hasName())
			argName = var->getName();

		IRBuilder<> builder(inv);

		Value * varAddr = builder.CreateBitCast(var, ptrTy);

		Value* args[] = {
			ConstantInt::get(intTy, loc, false), // line number
			builder.CreateGlobalStringPtr(FName, ""), // variable name
			builder.CreateGlobalStringPtr("rwlock read lock", ""), // operation
			varAddr
		};

		builder.CreateCall(p_probe_lock, args);
	}

	void rwlockWriteLog(InvokeInst *inv) {
		int loc = inv->getDebugLoc().getLine();
		Value * var = inv->getArgOperand(1);

		StringRef argName = "_noName_";
		if(var->hasName())
			argName = var->getName();

		IRBuilder<> builder(inv);

		Value * varAddr = builder.CreateBitCast(var, ptrTy);

		Value* args[] = {
			ConstantInt::get(intTy, loc, false), // line number
			builder.CreateGlobalStringPtr(FName, ""), // variable name
			builder.CreateGlobalStringPtr("rwlock write lock" , ""), // operation
			varAddr
		};

		builder.CreateCall(p_probe_lock, args);
	}
};


} // namespace

// -----------------------------------------------------------------------------------

llvm::PassPluginLibraryInfo getHelloWorldPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HelloWorld", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hello-world") {
                    FPM.addPass(HelloWorld());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getHelloWorldPluginInfo();
}
