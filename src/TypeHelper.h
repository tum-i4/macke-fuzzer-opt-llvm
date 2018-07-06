
#ifndef __TYPE_HELPER_H
#define __TYPE_HELPER_H

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

llvm::Type* GetSizeType(llvm::Module* module);
llvm::Type* GetInt32Type(llvm::Module* module);
llvm::Type* GetInt8Type(llvm::Module* module);
llvm::Type* GetInt8PtrType(llvm::Module* module);

size_t GetTypeSize(const llvm::DataLayout* layout, llvm::Type* type);

llvm::FunctionType* GetFuzzDriverType(llvm::Module* module);
llvm::FunctionType* GetFuzzInputGeneratorType(llvm::Module* module);

llvm::ConstantInt* GetSize(uint64_t size, llvm::Module* module, llvm::IRBuilder<>* builder);

#endif // __TYPE_HELPER_H
