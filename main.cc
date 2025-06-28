#include <iostream>
#include <memory>
#include <vector>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

int main() {
    LLVMContext Context;
    IRBuilder<> Builder(Context);
    auto TheModule = std::make_unique<Module>("branching example", Context);

    // Declare printf
    FunctionType *printfType = FunctionType::get(
        IntegerType::getInt32Ty(Context), PointerType::get(Type::getInt8Ty(Context), 0), true
    );
    FunctionCallee printfFunc = TheModule->getOrInsertFunction("printf", printfType);

    // Create main function
    FunctionType *funcType = FunctionType::get(Type::getInt32Ty(Context), false);
    Function *mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", TheModule.get());
    BasicBlock *entry = BasicBlock::Create(Context, "entry", mainFunc);
    BasicBlock *evenBlock = BasicBlock::Create(Context, "even", mainFunc);
    BasicBlock *oddBlock = BasicBlock::Create(Context, "odd", mainFunc);
    BasicBlock *mergeBlock = BasicBlock::Create(Context, "merge", mainFunc);

    Builder.SetInsertPoint(entry);

    // int x = 5;
    AllocaInst *xAlloca = Builder.CreateAlloca(Type::getInt32Ty(Context), nullptr, "x");
    Builder.CreateStore(ConstantInt::get(Context, APInt(32, 5)), xAlloca);
    Value *xVal = Builder.CreateLoad(Type::getInt32Ty(Context), xAlloca);

    // if (x % 2 == 0)
    Value *mod = Builder.CreateSRem(xVal, ConstantInt::get(Context, APInt(32, 2)), "modtmp");
    Value *cond = Builder.CreateICmpEQ(mod, ConstantInt::get(Context, APInt(32, 0)), "cmptmp");
    Builder.CreateCondBr(cond, evenBlock, oddBlock);

    // even block
    Builder.SetInsertPoint(evenBlock);
    Value *evenStr = Builder.CreateGlobalStringPtr("Even\\n");
    Builder.CreateCall(printfFunc, {evenStr});
    Builder.CreateBr(mergeBlock);

    // odd block
    Builder.SetInsertPoint(oddBlock);
    Value *oddStr = Builder.CreateGlobalStringPtr("Odd\\n");
    Builder.CreateCall(printfFunc, {oddStr});
    Builder.CreateBr(mergeBlock);

    // merge
    Builder.SetInsertPoint(mergeBlock);
    Builder.CreateRet(ConstantInt::get(Context, APInt(32, 0)));

    // Run mem2reg optimization
    legacy::FunctionPassManager fpm(TheModule.get());
    fpm.add(createPromoteMemoryToRegisterPass());
    fpm.doInitialization();
    fpm.run(*mainFunc);

    // Print IR
    TheModule->print(outs(), nullptr);

    return 0;
}
