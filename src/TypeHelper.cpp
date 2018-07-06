
#include <llvm/ADT/Triple.h>

#include "TypeHelper.h"

llvm::Type* GetSizeType(llvm::Module* module)
{
	return llvm::Triple(module->getTargetTriple()).isArch64Bit() ? llvm::Type::getInt64Ty(module->getContext())
	                                                             : llvm::Type::getInt32Ty(module->getContext());
}

llvm::Type* GetInt32Type(llvm::Module* module)
{
	return llvm::Type::getInt32Ty(module->getContext());
}

llvm::Type* GetInt8Type(llvm::Module* module)
{
	return llvm::Type::getInt8Ty(module->getContext());
}

llvm::Type* GetInt8PtrType(llvm::Module* module)
{
	return GetInt8Type(module)->getPointerTo();
}

size_t GetTypeSize(const llvm::DataLayout* layout, llvm::Type* type)
{
	size_t dlSize = layout->getTypeAllocSize(type);
	/* Void* will be treated as char* */
	if(dlSize == 0)
		return 1;
	return dlSize;
}

llvm::FunctionType* GetFuzzDriverType(llvm::Module* module)
{
	/**
	 * signature from libfuzzer documentation:
	 * int(const uint8_t*,size_t)
	 *
	 * "int" commonly equals "int32"
	 * const uint8* is i8* in llvm
	 * We get size_t type from GetSizeType function
	 */
	return llvm::FunctionType::get(GetInt32Type(module), llvm::ArrayRef<llvm::Type*>(std::vector<llvm::Type*>{GetInt8PtrType(module), GetSizeType(module)}), false);
}


llvm::FunctionType* GetFuzzInputGeneratorType(llvm::Module* module)
{
	/**
	 * signature:
	 * char*(size_t arrLen, size_t* retLen)
	 */
	return llvm::FunctionType::get(GetInt8PtrType(module), llvm::ArrayRef<llvm::Type*>(std::vector<llvm::Type*>{GetSizeType(module), GetSizeType(module)->getPointerTo()}), false);
}


llvm::ConstantInt* GetSize(uint64_t size, llvm::Module* module, llvm::IRBuilder<>* builder)
{
	return llvm::Triple(module->getTargetTriple()).isArch64Bit() ? builder->getInt64(size)
	                                                             : builder->getInt32(size);
}
