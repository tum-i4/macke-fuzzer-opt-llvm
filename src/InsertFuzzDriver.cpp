
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>


#include "Compat.h"
#include "Config.h"
#include "TypeHelper.h"
#include "FuzzDriver.h"
#include "FuzzInputGenerators.h"

namespace {



static llvm::cl::opt<std::string> FuzzFunc(
	"fuzz-func",
	llvm::cl::desc("Function the driver should be generated for"));


struct InsertFuzzDriver : public llvm::ModulePass
{
	static char ID; /* Used for pass registration */

	InsertFuzzDriver() : llvm::ModulePass(ID) { };

	bool runOnModule(llvm::Module& M) override;
};

bool InsertFuzzDriver::runOnModule(llvm::Module& M)
{
	/* If no function is specified, generate one for all functions */
	if(FuzzFunc.empty())
	{
		/* Create global module builder */
		llvm::IRBuilder<> moduleBuilder(M.getContext());

		/* Declare driver and check if it exists */
		llvm::Function* fuzzDriver = DeclareFuzzDriver(&M, LibFuzzerDriverName);

		if(!fuzzDriver)
		{
			llvm::errs() << "Error: " << LibFuzzerDriverName
				          << " already exists in the module.\n";
			return false;
		}
		/* Create global driver description array type and vector to save entries*/
		llvm::FunctionType* driverType = GetFuzzDriverType(&M);
		llvm::FunctionType* generatorType = GetFuzzInputGeneratorType(&M);
		llvm::StructType* descStruct = llvm::StructType::create(
				llvm::ArrayRef<llvm::Type*>(
						std::vector<llvm::Type*>(
								{
									GetInt8PtrType(&M),
									driverType->getPointerTo(),
									generatorType->getPointerTo()
								})),
				DriverDescName);
		std::vector<llvm::Constant*> descEntries;

		/* Collect all functions that can be fuzzed */
		std::vector<llvm::Function*> fuzzingTargets;
		for(llvm::Function& f : GetModuleFunctionList(&M))
		{
			if(CanBeFuzzed(&f))
				fuzzingTargets.push_back(&f);
		}

		/* Create driver for each function and add an entry to the description array */
		std::vector<llvm::Function*> fuzzingDrivers;
		for(llvm::Function* f : fuzzingTargets)
		{
			f->addFnAttr(llvm::Attribute::NoInline);
			std::string driverName = FUNCTION_PREFIX "driver_";
			driverName += f->getName();
			llvm::Function* functionDriver = CreateFuzzDriverFor(&M, f, driverName);
			fuzzingDrivers.push_back(functionDriver);

			llvm::Constant* strConstant = llvm::ConstantDataArray::getString(M.getContext(), f->getName());
			llvm::GlobalVariable *strGlobal = new llvm::GlobalVariable(M, strConstant->getType(),
					true, llvm::GlobalValue::PrivateLinkage,
					strConstant);

			llvm::Function* inputGenerator = GetInputGeneratorForFunction(&M, f);

			SetUnnamedAddr(strGlobal);

			descEntries.push_back(llvm::ConstantStruct::get(descStruct,
					llvm::ArrayRef<llvm::Constant*>(
							std::vector<llvm::Constant*>(
								{
									llvm::ConstantExpr::getBitCast(strGlobal, GetInt8PtrType(&M)),
									functionDriver,
									inputGenerator
								}))));
		}

		/* Create global fuzzDriverPtr variable */
		llvm::GlobalVariable* driverPtr = new llvm::GlobalVariable(M, driverType->getPointerTo(), false,
				llvm::GlobalValue::ExternalLinkage, llvm::Constant::getNullValue(driverType->getPointerTo()), DriverPtrName);

		/* Create fuzzDriver method */
		{
			llvm::BasicBlock* funcBlock = llvm::BasicBlock::Create(M.getContext(), "", fuzzDriver);
			llvm::IRBuilder<> irBuilder(funcBlock);

			/* Values from args */
			std::vector<llvm::Value*> args;
			for(auto& arg : GetFunctionArgumentList(fuzzDriver))
				args.push_back(&arg);
			irBuilder.CreateRet(irBuilder.CreateCall(irBuilder.CreateLoad(driverPtr), llvm::ArrayRef<llvm::Value*>(args)));
		}

		/* Desc array is nullterminated, thus create null element */
		descEntries.push_back(llvm::Constant::getNullValue(descStruct));

		/* Create constant driverDescription array */
		llvm::ArrayType* descArrayType = llvm::ArrayType::get(descStruct, descEntries.size());
		llvm::Constant* descArrayInitializer = llvm::ConstantArray::get(descArrayType, llvm::ArrayRef<llvm::Constant*>(descEntries));
		llvm::GlobalVariable* driverDescriptions = new llvm::GlobalVariable(M, descArrayType, true,
				llvm::GlobalValue::ExternalLinkage, descArrayInitializer, DriverArrayName);
		(void)driverDescriptions;


		return true;
	}

	llvm::Function* targetFunction = M.getFunction(FuzzFunc);

	if(!targetFunction)
	{
		llvm::errs() << "Error: " << FuzzFunc
		             << " is no function inside the module.\n"
		             << "Fuzzing driver generation is not possible!\n";
		return false;
	}
	targetFunction->addFnAttr(llvm::Attribute::NoInline);

	if(!CreateFuzzDriverFor(&M, targetFunction, LibFuzzerDriverName))
	{
		llvm::errs() << "Error: fuzzing driver could not be generated!\n";
		return false;
	}

	return true;
}

char InsertFuzzDriver::ID = 0; /* Value is ignored */

/* Register the new pass */
static llvm::RegisterPass<InsertFuzzDriver> X(
	"insert-fuzzdriver", "Insert a libFuzzer driver for a specific function",
	false, /* Does modify CFG */
	false  /* Is not only analysis */
	);

} /* Namespace */

