
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>

#include "Config.h"

/**
 * Pass to set the SanitizeAddr flag,
 * so that we are able to add asan-instruction to bitcode files with opt tool
 */

namespace
{

struct EnableAsan : public llvm::FunctionPass
{
	static char ID; /* Used for pass registration */

	EnableAsan() : llvm::FunctionPass(ID) { }

	bool runOnFunction(llvm::Function& F) override
	{
		F.addFnAttr(llvm::Attribute::SanitizeAddress);
		return true;
	}
};


char EnableAsan::ID = 0; /* Value is ignored */

/* Register the new pass */
static llvm::RegisterPass<EnableAsan> X(
	"enable-asan", "Add sanitize_addr attribute to all functions - use before instructing bc files with asan",
	false, /* Does not modify CFG */
	false  /* Is not only analysis */
	);

} /* Namespace */
