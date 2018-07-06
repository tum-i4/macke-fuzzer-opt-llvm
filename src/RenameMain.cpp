
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>


namespace {

static llvm::cl::opt<std::string> NewMainName(
	"new-main-name",
	llvm::cl::desc("New name of the main function (defaults to ___old_main)"));


struct RenameMain : public llvm::ModulePass
{
	static char ID; /* Used for pass registration */

	RenameMain() : llvm::ModulePass(ID) { };

	bool runOnModule(llvm::Module& M) override;
};

bool RenameMain::runOnModule(llvm::Module& M)
{
	std::string newName;
	if(NewMainName.empty())
		newName = "___old_main";
	else
		newName = NewMainName;

	llvm::Function* main = M.getFunction("main");

	if(!main)
		return false; /* Tell llvm we didn't change the module */

	main->setName(newName);
	return true; /* Tell llvm the module was modified */
}

char RenameMain::ID = 0; /* Value is ignored */

/* Register the new pass */
static llvm::RegisterPass<RenameMain> X(
	"renamemain", "Change the name of the main function",
	false, /* Does not modify CFG */
	false  /* Is not only analysis */
	);

} /* Namespace */

