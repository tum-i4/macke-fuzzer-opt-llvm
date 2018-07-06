
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <sstream>

#include <klee/Internal/ADT/KTest.h>

#include "Compat.h"
#include "TypeHelper.h"

#define MIN(a,b)   (((a) < (b)) ? (a) : (b))

/**
 * Helper pass to extract a ktest file from an inputfile and the functionname
 */

/* Extern helper function declarations */
extern "C" {
	extern size_t macke_fuzzer_array_byte_size(const uint8_t* src, size_t max);
	extern const uint8_t* macke_fuzzer_array_extract(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t dstLen);
}

namespace
{

	static llvm::cl::opt<std::string> KTestFunction(
		"ktestfunction",
		llvm::cl::desc("Name of the function the ktest should be generated for"));

	static llvm::cl::opt<std::string> KTestInputFile(
		"ktestinputfile",
		llvm::cl::desc("Path to the fuzzer-input file containing the data"));

	static llvm::cl::opt<std::string> KTestOut(
		"ktestout",
		llvm::cl::desc("Where to save the ktest file"));

	static llvm::cl::list<std::string> KleeArgs(
		"kleeargs",
		llvm::cl::desc("KleeArgs that should be saved in the ktest file"));

	struct KTestGenerator : public llvm::ModulePass
	{
		static char ID; /* Used for pass registration */

		KTestGenerator() : llvm::ModulePass(ID) { };

		bool runOnModule(llvm::Module &M) override;
	};


	bool KTestGenerator::runOnModule(llvm::Module &M)
	{
		assert(kTest_getCurrentVersion() == 3);

		/* Check all the command line arguments */
		if(KTestFunction.empty())
		{
			llvm::errs() << "Error: -ktestfunction parameter is needed!\n";
			return false;
		}

		if(KTestInputFile.empty())
		{
			llvm::errs() << "Error: -ktestinputfile parameter is needed!\n";
			return false;
		}

		if(KTestOut.empty())
		{
			llvm::errs() << "Error: -ktestout parameter is needed!\n";
			return false;
		}


		/* Look for the function */
		llvm::Function* backgroundFunc = M.getFunction(KTestFunction);

		// Check if the function given by the user really exists
		if(backgroundFunc == nullptr)
		{
			llvm::errs() << "Error: " << KTestFunction
			             << " is no function inside the module. " << '\n'
			             << "ktest generation is not possible!" << '\n';
			return false;
		}

		/* Create and initialize the ktest object */
		KTest* newKTest = (KTest*)malloc(sizeof(KTest));
		newKTest->symArgvs = 0;
		newKTest->symArgvLen = 0;
		newKTest->version = kTest_getCurrentVersion();
		newKTest->numArgs = KleeArgs.size();

		/* Allocate space for args */
		newKTest->args = (char**)malloc(sizeof(char*) * newKTest->numArgs);

		std::string arg;

		/* Copy the arguments into the kTest object */
		for(unsigned i = 0; i < newKTest->numArgs; ++i)
		{
			arg = KleeArgs[i];

			newKTest->args[i] = (char*)malloc(arg.size() + 1);
			memcpy(newKTest->args[i], arg.c_str(), arg.size());

			/* Terminating 0 byte */
			newKTest->args[i][arg.size()] = 0;
		}

		/* Read the input file into buffer */
		std::string inputBuffer;
		{
			std::ifstream instream_input(KTestInputFile);
			std::stringstream input_stream;
			input_stream << instream_input.rdbuf();
			inputBuffer = input_stream.str();
		}

		size_t remaining = inputBuffer.size();
		const uint8_t* current = (const uint8_t*)inputBuffer.c_str();

		/* Create DataLayout for size calculations */
		llvm::DataLayout dataLayout(&M);

		/* Allocate array for KTestObjects */
		newKTest->numObjects = backgroundFunc->arg_size();
		newKTest->objects = (KTestObject*)malloc(sizeof(KTestObject) * backgroundFunc->arg_size());
		KTestObject* obj = newKTest->objects;


		/* Fill KTestObjects for arguments */
		for(auto& arg : GetFunctionArgumentList(backgroundFunc))
		{
			if(arg.hasStructRetAttr())
				continue;

			std::string argName = GetArgumentName(&M, &arg);

			/* Copy the argument-name into the obj */
			obj->name = (char*)malloc(argName.size() + 1);
			memcpy(obj->name, argName.c_str(), argName.size());
			obj->name[argName.size()] = 0;

			if(argName.size() == 0)
				abort();


			/** Unpack the argument and save it into the object **/
			/* Pointers and non-pointers are treated differently */
			if(arg.getType()->isPointerTy())
			{
				size_t typeSize = GetTypeSize(&dataLayout, arg.getType()->getPointerElementType());
				size_t byteSize = macke_fuzzer_array_byte_size(current, remaining);

				/* Round down */
				obj->numBytes = (byteSize / typeSize) * typeSize;
				obj->bytes = (unsigned char*)malloc(obj->numBytes);
				const uint8_t* newCurrent = macke_fuzzer_array_extract(current, remaining, obj->bytes, obj->numBytes);
				remaining -= newCurrent - current;
				current = newCurrent;
			}
			else
			{
				size_t typeSize = GetTypeSize(&dataLayout, arg.getType());
				obj->numBytes = typeSize;
				/* 0 initialized */
				obj->bytes = (unsigned char*)calloc(1, obj->numBytes);

				size_t copyBytes = MIN(remaining, typeSize);
				memcpy(obj->bytes, current, copyBytes);
				current += copyBytes;
				remaining -= copyBytes;
			}

			/* Point to the next object */
			obj++;
		}


		/* Output the KTest to file */
		if(!kTest_toFile(newKTest, KTestOut.c_str()))
			llvm::errs() << "Unspecified error in kTest_toFile!\n";

		kTest_free(newKTest);

		return false;
	}

char KTestGenerator::ID = 0; /* Value is ignored */

/* Register the new pass */
static llvm::RegisterPass<KTestGenerator> X(
	"generate-ktest", "Create a ktest file for a macke-fuzzer-input",
	false, /* Does not only look at CFG */
	true   /* Is only analysis */
	);

}
