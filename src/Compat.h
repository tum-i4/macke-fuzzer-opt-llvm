
#ifndef __COMPAT_H
#define __COMPAT_H


/* Compatibility functions for llvm3.4 support */

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalValue.h>

#if LLVM_VERSION_MAJOR == 3
inline auto&& GetFunctionArgumentList(llvm::Function* func)       { return func->getArgumentList(); }
inline auto&& GetModuleFunctionList(llvm::Module* M)              { return M->getFunctionList(); }
inline auto&& GetFunctionArgumentList(const llvm::Function* func) { return func->getArgumentList(); }
inline auto&& GetModuleFunctionList(const llvm::Module* M)        { return M->getFunctionList(); }

inline void SetDoesNotAlias(llvm::Function* func)                 { func->setDoesNotAlias(0); }
inline void SetUnnamedAddr(llvm::GlobalValue* gv)                 { gv->setUnnamedAddr(true); }
#else
inline auto GetFunctionArgumentList(llvm::Function* func)         { return func->args(); }
inline auto GetModuleFunctionList(llvm::Module* M)                { return M->functions(); }
inline auto GetFunctionArgumentList(const llvm::Function* func)   { return func->args(); }
inline auto GetModuleFunctionList(const llvm::Module* M)          { return M->functions(); }

inline void SetDoesNotAlias(llvm::Function* func)                 { func->setReturnDoesNotAlias(); }

inline void SetUnnamedAddr(llvm::GlobalValue* gv)                 { gv->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global); }
#endif

std::string GetArgumentName(const llvm::Module* M, const llvm::Argument* argument);


#endif // __COMPAT_H
