
#ifndef __FUZZ_INPUT_GENERATORS_H
#define __FUZZ_INPUT_GENERATORS_H

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>

/* Returns nullptr when function with generatorName already exists */
llvm::Function* DeclareFuzzInputGenerator(llvm::Module* module, const std::string& generatorName);
llvm::Function* CreateFuzzInputGenerator(llvm::Module* module, llvm::Function* fuzzFunction, const std::string& generatorName);



/* Get a working input generator for a given function. Checks whether one exists already and reuses one in this case, otherwise creates a new function */
llvm::Function* GetInputGeneratorForFunction(llvm::Module* module, llvm::Function* fuzzFunction);

#endif // __FUZZ_INPUT_GENERATORS_H
