

#include "FunctionDeclarations.h"

#include "TypeHelper.h"
#include "Compat.h"


/* Wrapper around llvm function creation */
llvm::Function* declare_function(llvm::Module* module, const std::string name,
                                llvm::Type* returnTy,
                                std::vector<llvm::Type*> arguments) {
	/* Create the function, if it does not exist already */
	llvm::Function* func = llvm::cast<llvm::Function>(module->getOrInsertFunction(
		name, llvm::FunctionType::get(
			returnTy, llvm::ArrayRef<llvm::Type*>{arguments}, false)));

	/* Mark the new function as a basic c function */
	func->setCallingConv(llvm::CallingConv::C);

	return func;
}


/* declare noalias void* malloc(size_t) */
llvm::Function* declare_malloc(llvm::Module* module)
{
	llvm::Function* _malloc = declare_function(module, "malloc", GetInt8PtrType(module), {GetSizeType(module)});
	SetDoesNotAlias(_malloc);
	return _malloc;
}

/* declare void free(void*) */
llvm::Function* declare_free(llvm::Module* module)
{
	return declare_function(module, "free", llvm::Type::getVoidTy(module->getContext()), {GetInt8PtrType(module)});
}



/* declare int strcmp(const char*, const char*) */
llvm::Function* declare_strcmp(llvm::Module* module)
{
	return declare_function(module, "strcmp", GetInt32Type(module), {GetInt8PtrType(module), GetInt8PtrType(module)});
}



/* declare void* memcpy(void*, void*, size_t) */
llvm::Function* declare_memcpy(llvm::Module* module)
{
	return declare_function(module, "memcpy", GetInt8PtrType(module), {GetInt8PtrType(module), GetInt8PtrType(module), GetSizeType(module)});
}



/* declare void* memset(void*, int, size_t) */
llvm::Function* declare_memset(llvm::Module* module)
{
	return declare_function(module, "memset", GetInt8PtrType(module), {GetInt8PtrType(module), GetInt32Type(module), GetSizeType(module)});
}



llvm::Function* declare_macke_fuzzer_array_byte_size(llvm::Module* module)
{
	return declare_function(module, "macke_fuzzer_array_byte_size", GetSizeType(module), {GetInt8PtrType(module), GetSizeType(module)});
}
llvm::Function* declare_macke_fuzzer_array_extract(llvm::Module* module)
{
	return declare_function(module, "macke_fuzzer_array_extract", GetInt8PtrType(module), {GetInt8PtrType(module), GetSizeType(module), GetInt8PtrType(module), GetSizeType(module)});
}
