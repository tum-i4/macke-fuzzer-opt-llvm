
#ifndef __FUZZ_DRIVER_H
#define __FUZZ_DRIVER_H

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>

/* Returns whether a function is suitable for fuzzing */
bool CanBeFuzzed(const llvm::Function* function);

/* Returns nullptr when function with driverName already exists */
llvm::Function* DeclareFuzzDriver(llvm::Module* module, const std::string &driverName);
llvm::Function* CreateFuzzDriverFor(llvm::Module* module, llvm::Function* fuzzFunction, const std::string& driverName);


#endif // __FUZZ_DRIVER_H
