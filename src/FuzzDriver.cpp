


#include <assert.h>
#include <llvm/Support/raw_ostream.h>


#include "Compat.h"
#include "FuzzDriver.h"
#include "FunctionDeclarations.h"
#include "TypeHelper.h"




/* Returns whether a function is suitable for fuzzing */
bool CanBeFuzzed(const llvm::Function* function)
{
	/* Check whether the function is external (empty) */
	if(function->empty())
		return false;

	/* If function has no arguments, we cannot fuzz it (yet) */
	if(function->arg_empty())
		return false;


	/* Check arguments and check if there
	 * exists at least one non-sret argument */
	bool nonSret = false;
	for(auto& arg : GetFunctionArgumentList(function))
	{
		if(!arg.hasStructRetAttr())
		{
			nonSret = true;
			/* If we can not deduce the argument names, dont fuzz */
			if(GetArgumentName(function->getParent(), &arg).empty())
			{
				llvm::errs() << "Warning: Function '" << function->getName() << "' can not be fuzzed because argument names can not be deduced.\n";
				return false;
			}
		}

		llvm::Type* elemType = arg.getType();
		while(elemType->isPointerTy())
			elemType = elemType->getPointerElementType();
		if(elemType->isStructTy())
		{
			if(elemType->getStructName().startswith("pthread"))
				return false;
		}
		if(arg.getType()->isPointerTy())
		{
			if(arg.getType()->getPointerElementType()->isPointerTy())
				return false;
			if(arg.getType()->getPointerElementType()->isFunctionTy())
				return false;
		}
	}

	return nonSret;
}



/**
 * Takes as argument the value for the Data and Size, and writes back the new pointers to the remaining Data and Size
 */
llvm::Value* PrepareArgumentInstruction(
		llvm::Module* module, llvm::BasicBlock** currentBlock, llvm::Function* function,
		llvm::DataLayout* dataLayout, llvm::Type* type,
		llvm::Value* bufRef, llvm::Value* remainingSizeRef,
		llvm::Function* _memcpy, llvm::Function* _memset,
		llvm::Function* _malloc,
		std::vector<llvm::Value*>& saved_mallocs)
{
	/* Builder for beginning */
	llvm::IRBuilder<> beginBuilder(*currentBlock);


	/* Handle pointers and non-pointers differently */
	if(type->isPointerTy())
	{
		/**
		 * Pointer-types are interpreted as arrays.
		 * The passed array will be dynamically allocated by malloc, as the target function could potentially free it,
		 * and if it wouldn't be malloced the function would crash, even though this very likely is no vulnerability.
		 */
		llvm::Value* buf = beginBuilder.CreateLoad(bufRef);
		llvm::Value* remainingSize = beginBuilder.CreateLoad(remainingSizeRef);

		/* Calculate the size of the element in the array*/
		llvm::Value* typeSize = GetSize(GetTypeSize(dataLayout, type->getPointerElementType()), module, &beginBuilder);

		/* Calculate bufferSize */
		llvm::Function* bytesizeFunc = declare_macke_fuzzer_array_byte_size(module);
		llvm::Value* arrayReservedSpace = beginBuilder.CreateCall(bytesizeFunc, llvm::ArrayRef<llvm::Value*>{std::vector<llvm::Value*>{buf, remainingSize}});
		llvm::Value* bufferSize = beginBuilder.CreateMul(beginBuilder.CreateUDiv(arrayReservedSpace, typeSize), typeSize);

		/* Allocate buffer and save it in saved_mallocs */
		llvm::Value* malloced_buffer = beginBuilder.CreateCall(_malloc, bufferSize);
		saved_mallocs.push_back(malloced_buffer);

		/* Fill the buffer and calculate new buf and remainingSize for next argument */
		llvm::Function* extractFunc = declare_macke_fuzzer_array_extract(module);
		llvm::Value* newBuf = beginBuilder.CreateCall(extractFunc, llvm::ArrayRef<llvm::Value*>{
				std::vector<llvm::Value*>{buf, remainingSize, malloced_buffer, bufferSize}});
		llvm::Value* diff = beginBuilder.CreatePtrDiff(newBuf, buf);
		llvm::Value* newSize = beginBuilder.CreateSub(remainingSize, diff);

		/* Store newBuf and newSize */
		beginBuilder.CreateStore(newBuf, bufRef);
		beginBuilder.CreateStore(newSize, remainingSizeRef);

		return beginBuilder.CreateBitCast(malloced_buffer, type);
	}
	else
	{
		/* Allocate storage for argument */
		llvm::AllocaInst* alloc = beginBuilder.CreateAlloca(type);

		/**
		 * Last block which loads the variable, which is where the last branches should all branch to
		 * This is simply a load from the storage in the stack
		 */
		llvm::BasicBlock* loadBlock = llvm::BasicBlock::Create(module->getContext(), "loadVar", function);
		llvm::IRBuilder<> loadBuilder(loadBlock);

		/* Calculate the size of the argument */
		llvm::Value* typeSize = GetSize(GetTypeSize(dataLayout, type), module, &beginBuilder);

		llvm::Value* sizeCompare = beginBuilder.CreateICmpUGE(typeSize, beginBuilder.CreateLoad(remainingSizeRef));

		/**
		 * Create true branch
		 * In case the fuzzer Data does not contain enough bytes for the argument,
		 * zero everything out and memcpy everything that fits
		 *
		 * Finally, set the remainingSize to zero
		 */
		llvm::BasicBlock* tbBlock = llvm::BasicBlock::Create(module->getContext(), "if.true", function, loadBlock);
		llvm::IRBuilder<> tbBuilder(tbBlock);

		/* Zero everything out */
		tbBuilder.CreateCall(_memset, llvm::ArrayRef<llvm::Value*>{
				std::vector<llvm::Value*>{
						tbBuilder.CreateBitCast(alloc, tbBuilder.getInt8Ty()->getPointerTo()),
						tbBuilder.getInt32(0),
						typeSize}});


		/* Copy the remaining content */
		tbBuilder.CreateCall(_memcpy, llvm::ArrayRef<llvm::Value*>{
				std::vector<llvm::Value*>{
						tbBuilder.CreateBitCast(alloc, tbBuilder.getInt8Ty()->getPointerTo()),
						tbBuilder.CreateLoad(bufRef), tbBuilder.CreateLoad(remainingSizeRef)}});

		/* Set remaining size to 0 */
		tbBuilder.CreateStore(GetSize(0, module, &tbBuilder), remainingSizeRef);

		/* End the basic block with branch to loadBlock */
		tbBuilder.CreateBr(loadBlock);

		/**
		 * Create false branch
		 * In case the fuzzer Data does contain enough bytes for the argument,
		 * memcpy everything
		 */
		llvm::BasicBlock* fbBlock = llvm::BasicBlock::Create(module->getContext(), "if.false", function, loadBlock);
		llvm::IRBuilder<> fbBuilder(fbBlock);

		/* Copy the content */
		fbBuilder.CreateCall(_memcpy, llvm::ArrayRef<llvm::Value*>{
				std::vector<llvm::Value*>{
						fbBuilder.CreateBitCast(alloc, fbBuilder.getInt8Ty()->getPointerTo()),
						fbBuilder.CreateLoad(bufRef), typeSize}});

		/* Advance buf and remainingSize */
		fbBuilder.CreateStore(fbBuilder.CreateGEP(fbBuilder.CreateLoad(bufRef), typeSize), bufRef);
		fbBuilder.CreateStore(fbBuilder.CreateSub(fbBuilder.CreateLoad(remainingSizeRef), typeSize), remainingSizeRef);

		/* End the basic block with branch to loadBlock */
		fbBuilder.CreateBr(loadBlock);

		/* Create branch condition */
		beginBuilder.CreateCondBr(sizeCompare, tbBlock, fbBlock);
		*currentBlock = loadBlock;
		/* Return the load instruction */
		return loadBuilder.CreateLoad(alloc);
	}
}


