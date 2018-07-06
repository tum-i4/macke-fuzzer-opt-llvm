


#include "Compat.h"

#if LLVM_VERSION_MAJOR != 3
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/DebugInfoMetadata.h>
#endif

std::string GetArgumentName(const llvm::Module* M, const llvm::Argument* argument)
{
	std::string ret = "argno" + std::to_string(argument->getArgNo());
	return ret;
}
