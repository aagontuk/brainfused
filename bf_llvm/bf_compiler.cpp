#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include <stack>
#include <string>
#include <iostream>
#include <fstream>

#define TAP_SIZE 1048576

using namespace llvm;

void compile(std::string code) {
    LLVMContext Context;
    Module *module = new Module("brainfused", Context);
    IRBuilder<> Builder(Context);

    // Create tap
    ArrayType *MemoryType = ArrayType::get(Type::getInt8Ty(Context), TAP_SIZE);
    GlobalVariable *Memory = new GlobalVariable(*module, MemoryType, false,
                                                GlobalValue::PrivateLinkage,
                                                Constant::getNullValue(MemoryType),
                                                "memory");
    Value *TapePtr = Builder.CreateGEP(MemoryType, Memory, {Builder.getInt32(0), Builder.getInt32(0)}, "tape_ptr");

    // Create main function
    FunctionType *FuncType = FunctionType::get(Type::getInt32Ty(Context), false);
    Function *MainFunc = Function::Create(FuncType, Function::ExternalLinkage, "main", module);
    BasicBlock *EntryBB = BasicBlock::Create(Context, "entry", MainFunc);
    Builder.SetInsertPoint(EntryBB);

    // putchar function
    FunctionType *PutcharType = FunctionType::get(Type::getInt32Ty(Context), {Type::getInt32Ty(Context)}, false);
    FunctionCallee PutcharFunc = module->getOrInsertFunction("putchar", PutcharType);

    AllocaInst *TapIndex = Builder.CreateAlloca(Type::getInt64Ty(Context), nullptr, "tap_index");
    Builder.CreateStore(Builder.getInt64(0), TapIndex);

    std::stack<BasicBlock *> loopStartStack;
    std::stack<BasicBlock *> loopEndStack;

    for (char cmd : code) {
        switch (cmd) {
            case '>': {
                Value *CurrIndex = Builder.CreateLoad(Type::getInt64Ty(Context), TapIndex, "load_index");
                CurrIndex = Builder.CreateAdd(CurrIndex, Builder.getInt64(1), "inc_index");
                Builder.CreateStore(CurrIndex, TapIndex);
                break;
            }

            case '<': {
                Value *CurrIndex = Builder.CreateLoad(Type::getInt64Ty(Context), TapIndex, "load_index");
                CurrIndex = Builder.CreateSub(CurrIndex, Builder.getInt64(1), "dec_index");
                Builder.CreateStore(CurrIndex, TapIndex);
                break;
            }

            case '+': {
                Value *CurrIndex = Builder.CreateLoad(Type::getInt64Ty(Context), TapIndex, "load_index");
                Value *Ptr = Builder.CreateGEP(MemoryType, TapePtr, {Builder.getInt64(0), CurrIndex}, "get_ptr");
                Value *Val = Builder.CreateLoad(Type::getInt8Ty(Context), Ptr, "load_val");
                Val = Builder.CreateAdd(Val, Builder.getInt8(1), "inc_val");
                Builder.CreateStore(Val, Ptr);
                break;
            }

            case '-': {
                Value *CurrIndex = Builder.CreateLoad(Type::getInt64Ty(Context), TapIndex, "load_index");
                Value *Ptr = Builder.CreateGEP(MemoryType, TapePtr, {Builder.getInt64(0), CurrIndex}, "get_ptr");
                Value *Val = Builder.CreateLoad(Type::getInt8Ty(Context), Ptr, "load_val");
                Val = Builder.CreateSub(Val, Builder.getInt8(1), "dec_val");
                Builder.CreateStore(Val, Ptr);
                break;
            }

            case '.': {
                Value *CurrIndex = Builder.CreateLoad(Type::getInt64Ty(Context), TapIndex, "load_index");
                Value *Ptr = Builder.CreateGEP(MemoryType, TapePtr, {Builder.getInt64(0), CurrIndex}, "get_ptr");
                Value *Val = Builder.CreateLoad(Type::getInt8Ty(Context), Ptr, "load_val");
                Value *Int32Byte = Builder.CreateSExt(Val, Type::getInt32Ty(Context), "int32byte");
                Builder.CreateCall(PutcharFunc, Int32Byte);
                break;
            }

            case ',':
                break;

            case '[': {
                BasicBlock *LoopStartBB = BasicBlock::Create(Context, "loop_start", MainFunc);
                BasicBlock *LoopEndBB = BasicBlock::Create(Context, "loop_end", MainFunc);

                Value *CurrIndex = Builder.CreateLoad(Type::getInt64Ty(Context), TapIndex, "load_index");
                Value *Ptr = Builder.CreateGEP(MemoryType, TapePtr, {Builder.getInt64(0), CurrIndex}, "get_ptr");
                Value *Val = Builder.CreateLoad(Type::getInt8Ty(Context), Ptr, "load_val");
                Value *Cond = Builder.CreateICmpEQ(Val, Builder.getInt8(0), "loopcond");
                Builder.CreateCondBr(Cond, LoopEndBB, LoopStartBB);

                Builder.SetInsertPoint(LoopStartBB);
                
                loopStartStack.push(LoopStartBB);
                loopEndStack.push(LoopEndBB);
                break;
            }

            case ']': {
                BasicBlock *LoopStartBB = loopStartStack.top();
                loopStartStack.pop();
                BasicBlock *LoopEndBB = loopEndStack.top();
                loopEndStack.pop();

                Value *CurrIndex = Builder.CreateLoad(Type::getInt64Ty(Context), TapIndex, "load_index");
                Value *Ptr = Builder.CreateGEP(MemoryType, TapePtr, {Builder.getInt64(0), CurrIndex}, "get_ptr");
                Value *Val = Builder.CreateLoad(Type::getInt8Ty(Context), Ptr, "load_val");
                Value *Cond = Builder.CreateICmpEQ(Val, Builder.getInt8(0), "loopcond");
                Builder.CreateCondBr(Cond, LoopEndBB, LoopStartBB);
                Builder.SetInsertPoint(LoopEndBB);
                break;
          }
        }
    }

    Builder.CreateRet(Builder.getInt32(0));

    auto res = llvm::verifyModule(*module, &llvm::errs());
    assert(!res);

    // Output the LLVM IR
    module->print(outs(), nullptr);
    delete module;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <brainfuck code>" << std::endl;
        return 1;
    }

    std::ifstream infile(argv[1]);
    std::string code((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    compile(code);

    return 0;
}

