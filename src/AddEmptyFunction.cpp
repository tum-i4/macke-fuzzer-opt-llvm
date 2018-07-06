
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

/* Module to add empty body to otherwise undefined (external) function
 * Needed to add usage() function to libcoreutils testing */

namespace
{

static llvm::cl::opt<std::string> FunctionName(
		"function-name",
		llvm::cl::desc("Name of the function to add"));

struct AddEmptyFunction : public llvm::ModulePass
{
	static char ID;

	AddEmptyFunction() : llvm::ModulePass(ID) { };
	bool runOnModule(llvm::Module& M) override;
};


bool AddEmptyFunction::runOnModule(llvm::Module& M)
{
	if(FunctionName.empty())
	{
		llvm::errs() << "Error: -function-name argument missing\n";
		return false;
	}

	llvm::Function* f = M.getFunction(FunctionName);

	if(!f)
	{
		llvm::errs() << "Error: " << FunctionName << " is not included in the modules symbol table.\n";
		return false;
	}

	if(!f->empty())
	{
		llvm::errs() << "Error: " << FunctionName << " already has a body.\n";
		return false;
	}

	llvm::Type* retType = f->getReturnType();
	llvm::BasicBlock* retBlock = llvm::BasicBlock::Create(M.getContext(), "return", f);
	llvm::IRBuilder<> builder(retBlock);

	/* Add return */
	if(retType->isVoidTy())
		builder.CreateRetVoid();
	else
		builder.CreateRet(llvm::Constant::getNullValue(retType));

	return true;
}


char AddEmptyFunction::ID = 0; /* Value is ignored */

/* Register the new pass */
static llvm::RegisterPass<AddEmptyFunction> X(
	"add-empty-function", "Transform an external function to one with no implementation (returns null)",
	false, /* Does modify CFG */
	false  /* Is not only analysis */
	);

} /* Namespace */