llvm::Function* DeclareFuzzDriver(llvm::Module* module, const std::string &driverName)
{
	/* Check if function with driverName exists already */
	if(module->getFunction(driverName))
		return nullptr;

	/* Create the function */
	llvm::Function* func = llvm::cast<llvm::Function>(module->getOrInsertFunction(
		driverName, GetFuzzDriverType(module)));

	/* Mark the new function as a basic c function */
	func->setCallingConv(llvm::CallingConv::C);

	return func;
}


/* Returns nullptr when function with driverName already exists */
llvm::Function* CreateFuzzDriverFor(llvm::Module* module, llvm::Function* fuzzFunction, const std::string &driverName)
{
	assert(fuzzFunction);

	/* Declare driver */
	llvm::Function* driver = DeclareFuzzDriver(module, driverName);

	if(!driver)
		return nullptr;


	/* Get declarations of needed functions */
	llvm::Function* _malloc = declare_malloc(module);
	llvm::Function* _free = declare_free(module);
	llvm::Function* _memcpy = declare_memcpy(module);
	llvm::Function* _memset = declare_memset(module);

	/* Get driver arguments */
	llvm::Value* data;
	llvm::Value* size;
	{
		auto arg_it = driver->arg_begin();
		data = arg_it++;
		data->setName("Data");
		size = arg_it;
		size->setName("Size");
	}

	/* Array to save the args and mallocs */
	std::vector<llvm::Value*> fuzzArgs;
	std::vector<llvm::Value*> saved_mallocs;


	/* Create basic block and builder for the function */
	llvm::BasicBlock* latestBlock = llvm::BasicBlock::Create(module->getContext(), "", driver);



	/* Create DataLayout for size calculations */
	llvm::DataLayout dataLayout(module);

	/* Create builder for beginning */
	llvm::IRBuilder<> beginBuilder(latestBlock);

	/* Save arguments on stack so we can adjust them */
	llvm::Value* dataRef = beginBuilder.CreateAlloca(data->getType());
	llvm::Value* sizeRef = beginBuilder.CreateAlloca(size->getType());

	beginBuilder.CreateStore(data, dataRef);
	beginBuilder.CreateStore(size, sizeRef);

	/* Create instructions for arguments */
	for(auto& argument : GetFunctionArgumentList(fuzzFunction))
	{
		/* Do not fill sret arguments */
		if(argument.hasStructRetAttr())
		{
			llvm::IRBuilder<> sretBuilder(latestBlock);
			fuzzArgs.push_back(sretBuilder.CreateAlloca(argument.getType()->getPointerElementType()));
			continue;
		}
		fuzzArgs.push_back(PrepareArgumentInstruction(
					module, &latestBlock, driver,
					&dataLayout, argument.getType(),
					dataRef, sizeRef,
					_memcpy, _memset,
					_malloc, saved_mallocs));
	}

	/* Create builder for last call and ret */
	llvm::IRBuilder<> endBuilder(latestBlock);

	/* Call target function */
	endBuilder.CreateCall(fuzzFunction, llvm::ArrayRef<llvm::Value*>(fuzzArgs));

	/* Free everything alloced */
	for(auto& malloc : saved_mallocs)
		endBuilder.CreateCall(_free, malloc);

	/* Driver always returns 0 */
	endBuilder.CreateRet(endBuilder.getInt32(0));

	return driver;
}

