

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>

#include "Config.h"
#include "Compat.h"
#include "TypeHelper.h"
#include "FuzzDriver.h"
#include "FunctionDeclarations.h"

llvm::Function* DeclareFuzzInputGenerator(llvm::Module* module, const std::string& generatorName)
{
	/* Check if function with generatorName exists already */
	if(module->getFunction(generatorName))
		return nullptr;

	/* Create the function */
	llvm::Function* func = llvm::cast<llvm::Function>(module->getOrInsertFunction(
		generatorName, GetFuzzInputGeneratorType(module)));

	/* Mark the new function as a basic c function */
	func->setCallingConv(llvm::CallingConv::C);

	return func;
}


llvm::Function* CreateFuzzInputGenerator(llvm::Module* module, llvm::Function* fuzzFunction, const std::string& generatorName)
{
	assert(fuzzFunction);
	assert(CanBeFuzzed(fuzzFunction));

	/* Declare generator */
	llvm::Function* generator = DeclareFuzzInputGenerator(module, generatorName);

	if(!generator)
		return nullptr;


	/* Get declarations of needed functions */
	llvm::Function* _malloc = declare_malloc(module);
	llvm::Function* _memset = declare_memset(module);

	/* Get arguments */
	llvm::Value* arrLen;
	llvm::Value* retLen;
	{
		auto arg_it = generator->arg_begin();
		arrLen = arg_it++;
		arrLen->setName("arrLen");
		retLen = arg_it;
		retLen->setName("retLen");
	}

	/* Create basic block and builer for the function */
	llvm::BasicBlock* basicBlock = llvm::BasicBlock::Create(module->getContext(), "", generator);
	llvm::IRBuilder<> builder(basicBlock);

	/* Create DataLayout for size calculations */
	llvm::DataLayout dataLayout(module);

	llvm::Value* calcRetLen = GetSize(0, module, &builder);

	/* Calculate retLen */
	{
		bool first = true; /* On all arguments but the first we have to add the seperator to retLen */
		for(auto& argument : GetFunctionArgumentList(fuzzFunction))
		{
			/* Ignore sret arguments */
			if(argument.hasStructRetAttr())
				continue;

			if(argument.getType()->isPointerTy()) /* Array, add arrLen */
				calcRetLen = builder.CreateAdd(calcRetLen, arrLen);
			else /* Value, just add size */
				calcRetLen = builder.CreateAdd(calcRetLen, GetSize(GetTypeSize(&dataLayout, argument.getType()), module, &builder));
			if(first)
				first = false;
			else
				calcRetLen = builder.CreateAdd(calcRetLen, GetSize(1, module, &builder));
		}
	}

	/* Create array to return */
	llvm::Value* retArray = builder.CreateCall(_malloc, calcRetLen);

	/* Zero-initialize it */
	builder.CreateCall(_memset, llvm::ArrayRef<llvm::Value*>{
			std::vector<llvm::Value*>{
					retArray,
					builder.getInt32(0),
					calcRetLen}});


	/* Fill argument content */
	{
		llvm::Value* currentPtr = retArray;

		for(auto& argument : GetFunctionArgumentList(fuzzFunction))
		{
			if(argument.hasStructRetAttr())
				continue;

			if(argument.getType()->isPointerTy())
			{
				/* use memset and add arrLen */
				builder.CreateCall(_memset, llvm::ArrayRef<llvm::Value*>{
						std::vector<llvm::Value*>{
								currentPtr,
								builder.getInt32(INITIAL_INPUT_FILL_CHAR),
								arrLen}});

				currentPtr = builder.CreateGEP(currentPtr, arrLen);

				builder.CreateStore(builder.getInt8(DELIMITER_CHAR), currentPtr);
				currentPtr = builder.CreateGEP(currentPtr, builder.getInt8(1));
			}
			else
			{
				llvm::Value* typeSize = GetSize(GetTypeSize(&dataLayout, argument.getType()), module, &builder);
				/* use memset and add typesize */
				builder.CreateCall(_memset, llvm::ArrayRef<llvm::Value*>{
						std::vector<llvm::Value*>{
								currentPtr,
								builder.getInt32(INITIAL_INPUT_FILL_CHAR),
								typeSize}});

				currentPtr = builder.CreateGEP(currentPtr, typeSize);
			}
		}
	}

	/* Save len in retLen and return the array */
	builder.CreateStore(calcRetLen, retLen);

	/* Return the filled array */
	builder.CreateRet(retArray);

	return generator;
}



std::string GetInputGeneratorNameForFunction(llvm::Module* module, llvm::Function* function)
{
	std::string ret = FUNCTION_PREFIX "generator";
	/* Create DataLayout for size calculations */
	llvm::DataLayout dataLayout(module);

	for(auto& arg : GetFunctionArgumentList(function))
	{
		if(arg.hasStructRetAttr())
			continue;

		llvm::Type* elemType = arg.getType();
		if(elemType->isPointerTy())
			ret += "_ptr";
		else
		{
			ret += "_t";
			ret += GetTypeSize(&dataLayout, elemType);
		}
	}
	return ret;
}


llvm::Function* GetInputGeneratorForFunction(llvm::Module* module, llvm::Function* fuzzFunction)
{
	std::string generatorName = GetInputGeneratorNameForFunction(module, fuzzFunction);

	/* Check if function with generatorName exists already */
	llvm::Function* ret = module->getFunction(generatorName);
	if(ret)
		return ret;

	ret = CreateFuzzInputGenerator(module, fuzzFunction, generatorName);
	assert(ret);
	return ret;
}
