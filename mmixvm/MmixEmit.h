#pragma once

#include <stdint.h>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include <llvm/IR/IRBuilder.h>
#include "MemAccess.h"

namespace MmixLlvm {
	namespace Private {
		struct RegisterRecord {
			llvm::Value* value;

			bool changed;
		};

		typedef boost::unordered_map<uint8_t, RegisterRecord> RegistersMap;

		extern llvm::Value* emitAdjust64Endianness(llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitAdjust32Endianness(llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitAdjust16Endianness(llvm::IRBuilder<>& builder, llvm::Value* val);

		extern llvm::Value* emitRegisterLoad(llvm::LLVMContext& ctx, 
			llvm::IRBuilder<>& builder, llvm::Value* regGlob, RegistersMap& regMap, uint8_t reg);

		extern llvm::Value* emitGetTextPtr(llvm::LLVMContext& ctx,
			llvm::Module& m, llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::PointerType* ty);

		extern llvm::Value* emitGetDataPtr(llvm::LLVMContext& ctx,
			llvm::Module& m, llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::PointerType* ty);

		extern llvm::Value* emitGetPoolPtr(llvm::LLVMContext& ctx, llvm::Module& m,
			llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::PointerType* ty);

		extern llvm::Value* emitGetStackPtr(llvm::LLVMContext& ctx, llvm::Module& m,
			llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::PointerType* ty);

		extern llvm::Value* emitFetchMem(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::Type* ty, llvm::BasicBlock* exit);

		extern void emitStoreMem(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::IRBuilder<>& builder, llvm::Value* theA, llvm::Value* val, llvm::BasicBlock* exit);

		extern llvm::BasicBlock* emitLdo(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitLdt(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern llvm::BasicBlock* emitLdw(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern llvm::BasicBlock* emitLdb(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern llvm::BasicBlock* emitLdoi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitLdti(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern llvm::BasicBlock* emitLdwi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f, 
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern llvm::BasicBlock* emitLdbi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg, bool isSigned);

		extern llvm::BasicBlock* emitLdht(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitLdhti(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitSto(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStoi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStt(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStti(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStw(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStwi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStb(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStbi(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitSttu(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStwu(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStbu(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitSttui(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStwui(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);

		extern llvm::BasicBlock* emitStbui(llvm::LLVMContext& ctx, llvm::Module& m, llvm::Function& f,
			llvm::BasicBlock* entry, RegistersMap& regMap, uint8_t xarg, uint8_t yarg, uint8_t zarg);
	};

	typedef std::vector<uint64_t> EdgeList;

	boost::tuple<llvm::Function*, EdgeList> emitSimpleVertice(llvm::LLVMContext& ctx, llvm::Module& m, MmixLlvm::MemAccessor& ma, uint64_t xPtr);
};