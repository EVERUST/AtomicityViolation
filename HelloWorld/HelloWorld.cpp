
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

struct HelloWorld : PassInfoMixin<HelloWorld> {

	bool initial = false;

	Type *intTy, *ptrTy, *voidTy, *boolTy;

	FunctionCallee p_init;
	FunctionCallee p_probe;
	FunctionCallee p_probe_lock;

	// Main entry point, takes IR unit to run the pass on (&F) and the
	// corresponding pass manager (to be queried if need be)
	PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

			return PreservedAnalyses::all();
		SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
		string file;
		string directory;
		F.getAllMetadata(MDs);
		for (auto &MD : MDs) {
		  if (MDNode *N = MD.second) {
			 if (auto *subProgram = dyn_cast<DISubprogram>(N)) {
				file = subProgram->getFilename().str();
				directory = subProgram->getDirectory().str();
				break;
			 }
		  }
		}

		string moduleName = F.getParent()->getName().str();
		string filename = moduleName.substr(0, moduleName.rfind("-"));
		string filePath = directory + "/" + file;

		string FName = F.getName().str();


		if(FName.find(filename) == string::npos || (FName.find("core") != string::npos || FName.find("std") != string::npos || directory.length() == 0 || directory.find("cargo") != string::npos)) {
			return PreservedAnalyses::all();
		}
		errs() << "FName: " << FName << "\n";
		errs() << "module name: " << moduleName << "\n";
		errs() << "filename: " << filename << "\n";
		errs() << "directory: " << directory << "\n\n" ;
		//errs() << "filePath: " << filePath << "\n\n";
			return PreservedAnalyses::all();


		if(!initial) {
			initialization(F);
			initial = true;
			errs() << "init\n" ;
		}

		for(auto& B : F) {
			for(auto& I : B) {
				if(I.getOpcode() == Instruction::Store) { // store
					StoreInst * st = dyn_cast<StoreInst>(&I);

					if(st->getDebugLoc().get() != NULL) {
						int loc = st->getDebugLoc().getLine();
						Value * var = st->getPointerOperand();
						Value * val = st->getOperand(0);
						//errs() << "==========store==========\n";
						//st->getOperand(0)->getType()->dump();
						//st->getOperand(1)->getType()->dump();
						//errs() << "==========================\n";

						IRBuilder<> builder(st);
						Value * varAddr = builder.CreateBitCast(st->getOperand(1), ptrTy);

						Value* args[] = {
							ConstantInt::get(intTy, loc, false), // line number
							builder.CreateGlobalStringPtr(filePath, ""), // filename
							builder.CreateGlobalStringPtr(st->getOpcodeName(), ""), // store
							//builder.CreateGlobalStringPtr(st->getOpcodeName(), ""),
							varAddr
			//					val // value being stored
						};

						builder.CreateCall(p_probe, args);
					}

				} else if(I.getOpcode() == Instruction::Load) { // load
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
							builder.CreateGlobalStringPtr(filePath, ""),
							builder.CreateGlobalStringPtr(ld->getOpcodeName(), ""),
							varAddr
			//				ld // value being loaded
						};

						builder.CreateCall(p_probe, args);
					}
				}
				else if(I.getOpcode() == Instruction::Invoke) {
					InvokeInst * inv = dyn_cast<InvokeInst>(&I);
					if(inv->getDebugLoc().get() != NULL) {
						string funcName = inv->getCalledFunction()->getName().str();

						//errs() << "invoke - " << funcName << "\n";
								
						if(funcName.find("std4sync5mutex14Mutex$LT$T$GT$4lock") != std::string::npos) { // invoke lock
							int loc = inv->getDebugLoc().getLine();
							Value * var = inv->getArgOperand(1); 
							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();
					
							IRBuilder<> builder(inv);

							Value * varAddr = builder.CreateBitCast(var, ptrTy);

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("lock", ""), // operation
								varAddr
							};

							builder.CreateCall(p_probe_lock, args);
						} else if(funcName.find("drop_in_place$LT$std..sync..mutex..MutexGuard") != std::string::npos) { // invoke unlock
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
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("unlock", ""), // operation
								varAddr
								//lock
							};
							builder.CreateCall(p_probe_lock, args);
						} else if(funcName.find("std4sync6rwlock15RwLock$LT$T$GT$4read") != std::string::npos) { // invoke rwlock read lock
							int loc = inv->getDebugLoc().getLine();
							Value * var = inv->getArgOperand(0);

							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();

							IRBuilder<> builder(inv);

							Value * varAddr = builder.CreateBitCast(var, ptrTy);

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("invoke - rwlock read lock", ""), // operation
								varAddr
							};

