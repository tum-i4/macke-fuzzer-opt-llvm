
#ifndef __FUNCTION_DECLARATIONS_H
#define __FUNCTION_DECLARATIONS_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

llvm::Function* declare_malloc(llvm::Module* module);
llvm::Function* declare_free(llvm::Module* module);
llvm::Function* declare_strcmp(llvm::Module* module);
llvm::Function* declare_memcpy(llvm::Module* module);
llvm::Function* declare_memset(llvm::Module* module);

llvm::Function* declare_macke_fuzzer_array_byte_size(llvm::Module* module);
llvm::Function* declare_macke_fuzzer_array_extract(llvm::Module* module);

#endif // __FUNCTION_DECLARATIONS_H
