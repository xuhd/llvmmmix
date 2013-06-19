#include "stdafx.h"
#include "Util.h"
#include "MmixEmitPvt.h"
#include "MmixDef.h"

using llvm::LLVMContext;
using llvm::Module;
using llvm::Type;
using llvm::PointerType;
using llvm::Value;
using llvm::PHINode; 
using llvm::Function;
using llvm::BasicBlock;
using llvm::IRBuilder;
using llvm::ArrayRef;
using llvm::SwitchInst;
using llvm::cast;

using namespace MmixLlvm::Util;
using namespace MmixLlvm::Private;
using MmixLlvm::Private::RegisterRecord;
using MmixLlvm::Private::RegistersMap;
using MmixLlvm::MXByte;

namespace {
	template<class Cond> 
		struct EmitCond {
			static void emit(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate);
		};

		template<class Cond> void EmitCond<Cond>::emit(VerticeContext& vctx, 
			MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
		{
			LLVMContext& ctx = *vctx.Ctx;
			IRBuilder<> builder(ctx);
			builder.SetInsertPoint(vctx.Entry);
			BasicBlock *condTrueBlock = BasicBlock::Create(ctx, genUniq("cond_true"), vctx.Function);
			BasicBlock *condFalseBlock = BasicBlock::Create(ctx, genUniq("cond_false"), vctx.Function);
			BasicBlock *epilogue = BasicBlock::Create(ctx, genUniq("epilogue"), vctx.Function);
			Value* yarg0 = emitRegisterLoad(vctx, builder, yarg);
			Value* zarg0 =  immediate ? builder.getInt64(zarg) : emitRegisterLoad(vctx, builder, zarg);
			Value* cond0 = typename Cond::emitCond(builder, yarg0);
			builder.CreateCondBr(cond0, condTrueBlock, condFalseBlock);
			builder.SetInsertPoint(condTrueBlock);
			builder.CreateBr(epilogue);
			builder.SetInsertPoint(condFalseBlock);
			Value* defXval = typename Cond::emitDefXVal(vctx, builder, xarg);
			builder.CreateBr(epilogue);
			builder.SetInsertPoint(epilogue);
			PHINode* result = builder.CreatePHI(Type::getInt64Ty(ctx), 0);
			result->addIncoming(zarg0, condTrueBlock);
			result->addIncoming(defXval, condFalseBlock);
			builder.CreateBr(vctx.Exit);
			addRegisterToCache(vctx, xarg, result, true);
		}
};

void MmixLlvm::Private::emitCsn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSLT(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg) {
			return emitRegisterLoad(vctx, builder, xarg);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}
		
void MmixLlvm::Private::emitCsz(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpEQ(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg) {
			return emitRegisterLoad(vctx, builder, xarg);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}
		
void MmixLlvm::Private::emitCsp(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSGT(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg) {
			return emitRegisterLoad(vctx, builder, xarg);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}
		
void MmixLlvm::Private::emitCsod(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpNE(builder.CreateAnd(arg, builder.getInt64(1)), builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg) {
			return emitRegisterLoad(vctx, builder, xarg);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}
		
void MmixLlvm::Private::emitCsnn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSGE(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg) {
			return emitRegisterLoad(vctx, builder, xarg);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitCsnz(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpNE(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg) {
			return emitRegisterLoad(vctx, builder, xarg);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitCsnp(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSLE(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg) {
			return emitRegisterLoad(vctx, builder, xarg);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitCsev(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpEQ(builder.CreateAnd(arg, builder.getInt64(1)), builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext& vctx, IRBuilder<>& builder, MXByte xarg) {
			return emitRegisterLoad(vctx, builder, xarg);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitZsn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSLT(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext&, IRBuilder<>& builder, MXByte) {
			return builder.getInt64(0);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}
		
void MmixLlvm::Private::emitZsz(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpEQ(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext&, IRBuilder<>& builder, MXByte) {
			return builder.getInt64(0);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}
		
void MmixLlvm::Private::emitZsp(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSGT(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext&, IRBuilder<>& builder, MXByte) {
			return builder.getInt64(0);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}
		
void MmixLlvm::Private::emitZsod(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpNE(builder.CreateAnd(arg, builder.getInt64(1)), builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext&, IRBuilder<>& builder, MXByte) {
			return builder.getInt64(0);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}
		
void MmixLlvm::Private::emitZsnn(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSGE(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext&, IRBuilder<>& builder, MXByte) {
			return builder.getInt64(0);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitZsnz(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpNE(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext&, IRBuilder<>& builder, MXByte) {
			return builder.getInt64(0);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitZsnp(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpSLE(arg, builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext&, IRBuilder<>& builder, MXByte) {
			return builder.getInt64(0);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}

void MmixLlvm::Private::emitZsev(VerticeContext& vctx, MXByte xarg, MXByte yarg, MXByte zarg, bool immediate)
{
	struct Cond {
		static Value* emitCond(IRBuilder<>& builder, Value* arg) {
			return builder.CreateICmpEQ(builder.CreateAnd(arg, builder.getInt64(1)), builder.getInt64(0));
		}

		static Value* emitDefXVal(VerticeContext&, IRBuilder<>& builder, MXByte) {
			return builder.getInt64(0);
		}
	};
	EmitCond<Cond>::emit(vctx, xarg, yarg, zarg, immediate);
}