							builder.CreateCall(p_probe_lock, args);
						} else if(funcName.find("drop_in_place$LT$std..sync..rwlock..RwLockReadGuard") != std::string::npos) { // invoke rwlock read unlock
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
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("invoke - rwlock read unlock", ""), // operation
								varAddr
							};
							builder.CreateCall(p_probe_lock, args);
						} else if(funcName.find("std4sync6rwlock15RwLock$LT$T$GT$5write") != std::string::npos) { // invoke rwlock write lock
							//errs() << "invoke rwlock write lock - " << funcName << "\n";
							//for (auto iter = inv->arg_begin(); iter != inv->arg_end(); iter++) {
								//errs() << iter->get()->getName() <<"\n";
								//iter->get()->getType()->dump();
							//}
							//errs() << "-----------------------------------\n";
							int loc = inv->getDebugLoc().getLine();
							Value * var = inv->getArgOperand(1);

							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();

							IRBuilder<> builder(inv);

							Value * varAddr = builder.CreateBitCast(var, ptrTy);

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("invoke - rwlock write lock" , ""), // operation
								varAddr
							};

							builder.CreateCall(p_probe_lock, args);
						} else if(funcName.find("drop_in_place$LT$std..sync..rwlock..RwLockWriteGuard") != std::string::npos) { // invoke rwlock write unlock
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
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("invoke - rwlock write unlock", ""), // operation
								varAddr
							};
							builder.CreateCall(p_probe_lock, args);
						}
					}
				}
				else if(I.getOpcode() == Instruction::Call) { // call
					CallInst * call = dyn_cast<CallInst>(&I);

					if(call->getDebugLoc().get() != NULL) {
						string funcName = call->getCalledFunction()->getName().str();
								
						//errs() << "call - " << funcName << "\n";
						if(funcName.find("std4sync6rwlock15RwLock$LT$T$GT$5write") != std::string::npos) { // rwlock write lock
							int loc = call->getDebugLoc().getLine();
							Value * var = call->getArgOperand(1);

							//errs() << "call rwlock write lock - " << funcName << "\n";
							//for (auto iter = call->arg_begin(); iter != call->arg_end(); iter++) {
								//errs() << iter->get()->getName() <<"\n";
								//iter->get()->getType()->dump();
							//}

							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();

							IRBuilder<> builder(call);

							Value * varAddr = builder.CreateBitCast(var, ptrTy);

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("call - rwlock write lock" , ""), // operation
								varAddr
							};

							builder.CreateCall(p_probe_lock, args);
						} else if(funcName.find("std4sync6rwlock15RwLock$LT$T$GT$4read") != std::string::npos) { // rwlock read lock
							int loc = call->getDebugLoc().getLine();
							Value * var = call->getArgOperand(0);

							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();

							IRBuilder<> builder(call);

							Value * varAddr = builder.CreateBitCast(var, ptrTy);

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("call - rwlock read lock", ""), // operation
								varAddr
							};

							builder.CreateCall(p_probe_lock, args);

						} else if(funcName.find("drop_in_place$LT$std..sync..rwlock..RwLockReadGuard") != std::string::npos){ // rwlock read unlock
							int loc = call->getDebugLoc().getLine();
							Value * var = call->getArgOperand(0);

							//errs() << "call rwlock read unlock - " << funcName << "\n";
							//for (auto iter = call->arg_begin(); iter != call->arg_end(); iter++) {
								//errs() << iter->get()->getName() <<"\n";
								//iter->get()->getType()->dump();
							//}

							IRBuilder<> builder(call);

							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();

							Value * MutexGuard = builder.CreateLoad(var->getType()->getPointerElementType(), var);
							//MutexGuard->getType()->dump();

							Value * varAddr = builder.CreateBitCast(MutexGuard, ptrTy);
							//errs() << "\t\t\t rwlock read unlock\n";
							//varAddr->dump();
							//errs() << "-------------------------\n\n";

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("call - rwlock read unlock", ""), // operation
								varAddr
							};
							builder.CreateCall(p_probe_lock, args);
						} else if(funcName.find("drop_in_place$LT$std..sync..rwlock..RwLockWriteGuard") != std::string::npos) { // rwlock write unlock
							//errs() << "call write unlock Function Name: " << funcName << "\n";
							//for (auto iter = call->arg_begin(); iter != call->arg_end(); iter++) {
								//errs() << "arg type: ";
								//iter->get()->getType()->getPointerElementType()->dump();
							//}

							int loc = call->getDebugLoc().getLine();
							Value * var = call->getArgOperand(0); 
							StringRef argName = "_noName_";
							if(var->hasName())
								argName = var->getName();
					
							IRBuilder<> builder(call);

							Value * MutexGuard = builder.CreateLoad(var->getType()->getPointerElementType(), var);
							Value * lockPtr = builder.CreateStructGEP(MutexGuard->getType(), var, 0);
							Value * lock = builder.CreateLoad(lockPtr->getType()->getPointerElementType(), lockPtr);

							Value * varAddr = builder.CreateBitCast(lock, ptrTy);
							//errs() << "varAddr->dump()\n";
							//varAddr->dump();
							//errs() << "-------------------------\n\n";

							Value* args[] = {
								ConstantInt::get(intTy, loc, false), // line number
								builder.CreateGlobalStringPtr(filePath, ""), // variable name
								builder.CreateGlobalStringPtr("call - rwlock write unlock", ""), // operation
								varAddr
							};

							builder.CreateCall(p_probe_lock, args);
						}
						else if(funcName.find("core4sync6atomic") != std::string::npos && funcName.find("Atomic") != std::string::npos) { // atomic data type
							break ;
							if(funcName.find("load") != std::string::npos) { // atomic data type load
								int loc = call->getDebugLoc().getLine();
								Value * var = call->getArgOperand(0);
								Value * addr = call->getOperand(0);
								//errs() << "----------load----------\n";
								//val->dump();
								//val->getType()->dump();
								//errs() << "------------------------\n";

								StringRef argName = "_noName_";
								if(var->hasName())
									argName = var->getName();
								
								IRBuilder<> builder(call);
								builder.SetInsertPoint(&B, ++builder.GetInsertPoint());

								Value * varAddr = builder.CreateBitCast(addr, ptrTy);

								Value* args[] = {
									ConstantInt::get(intTy, loc, false),
									builder.CreateGlobalStringPtr(filePath, ""),
									builder.CreateGlobalStringPtr("call - atomic load", ""),
									varAddr
								};

								builder.CreateCall(p_probe, args);

							} else if(funcName.find("store") != std::string::npos) { // atomic data type store
								int loc = call->getDebugLoc().getLine();
								Value * var = call->getArgOperand(0);
								//errs() << "--------------store-----------------\n";
								//errs() << funcName << "\n";
								//var->dump();
								//errs() << "dumped\n";
								//errs() << "-----------------------------------\n";
								
								StringRef argName = "_noName_";
								if(var->hasName())
									argName = var->getName();
								
								IRBuilder<> builder(call);

								Value * varAddr = builder.CreateBitCast(var, ptrTy);
								//errs() << "varAddr: " << varAddr << "\n";
								//errs() << "-----------------------------------\n";

								Value* args[] = {
									ConstantInt::get(intTy, loc, false), // line number
									builder.CreateGlobalStringPtr(filePath, ""), // variable name
									builder.CreateGlobalStringPtr("call - atomic store", ""), // store
									varAddr
								};

								builder.CreateCall(p_probe, args);
							}
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
		//errs() << "init!\n";
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
