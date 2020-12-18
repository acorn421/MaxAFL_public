#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/ModuleSlotTracker.h"

#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/lookup_edge.hpp>
// #include <boost/graph/graphviz.hpp>

#include <vector>
#include <queue>
#include <stack>
#include <map>
#include <tuple>
#include <iostream>
#include <fstream>

#include "common.h"

using namespace llvm;
using namespace std;
using namespace boost;

namespace
{
  class CmpInfo
  {
  public:
    int moduleId;
    int id;
    int cmpType;

    CmpInfo() : moduleId(-1), id(-1), cmpType(-1) {}
    CmpInfo(int cmpType) : moduleId(-1), id(-1), cmpType(cmpType) {}
    CmpInfo(int moduleId, int id) : moduleId(moduleId), id(id), cmpType(-1) {}

    void print(raw_ostream &out)
    {
      out << "ModuleId :\t" << moduleId << ",id :\t" << id << ",cmpType :\t" << cmpType;
    }

    void save(raw_ostream &out)
    {
      out << moduleId << "\t" << id << "\t" << cmpType << endl;
    }
  };

  class CmpInfoWithNo : public CmpInfo
  {
  public:
    bool no;

    CmpInfoWithNo() : CmpInfo(), no(false) {}
    CmpInfoWithNo(CmpInfo cmpInfo, bool no)
    {
      this->moduleId = cmpInfo.moduleId;
      this->id = cmpInfo.id;
      this->cmpType = cmpInfo.cmpType;
      this->no = no;
    }
    void print(raw_ostream &out)
    {
      out << "ModuleId :\t" << moduleId << ",id :\t" << id << ",cmpType :\t" << cmpType << ",no : " << no;
    }
  };

  class BrInfo
  {
  public:
    int moduleId;
    int id;
    int left, right;
    vector<CmpInfoWithNo> cmpvec;

    BrInfo() : moduleId(-1), id(-1), left(-1), right(-1) {}
    BrInfo(int moduleId, int id, int left, int right) : moduleId(moduleId), id(id), left(left), right(right) {}

    void print(raw_ostream &out)
    {
      for (auto cmp : cmpvec)
      {
        out << "(" << cmp.cmpType << "," << cmp.id << ")";
      }
    }

    void save(raw_ostream &out)
    {
      out << moduleId << "\t" << id << "\t" << left << "\t" << right << "\t" << cmpvec.size();
      for (auto cmp : cmpvec)
      {
        out << "\t" << cmp.id << "\t" << cmp.cmpType << "\t" << cmp.no << "\t";
      }
      out << endl;
    }
  };

  class MaxAFLPass : public ModulePass
  {
  public:
    static char ID;

    u32 moduleId;
    u32 idCnt;
    u32 cmpCnt;
    u32 brCnt;

    // Types
    Type *VoidTy;
    IntegerType *Int1Ty;
    IntegerType *Int8Ty;
    IntegerType *Int16Ty;
    IntegerType *Int32Ty;
    IntegerType *Int64Ty;
    Type *Float32Ty;
    Type *Double64Ty;
    Type *Int8PtrTy;
    Type *Int64PtrTy;

    FunctionType *VisitCmpIntTy;
    FunctionType *VisitCmpFloatTy;
    FunctionType *VisitBrTy;
    FunctionType *VisitEtcTy;
    FunctionType *EnterInternalTy;
    FunctionType *ExitInternalTy;

    Constant *VisitCmpInt;
    Constant *VisitCmpFloat;
    Constant *VisitBr;
    Constant *VisitEtc;
    Constant *EnterInternal;
    Constant *ExitInternal;

    Function *VisitCmpIntFunc;
    Function *VisitCmpFloatFunc;
    Function *VisitBrFunc;
    Function *VisitEtcFunc;
    Function *EnterInternalFunc;
    Function *ExitInternalFunc;

    Constant *VisitCmpIntPtr;
    Constant *VisitCmpFloatPtr;
    Constant *VisitBrPtr;
    Constant *VisitEtcPtr;
    Constant *EnterInternalPtr;
    Constant *ExitInternalPtr;

    unsigned NoSanMetaId;
    MDTuple *NoneMetaNode;

    unsigned MaxExpInstCntMetaId;
    unsigned LoopCmpMetaId;
    unsigned LoopBBMetaId;
    unsigned CmpIdMetaId;

    vector<BrInfo> brvec;
    vector<CmpInfo> cmpvec;

    raw_fd_ostream *infoFile;
    raw_fd_ostream *comInfoFile;
    raw_fd_ostream *resultFile;

    map<string, pair<int, int>>
        funcMap;
    map<int, string> moduleMap;
    set<int> moduleDep;

    typedef pair<BasicBlock *, BasicBlock *> BackEdge;

    static std::string getSimpleNodeLabel(const BasicBlock *Node,
                                          const Function *)
    {
      if (Node->hasName() || !Node->getName().empty())
        return Node->getName().str();

      std::string Str;

      raw_string_ostream OS(Str);

      Node->printAsOperand(OS, false);
      return OS.str();
    };
    static int getSimpleNodeNumber(const BasicBlock *Node,
                                   const Function *)
    {
      if (Node->hasName() || !Node->getName().empty())
        return -1;

      std::string Str;

      raw_string_ostream OS(Str);

      Node->printAsOperand(OS, false);

      return std::stoi(OS.str().substr(1));
    };
    MaxAFLPass() : ModulePass(ID) {}
    void initVariable(Module &M);
    void freeVariable(Module &M);
    Value *castCmpArgType(IRBuilder<> &IRB, Value *V, CmpInst *inst, bool extType);
    int getNextId();
    void setValueNonSan(Value *v);
    void setInsNonSan(Instruction *ins);
    bool setCmpId(Module &M, Instruction *ins, int id);
    int getCmpId(Module &M, Instruction *ins);
    void printInstWithTab(Instruction *inst, int tab, string prefix = "");
    bool visitBrInst(Module &M, Value *v, BrInfo &brinfo, bool no, int debugDepth);
    bool visitCmpInst(Module &M, Instruction *inst, CmpInfo &cmpinfo);
    bool visitEtcInst(Module &M, Instruction *inst, CmpInfo &cmpinfo);
    bool getIncomingAndBackEdge(Loop *L, BasicBlock *&Incoming, BasicBlock *&Backedge);
    void getBackedges(Loop *L, set<BackEdge> &backEdgeSet);
    void getLoopBB(Loop *L, set<BasicBlock *> &);
    bool runOnModule(Module &M) override;
    void getAnalysisUsage(AnalysisUsage &AU) const override;
  };

  char MaxAFLPass::ID = 0;

  class MyBasicBlock
  {
  public:
    BasicBlock *ptr;
    int instCnt;

    MyBasicBlock() : ptr(nullptr), instCnt(0) {}
    MyBasicBlock(BasicBlock *B) : ptr(B), instCnt(0) {}
    bool operator==(const MyBasicBlock &other) const
    {
      return (this->ptr == other.ptr);
    }
    bool operator==(const BasicBlock &other) const
    {
      return (this->ptr == &other);
    }
    bool operator==(const BasicBlock *&other) const
    {
      return (this->ptr == other);
    }
    bool operator<(const MyBasicBlock &other) const
    {
      return (this->ptr < other.ptr);
    }
    bool operator>(const MyBasicBlock &other) const
    {
      return (this->ptr > other.ptr);
    }
  };
} // namespace

void MaxAFLPass::initVariable(Module &M)
{
  ifstream funcMapStream;
  moduleId = 0;

  funcMapStream.open(common::funcMapFileName);
  if (funcMapStream.fail())
  {
    common::print << "no funcMap file" << endl;
  }
  else
  {
    string funcName, moduleName;
    int funcModuleId, funcMEIC, moduleNum;

    funcMapStream >> moduleId;
    for (unsigned int i = 0; i < moduleId; i++)
    {
      funcMapStream >> moduleNum >> moduleName;
      moduleMap[moduleNum] = moduleName;
    }
    while (funcMapStream >> funcName >> funcModuleId >> funcMEIC)
    {
      funcMap[funcName] = make_pair(funcModuleId, funcMEIC);
    }
    funcMapStream.close();
  }

  common::print << "ModueId : " << moduleId << endl;

  idCnt = 0;
  brCnt = 0;
  cmpCnt = 0;

  LLVMContext &C = M.getContext();
  VoidTy = Type::getVoidTy(C);
  Int1Ty = IntegerType::getInt1Ty(C);
  Int8Ty = IntegerType::getInt8Ty(C);
  Int32Ty = IntegerType::getInt32Ty(C);
  Int64Ty = IntegerType::getInt64Ty(C);
  Float32Ty = Type::getFloatTy(C);
  Double64Ty = Type::getDoubleTy(C);
  Int8PtrTy = PointerType::getUnqual(Int8Ty);
  Int64PtrTy = PointerType::getUnqual(Int64Ty);

  NoSanMetaId = C.getMDKindID("nosanitize");
  NoneMetaNode = MDNode::get(C, None);

  MaxExpInstCntMetaId = C.getMDKindID("max_meic");

  LoopCmpMetaId = C.getMDKindID("max_loopcmp");
  LoopBBMetaId = C.getMDKindID("max_loopbb");

  CmpIdMetaId = C.getMDKindID("max_cmpid");

  Type *VisitCmpIntArgs[9] = {Int32Ty, Int32Ty, Int32Ty, Int32Ty, Int32Ty, Int64Ty, Int64Ty, Int64Ty, Int64Ty};
  VisitCmpIntTy = FunctionType::get(Int32Ty, VisitCmpIntArgs, false);
  VisitCmpInt = M.getOrInsertFunction("__maxafl_visit_cmp_integer", VisitCmpIntTy);
  VisitCmpIntFunc = dyn_cast<Function>(VisitCmpInt);
  if (VisitCmpIntFunc)
  {
    VisitCmpIntFunc->addAttribute(~0U, Attribute::NoUnwind);
    VisitCmpIntFunc->addAttribute(~0U, Attribute::ReadNone);
  }

  Type *VisitCmpFloatArgs[7] = {Int32Ty, Int32Ty, Int32Ty, Int32Ty, Int32Ty, Double64Ty, Double64Ty};
  VisitCmpFloatTy = FunctionType::get(Int32Ty, VisitCmpFloatArgs, false);
  VisitCmpFloat = M.getOrInsertFunction("__maxafl_visit_cmp_float", VisitCmpFloatTy);
  VisitCmpFloatFunc = dyn_cast<Function>(VisitCmpFloat);
  if (VisitCmpFloatFunc)
  {
    VisitCmpFloatFunc->addAttribute(~0U, Attribute::NoUnwind);
    VisitCmpFloatFunc->addAttribute(~0U, Attribute::ReadNone);
  }

  Type *VisitBrArgs[2] = {Int32Ty, Int32Ty};
  VisitBrTy = FunctionType::get(Int32Ty, VisitBrArgs, false);
  VisitBr = M.getOrInsertFunction("__maxafl_visit_br", VisitBrTy);
  VisitBrFunc = dyn_cast<Function>(VisitBr);
  if (VisitBrFunc)
  {
    VisitBrFunc->addAttribute(~0U, Attribute::NoUnwind);
    VisitBrFunc->addAttribute(~0U, Attribute::ReadNone);
  }

  Type *VisitEtcArgs[3] = {Int32Ty, Int32Ty, Int32Ty};
  VisitEtcTy = FunctionType::get(Int32Ty, VisitEtcArgs, false);
  VisitEtc = M.getOrInsertFunction("__maxafl_visit_etc", VisitEtcTy);
  VisitEtcFunc = dyn_cast<Function>(VisitEtc);
  if (VisitEtcFunc)
  {
    VisitEtcFunc->addAttribute(~0U, Attribute::NoUnwind);
    VisitEtcFunc->addAttribute(~0U, Attribute::ReadNone);
  }

  Type *EnterInternalArgs[1] = {Int32Ty};
  EnterInternalTy = FunctionType::get(Int32Ty, EnterInternalArgs, false);
  EnterInternal = M.getOrInsertFunction("__maxafl_enter_internal_call", EnterInternalTy);
  EnterInternalFunc = dyn_cast<Function>(EnterInternal);
  if (EnterInternalFunc)
  {
    EnterInternalFunc->addAttribute(~0U, Attribute::NoUnwind);
    EnterInternalFunc->addAttribute(~0U, Attribute::ReadNone);
  }

  Type *ExitInternalArgs[1] = {Int32Ty};
  ExitInternalTy = FunctionType::get(Int32Ty, ExitInternalArgs, false);
  ExitInternal = M.getOrInsertFunction("__maxafl_exit_internal_call", ExitInternalTy);
  ExitInternalFunc = dyn_cast<Function>(ExitInternal);
  if (ExitInternalFunc)
  {
    ExitInternalFunc->addAttribute(~0U, Attribute::NoUnwind);
    ExitInternalFunc->addAttribute(~0U, Attribute::ReadNone);
  }

  VisitCmpIntPtr = M.getOrInsertGlobal("__maxafl_visit_integer_func", PointerType::get(VisitCmpIntTy, 0));
  VisitCmpFloatPtr = M.getOrInsertGlobal("__maxafl_visit_float_func", PointerType::get(VisitCmpFloatTy, 0));
  VisitBrPtr = M.getOrInsertGlobal("__maxafl_visit_br_func", PointerType::get(VisitBrTy, 0));
  VisitEtcPtr = M.getOrInsertGlobal("__maxafl_visit_etc_func", PointerType::get(VisitEtcTy, 0));
  EnterInternalPtr = M.getOrInsertGlobal("__maxafl_enter_internal_call_func", PointerType::get(EnterInternalTy, 0));
  ExitInternalPtr = M.getOrInsertGlobal("__maxafl_exit_internal_call_func", PointerType::get(ExitInternalTy, 0));

  std::error_code EC, EC2;
  string filename = M.getName().str();
  common::print << "filename " << filename << endl;
  filename = filename.substr(filename.rfind("/") + 1, filename.rfind(".") - filename.rfind("/") - 1);
  common::print << "original file name : " << filename << endl;
  string fileName = filename + ".info";
  common::print << "Info file name : " << fileName << endl;
  infoFile = new raw_fd_ostream(fileName, EC);

  string filename2 = M.getName().str();
  filename2 = filename2.substr(filename2.rfind("/") + 1, filename2.rfind(".") - filename2.rfind("/") - 1);
  string fileName2 = filename2 + "_opt.ll";
  common::print << "IR file name : " << fileName2 << endl;
  resultFile = new raw_fd_ostream(fileName2, EC);

  string filename3 = "infofile.info";
  comInfoFile = new raw_fd_ostream(filename3, EC, (llvm::sys::fs::OpenFlags)2);

  moduleMap[moduleId] = filename;

  return;
}

void MaxAFLPass::freeVariable(Module &M)
{
  infoFile->close();
  resultFile->close();
  comInfoFile->close();

  delete infoFile, resultFile, comInfoFile;

  return;
}

int MaxAFLPass::getNextId()
{
  return this->idCnt++;
}

Value *MaxAFLPass::castCmpArgType(IRBuilder<> &IRB, Value *V, CmpInst *inst, bool extType)
{
  Type *OpType = V->getType();
  Value *NV = V;
  if (OpType->isFloatTy())
  {
    NV = IRB.CreateFPExt(V, Double64Ty);
  }
  else if (OpType->isPointerTy())
  {
    NV = IRB.CreatePtrToInt(V, Int64Ty);
    if (extType)
    {
      NV = IRB.CreateSExt(NV, Int64Ty);
    }
    else
    {
      NV = IRB.CreateZExt(NV, Int64Ty);
    }
  }
  else
  {
    if (OpType->isIntegerTy() && OpType->getIntegerBitWidth() < 64)
    {
      if (extType)
      {
        NV = IRB.CreateSExt(V, Int64Ty);
      }
      else
      {
        NV = IRB.CreateZExt(V, Int64Ty);
      }
    }
  }
  return NV;
}

void MaxAFLPass::setValueNonSan(Value *v)
{
  if (Instruction *ins = dyn_cast<Instruction>(v))
    setInsNonSan(ins);
}

void MaxAFLPass::setInsNonSan(Instruction *ins)
{
  if (ins)
    ins->setMetadata(NoSanMetaId, NoneMetaNode);
}

bool MaxAFLPass::setCmpId(Module &M, Instruction *ins, int id)
{
  Metadata *MDs[] = {ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(M.getContext()), id))};
  ins->setMetadata(CmpIdMetaId, MDNode::get(M.getContext(), MDs));

  return true;
}

int MaxAFLPass::getCmpId(Module &M, Instruction *ins)
{
  if (MDNode *metadata = ins->getMetadata(CmpIdMetaId))
  {
    Value *val = dyn_cast<ValueAsMetadata>(metadata->getOperand(0))->getValue();
    ConstantInt *cmpid = dyn_cast<ConstantInt>(val);
    return cmpid->getSExtValue();
  }
  else
  {
    return -1;
  }
}

void MaxAFLPass::printInstWithTab(Instruction *inst, int tab, string prefix)
{
  // #ifdef _MAXAFL_DEBUG
  common::print << "[DBG]\t";
  for (int i = 0; i < tab; i++)
    common::print << "\t";
  common::print << prefix << " Inst : ";
  inst->print(common::print);
  common::print << endl;
  // #endif
}

bool MaxAFLPass::visitBrInst(Module &M, Value *v, BrInfo &brinfo, bool no, int debugDepth)
{
  bool res = true;
  Instruction *inst = dyn_cast<Instruction>(v);

  // inst->print(common::print);

  // inst->print(common::print);
  // common::print << endl;
  // common::print << inst->getValueID() << endl;

  if (isa<CmpInst>(inst))
  {
    // printInstWithTab(inst, debugDepth, "Cmp");
    // CmpInst *cmpInst = dyn_cast<CmpInst>(inst);
    CmpInfo cmpinfo;

    int tmpid = getCmpId(M, inst);
    if (tmpid != -1)
    {
      cmpinfo.moduleId = moduleId;
      cmpinfo.id = tmpid;
      cmpinfo.cmpType = cmpvec[tmpid].cmpType;
    }
    else
    {
      setCmpId(M, inst, cmpCnt++);
      visitCmpInst(M, inst, cmpinfo);
    }
    brinfo.cmpvec.push_back(CmpInfoWithNo(cmpinfo, no));
  }
  else if (isa<BinaryOperator>(inst))
  {
    // printInstWithTab(inst, debugDepth, "Bin");
    BinaryOperator *binInst = dyn_cast<BinaryOperator>(inst);

    int op = binInst->getOpcode();
    //  AND, OR, XOR
    if (op != BINOP_AND && op != BINOP_OR && op != BINOP_XOR)
    {
      common::print << "[WARN]\tUnexpected opcode at binary operator : " << endl;
      printInstWithTab(inst, debugDepth, "Bin");

      Instruction *inst0 = dyn_cast<Instruction>(binInst->getOperand(0));
      if (inst0)
      {
        printInstWithTab(inst0, debugDepth + 1, "USE1 ");
      }
      Instruction *inst1 = dyn_cast<Instruction>(binInst->getOperand(1));
      if (inst1)
      {
        printInstWithTab(inst1, debugDepth + 1, "USE2 ");
      }

      return false;
    }

    // Value *opb0 = binInst->getOperand(0);
    // Value *opb1 = binInst->getOperand(1);

    Value *useb0 = binInst->getOperand(0)->use_begin()->get();
    Value *useb1 = binInst->getOperand(1)->use_begin()->get();

    // One of the operands is constant(e.g. a<b OR true -> true)
    if (isa<Constant>(useb0) != isa<Constant>(useb1))
    {
      printInstWithTab(inst, debugDepth, "Bin w Const");

      Constant *constant = isa<Constant>(useb0) ? dyn_cast<Constant>(useb0) : dyn_cast<Constant>(useb1);
      Value *variable = isa<Constant>(useb0) ? useb1 : useb0;
      bool logic = constant->getUniqueInteger().getBoolValue();

      if ((!logic && op == BINOP_OR) || (logic && op == BINOP_AND) || (!logic && op == BINOP_XOR))
      {
        visitBrInst(M, variable, brinfo, no, debugDepth + 1);
      }
      else if ((logic && op == BINOP_XOR))
      {
        visitBrInst(M, variable, brinfo, !no, debugDepth + 1);
      }
      else
      {
        common::print << "[FATAL]\tUnexpected binary operator operands. It's always true or false" << endl;
        return false;
      }
    }
    // Both of operands aren't Constants(e.g. a<b AND c<d)
    else if (!isa<Constant>(useb0) && !isa<Constant>(useb1))
    {
      // Add Binary Operator to vector
      // In the case of OR or AND, add it to the vector
      if (op == BINOP_AND || op == BINOP_OR)
      {
        CmpInfo cmpinfo(op);
        brinfo.cmpvec.push_back(CmpInfoWithNo(cmpinfo, no));

        res &= visitBrInst(M, useb0, brinfo, no, debugDepth + 1);
        res &= visitBrInst(M, useb1, brinfo, no, debugDepth + 1);
      }
      //  In the case of XOR, the expression is expanded using OR and AND, and then add it to the vector.
      else if (op == BINOP_XOR)
      {
        CmpInfo cmpinfo(BINOP_OR);
        brinfo.cmpvec.push_back(CmpInfoWithNo(cmpinfo, no));

        cmpinfo.cmpType = BINOP_AND;

        brinfo.cmpvec.push_back(CmpInfoWithNo(cmpinfo, no));
        res &= visitBrInst(M, useb0, brinfo, !no, debugDepth + 1);
        res &= visitBrInst(M, useb1, brinfo, no, debugDepth + 1);

        brinfo.cmpvec.push_back(CmpInfoWithNo(cmpinfo, no));
        res &= visitBrInst(M, useb0, brinfo, no, debugDepth + 1);
        res &= visitBrInst(M, useb1, brinfo, !no, debugDepth + 1);
      }
    }
    else
    {
      common::print << "[FATAL]\tUnexpected binary operator operands. (const) (binop) (const)" << endl;
      return false;
    }
  }
  // else if (isa<LoadInst>(inst))
  // {
  //   common::print << "Load Uses : \t";
  //   LoadInst *loadInst = dyn_cast<LoadInst>(inst);
  //   loadInst->print(common::print);
  //   common::print << endl;
  // }
  // else if (isa<ExtractElementInst>(inst))
  // {
  //   common::print << "ExtractElement Uses : \t";
  //   ExtractElementInst *extInst = dyn_cast<ExtractElementInst>(inst);
  //   extInst->print(common::print);
  //   common::print << endl;
  // }
  else
  {
    printInstWithTab(inst, debugDepth, "Etc");
    CmpInfo cmpinfo;
    int tmpid = getCmpId(M, inst);

    if (tmpid != -1)
    {
      cmpinfo.moduleId = moduleId;
      cmpinfo.id = tmpid;
      cmpinfo.cmpType = -1;
    }
    else
    {
      setCmpId(M, inst, cmpCnt++);
      visitEtcInst(M, inst, cmpinfo);
    }

    brinfo.cmpvec.push_back(CmpInfoWithNo(cmpinfo, no));
  }
  return res;
}

bool MaxAFLPass::visitCmpInst(Module &M, Instruction *inst, CmpInfo &cmpinfo)
{
  // ! Insert __maxafl_visit_cmp runtime function at cmp instructions
  Value *mId, *cmpId, *cmpType, *argType, *opSize;
  int cmpid, cmptype, argtype, opsize;
  CmpInst *cmpInst = dyn_cast<CmpInst>(inst);

  IRBuilder<> IRB(cmpInst);
  Value *OpSArg[2];
  Value *OpZArg[2];

  OpSArg[0] = cmpInst->getOperand(0);
  OpSArg[1] = cmpInst->getOperand(1);
  OpSArg[0] = castCmpArgType(IRB, OpSArg[0], cmpInst, true);
  OpSArg[1] = castCmpArgType(IRB, OpSArg[1], cmpInst, true);
  OpZArg[0] = cmpInst->getOperand(0);
  OpZArg[1] = cmpInst->getOperand(1);
  OpZArg[0] = castCmpArgType(IRB, OpZArg[0], cmpInst, false);
  OpZArg[1] = castCmpArgType(IRB, OpZArg[1], cmpInst, false);

  cmpid = getCmpId(M, inst);
  cmptype = cmpInst->getPredicate();
  argtype = OpSArg[0]->getType()->getTypeID();
  opsize = 32;

  mId = ConstantInt::get(M.getContext(), APInt(32, moduleId, false));
  cmpId = ConstantInt::get(M.getContext(), APInt(32, cmpid, false));
  cmpType = ConstantInt::get(M.getContext(), APInt(32, cmptype, false));
  argType = ConstantInt::get(M.getContext(), APInt(32, argtype, false));
  opSize = ConstantInt::get(M.getContext(), APInt(32, opsize, false));

  if (cmpInst->isIntPredicate())
  {
    // CallInst *ProxyCall = IRB.CreateCall(VisitCmpIntFunc, {mId, cmpId, cmpType, argType, opSize, OpSArg[0], OpSArg[1], OpZArg[0], OpZArg[1]});
    // setInsNonSan(ProxyCall);

    auto VisitCmpIntF = IRB.CreateLoad(VisitCmpIntPtr);
    CallInst *ProxyCall = IRB.CreateCall(VisitCmpIntF, {mId, cmpId, cmpType, argType, opSize, OpSArg[0], OpSArg[1], OpZArg[0], OpZArg[1]});
    setInsNonSan(ProxyCall);
  }
  else
  {
    // CallInst *ProxyCall = IRB.CreateCall(VisitCmpFloatFunc, {mId, cmpId, cmpType, argType, opSize, OpSArg[0], OpSArg[1]});
    // setInsNonSan(ProxyCall);

    auto VisitCmpFloatF = IRB.CreateLoad(VisitCmpFloatPtr);
    CallInst *ProxyCall = IRB.CreateCall(VisitCmpFloatF, {mId, cmpId, cmpType, argType, opSize, OpSArg[0], OpSArg[1]});
    setInsNonSan(ProxyCall);
  }

  cmpinfo.moduleId = moduleId;
  cmpinfo.id = cmpid;
  cmpinfo.cmpType = cmptype;

  cmpvec.push_back(cmpinfo);

  // common::print << "\t\t\tVisitCmpInst : ";
  // inst->print(common::print);
  // common::print << endl;

  return true;
}

bool MaxAFLPass::visitEtcInst(Module &M, Instruction *inst, CmpInfo &cmpinfo)
{
  Value *mId, *cmpId;
  int cmpid;
  Instruction *insert;

  for (insert = inst->getNextNode(); PHINode::classof(insert); insert = insert->getNextNode())
  {
  }

  IRBuilder<> IRB(insert);

  cmpid = getCmpId(M, inst);

  mId = ConstantInt::get(M.getContext(), APInt(32, moduleId, false));
  cmpId = ConstantInt::get(M.getContext(), APInt(32, cmpid, false));

  auto CastedRealVal = CastInst::CreateIntegerCast(inst, Int32Ty, 0, "", insert);
  auto VisitEtcF = IRB.CreateLoad(VisitEtcPtr);
  CallInst *ProxyCall = IRB.CreateCall(VisitEtcF, {mId, cmpId, CastedRealVal});
  setInsNonSan(ProxyCall);

  cmpinfo.moduleId = moduleId;
  cmpinfo.id = cmpid;
  cmpinfo.cmpType = -1;

  cmpvec.push_back(cmpinfo);

  return true;
}

bool MaxAFLPass::getIncomingAndBackEdge(Loop *L, BasicBlock *&Incoming, BasicBlock *&Backedge)
{
  BasicBlock *H = L->getHeader();

  Incoming = nullptr;
  Backedge = nullptr;
  pred_iterator PI = pred_begin(H);
  assert(PI != pred_end(H) && "Loop must have at least one backedge!");
  Backedge = *PI++;
  if (PI == pred_end(H))
    return false; // dead loop
  Incoming = *PI++;
  if (PI != pred_end(H))
    return false; // multiple backedges?

  if (L->contains(Incoming))
  {
    if (L->contains(Backedge))
      return false;
    std::swap(Incoming, Backedge);
  }
  else if (!L->contains(Backedge))
    return false;

  assert(Incoming && Backedge && "expected non-null incoming and backedges");
  return true;
}

void MaxAFLPass::getBackedges(Loop *L, set<BackEdge> &backEdgeSet)
{
  BasicBlock *header = L->getHeader();

  BasicBlock *incoming, *backedge;
  if (getIncomingAndBackEdge(L, incoming, backedge) == false)
  {
    common::print << "[WARN]\tCannot get backedges from loop!" << endl;
  }

  backEdgeSet.insert(BackEdge(header, backedge));

  vector<Loop *> subLoops = L->getSubLoops();
  // common::print << "Subloop size : " << subLoops.size() << endl;
  // common::print << "End : " << subLoops.end() - subLoops.begin() << ", Same? : " << (subLoops.begin() == subLoops.end()) << endl;
  // for (vector<Loop *>::iterator iter = subLoops.begin(); iter != subLoops.end(); iter++)
  // {
  //   getBackedges(*iter, backEdgeSet);
  // }

  // for (int i = 0; i < subLoops.size(); i++)
  // {
  //   auto subLoop = subLoops[i];
  //   common::print << "[DBG]\t SubLoop : " << subLoop << endl;
  // }

  for (auto subLoop : subLoops)
  {
    getBackedges(subLoop, backEdgeSet);
  }
}

void MaxAFLPass::getLoopBB(Loop *L, set<BasicBlock *> &loopBBSet)
{
  SmallVector<BasicBlock *, 4> latchBlocks;

  latchBlocks.clear();
  L->getLoopLatches(latchBlocks);
  for (auto latchBlock : latchBlocks)
  {
    loopBBSet.insert(latchBlock);
  }

  vector<Loop *> subLoops = L->getSubLoops();
  common::print << "Subloop size2 : " << subLoops.size() << endl;
  common::print << "End2 : " << subLoops.end() - subLoops.begin() << ", Same? : " << (subLoops.begin() == subLoops.end()) << endl;
  for (auto subLoop : subLoops)
  {
    getLoopBB(subLoop, loopBBSet);
  }
  // Loop::getIncomingAndBackEdge
}

bool MaxAFLPass::runOnModule(Module &M)
{
  // ! Initialize variables.
  initVariable(M);

  // ####################################################################
  //              Phase 0 : Delete dead code
  // ####################################################################

  // TargetTransformInfoWrapperPass *TTI = getAnalysisIfAvailable<TargetTransformInfoWrapperPass>();
  // for (auto &F : M)
  // {
  //   TargetTransformInfo &tti = TTI->getTTI(F);
  //   for (auto &BB : F)
  //   {
  //     BB.print(common::print);
  //     common::print << endl;
  //     simplifyCFG(&BB, tti);
  //     common::print << "AFTER : \n";
  //     BB.print(common::print);
  //     common::print << endl;
  //     // SimplifyInstructionsInBlock(&BB);
  //   }
  // }

  // FunctionPass *dcePass = createDeadCodeEliminationPass();
  // for (auto &F : M)
  // {
  //   if (F.getBasicBlockList().size() == 0)
  //   {
  //     continue;
  //   }
  //   F.print(common::print);
  //   dcePass->runOnFunction(F);
  //   common::print << "AFTER :\n";
  //   F.print(common::print);
  // }

  // ####################################################################
  //              Phase 1 : Analyze calllgraph
  // ####################################################################

  typedef adjacency_list<vecS, vecS, bidirectionalS> Graph;
  typedef Graph::vertex_descriptor Vertex;
  typedef std::vector<Vertex> Container;
  typedef map<Function *, int> PtrIdMap;
  typedef map<int, Function *> IdPtrMap;
  Graph callG;
  PtrIdMap ptrIdMap;
  IdPtrMap idPtrMap;

  // ! Generate call graph to get analysis order of each function.
  CallGraph &callgraph = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  int funcId = 0;

  // ! Execption when module has no function.
  if (++callgraph.begin() == callgraph.end())
  {
    return false;
  }

  // ! Make Maps for function id to function ptr vice versa.
  for (auto &node : callgraph)
  {
    Function *funcPtr = node.second.get()->getFunction();
    funcId = add_vertex(callG);
    ptrIdMap[funcPtr] = funcId;
    idPtrMap[funcId] = funcPtr;
    // funcId++;
  }

  // ! Convert raw call graph to boost graph before executing topological sort.

  common::print << "External Node : " << callgraph.getExternalCallingNode()->getFunction() << endl;
  common::print << "First iterator node of callgraph : " << callgraph.begin()->second.get()->getFunction() << endl;
  common::print << "Second iterator node of callgraph : " << (++callgraph.begin())->second.get()->getFunction() << endl;

  for (auto &node : callgraph)
  {
    Function *ptrU, *ptrV;
    ptrU = node.second.get()->getFunction();
    if (ptrU != nullptr && ptrU->hasName())
    {
      common::print << "U : " << ptrU->getName() << endl;
    }
    else
    {
      common::print << "U : No-Named" << endl;
    }

    for (auto &next : *node.second.get())
    {
      if (auto func = next.second->getFunction())
      {
        ptrV = func;
        // common::print << "V : " << ptrIdMap[ptrV] << ", U : " << ptrIdMap[ptrU] << endl;
        if (lookup_edge(ptrIdMap[ptrV], ptrIdMap[ptrU], callG).second == false)
        {
          add_edge(ptrIdMap[ptrV], ptrIdMap[ptrU], callG);
        }
        // ! Note that we add edges reversely to the callgraph.(original callG main->f, our callG f->main)
        // add_edge(ptrIdMap[ptrV], ptrIdMap[ptrU], callG);
        // add_edge(ptrIdMap[ptrU], ptrIdMap[ptrV], callG);
      }
    }
  }

  // Graph::in_edge_iterator begin, end;
  // common::print << "out_edges of 0 : ";
  // for (tie(begin, end) = in_edges(0, callG); begin != end; begin++)
  // {
  //   common::print << source(*begin, callG) << "\t";
  // }
  // common::print << endl;

  // ! Find entry function pointer
  Function *entryF = callgraph.getExternalCallingNode()->getFunction();

  // ! Remove cycles in callgraph using DFS-like algorithm.
  stack<Vertex> stk;
  Vertex cur;
  set<Vertex> visited;
  set<Vertex> finished;
  stk.push(ptrIdMap[entryF]);
  while (!stk.empty())
  {
    set<Vertex> removed;

    cur = stk.top();

    if (visited.count(cur) == 0)
    {
      visited.insert(cur);
    }

    Graph::in_edge_iterator begin, end;

    // int cnt = 0;
    // for (tie(begin, end) = in_edges(cur, callG); begin != end; begin++)
    // {
    //   cnt++;
    //   common::print << "\tIn-edge : " << source(*begin, callG) << endl;
    // }
    // common::print << "size of in_edges : " << cnt << endl;
    for (tie(begin, end) = in_edges(cur, callG); begin != end; begin++)
    {
      Vertex src = source(*begin, callG);
      if (visited.count(src) > 0)
      {
        // common::print << "\t\tRemoved\tCurrent Node : " << cur << ", Src Node : " << src << endl;
        removed.insert(src);
      }
      else if (finished.count(src) == 0)
      {
        // common::print << "\tPushed\tCurrent Node : " << cur << ", Src Node : " << src << endl;
        stk.push(src);
        break;
      }
    }
    for (auto src : removed)
    {
      remove_edge(src, cur, callG);
    }
    // common::print << endl;
    if (cur == stk.top())
    {
      visited.erase(cur);
      finished.insert(cur);
      stk.pop();
    }
  }

  // ! Executing topological sort in callgraph.
  Container topo_callG;
  topological_sort(callG, std::back_inserter(topo_callG));

  for (auto node : topo_callG)
  {
    common::print << node << " ->\t";
  }
  common::print << endl;

  // ####################################################################
  //              Phase 2 : Analyze each CFG of funciton.
  // ####################################################################

  typedef Graph::vertex_descriptor Vertex;
  typedef std::vector<Vertex> Container;
  typedef map<MyBasicBlock, int> BBIdMap;
  typedef map<int, MyBasicBlock> IdBBMap;

  map<Function *, std::tuple<Graph, Container, BBIdMap, IdBBMap, set<BasicBlock *>>> cfGMap;
  set<Function *> userFuncSet;

  for (auto &F : M)
  {
    // ! Skip special function that has only declaration
    if (F.isDeclaration())
      continue;

    // if (F.hasName())
    // {
    //   common::print << "Function Name : " << F.getName() << endl;
    // }

    userFuncSet.insert(&F);

    LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();

    // ! Get Loop Backedges in Function F
    set<BackEdge> backEdgeSet;
    for (auto *L : *LI)
    {
      getBackedges(L, backEdgeSet);
    }

    // ! Get Loop BasicBlock in Function F

    set<BasicBlock *> loopBBSet;
    for (auto *L : *LI)
    {
      getLoopBB(L, loopBBSet);
    }

    for (auto &iter : loopBBSet)
    {
      Metadata *MDs[] = {ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(M.getContext()), 1))};
      iter->getInstList().begin()->setMetadata(LoopBBMetaId, MDNode::get(M.getContext(), MDs));
      BasicBlock *loopBB = iter->getSingleSuccessor();
      if (loopBB != nullptr)
      {
        // Metadata *MDs[] = {ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(M.getContext()), 1))};
        loopBB->getInstList().begin()->setMetadata(LoopCmpMetaId, MDNode::get(M.getContext(), MDs));
      }
      // common::print << "[DBG] LoopBBSet : " << getSimpleNodeLabel(iter, &F) << endl;
    }

    // ! Get all BasicBlocks in Function F and make map for graph id and BB
    Graph cfG;
    BBIdMap bbIdMap;
    IdBBMap idBBMap;
    int bbid = 0;
    for (auto &BB : F)
    {
      MyBasicBlock mBB(&BB);
      bbid = add_vertex(cfG);
      bbIdMap[mBB] = bbid;
      idBBMap[bbid] = mBB;
    }

    typedef set<BasicBlock *> PredSet;
    PredSet predSet;
    for (auto &BB : F)
    {
      // ! Get all parent BasicBlocks and remove duplicate
      pred_iterator PI = pred_begin(&BB), E = pred_end(&BB);

      if (PI == E)
      {
        continue;
      }

      predSet.clear();
      for (auto iter = PI; iter != E; iter++)
      {
        BasicBlock *pred = *iter;
        predSet.insert(pred);
      }

      // ! Add edges to graph without edge in loop block
      for (auto iter : predSet)
      {
        // if (loopBBSet.count(iter) <= 0 || getSimpleNodeNumber(&BB, &F) > getSimpleNodeNumber(iter, &F))
        // {
        //   if (lookup_edge(bbIdMap[&BB], bbIdMap[iter], cfG).second == false)
        //   {
        //     add_edge(bbIdMap[&BB], bbIdMap[iter], cfG);
        //   }
        // }
        if (backEdgeSet.count(BackEdge(&BB, iter)) == 0)
        {
          if (lookup_edge(bbIdMap[&BB], bbIdMap[iter], cfG).second == false)
          {
            add_edge(bbIdMap[&BB], bbIdMap[iter], cfG);
          }
        }
        else
        {
          common::print << "Ignore backedge..." << endl;
        }
      }
    }

    for (auto vd : make_iterator_range(vertices(cfG)))
    {
      if (out_degree(vd, cfG) == 0)
      {
        common::print << "\tVertex Name" << getSimpleNodeLabel(idBBMap[vd].ptr, &F) << "Out-degree : " << out_degree(vd, cfG) << ", \tIn-degree : " << in_degree(vd, cfG) << endl;
      }
    }

    // ! Remove cycles in CFG using DFS-like algorithm.
    BasicBlock &entryBB = F.getEntryBlock();
    stack<Vertex> stk;
    Vertex cur;
    set<Vertex> visited;
    set<Vertex> finished;
    stk.push(bbIdMap[&entryBB]);
    while (!stk.empty())
    {
      set<Vertex> removed;

      cur = stk.top();

      if (visited.count(cur) == 0)
      {
        visited.insert(cur);
      }

      Graph::in_edge_iterator begin, end;
      for (tie(begin, end) = in_edges(cur, cfG); begin != end; begin++)
      {
        Vertex src = source(*begin, cfG);
        if (visited.count(src) > 0)
        {
          removed.insert(src);
        }
        else if (finished.count(src) == 0)
        {
          stk.push(src);
          break;
        }
      }
      for (auto src : removed)
      {
        remove_edge(src, cur, cfG);
      }
      if (cur == stk.top())
      {
        visited.erase(cur);
        finished.insert(cur);
        stk.pop();
      }
    }

    if (F.hasName() && F.getName() == "aout_32_final_link")
    {
      common::print << "graphviz" << endl;
      ofstream ofs;
      ofs.open("aout_32_final_link.dot", ofstream::out);
      // write_graphviz(ofs, cfG);
      // write_graphviz(ofs, cfG);
      ofs.close();
    }

    // ! Execute topological sort in CFG to get order of BasicBlocks
    Container topo_cfG;
    topological_sort(cfG, std::back_inserter(topo_cfG));

    // ! Save CFG information of function as tuple
    cfGMap[&F] = std::tuple<Graph, Container, BBIdMap, IdBBMap, set<BasicBlock *>>(cfG, topo_cfG, bbIdMap, idBBMap, loopBBSet);
  }

  // ####################################################################
  //              Phase 3 : Calculate MEIC
  // ####################################################################

  // ! Iterate Function
  for (Container::reverse_iterator ii = topo_callG.rbegin(); ii != topo_callG.rend(); ii++)
  {
    Function *F = idPtrMap[*ii];

    // ! Skip special case that has no funciton
    if (userFuncSet.count(F) == 0)
    {
      continue;
    }

    // ! Load saved CFG information of the function
    Graph &cfG = std::get<0>(cfGMap[F]);
    Container &topo_cfG = std::get<1>(cfGMap[F]);
    BBIdMap &bbIdMap = std::get<2>(cfGMap[F]);
    IdBBMap &idBBMap = std::get<3>(cfGMap[F]);
    // set<BasicBlock *> &loopBBSet = std::get<4>(cfGMap[F]);

    // ! Iterate CallGraph reversely and calculate MEIC of function
    for (Container::reverse_iterator jj = topo_cfG.rbegin(); jj != topo_cfG.rend(); jj++)
    {
      bool isExitNode = false;
      MyBasicBlock &mBB = idBBMap[*jj];
      mBB.instCnt = mBB.ptr->getInstList().size();

      Instruction *internalCall = nullptr;

      // ! Add MEIC of called function
      for (auto &inst : *(mBB.ptr))
      {
        if (isa<CallInst>(inst))
        {
          CallInst &callInst = (CallInst &)inst;
          Function *calledFunc = callInst.getCalledFunction();
          if (calledFunc == NULL)
          {
            continue;
          }
          if (!calledFunc->hasName() || calledFunc->getName().empty())
          {
            common::print << "called to no-named function!" << endl;
          }
          else
          {
            string funcName = calledFunc->getName().str();
            if (funcMap.count(funcName) > 0)
            {
              internalCall = &inst;

              moduleDep.insert(funcMap[funcName].first);
              mBB.instCnt += funcMap[funcName].second;
              if (funcName == "ErrFatal")
              {
                common::print << "[DBG]\tThis is Exit Node" << endl;
                isExitNode = true;
                // mBB.instCnt -= 1000;
              }
            }
          }
        }
      }

      if (!isExitNode)
      {
        // ! Iterate all BasicBlock of current function and select greater MEIC of child of current BasicBlock
        Graph::in_edge_iterator begin, end;
        int maxInstCnt = 0;
        for (tie(begin, end) = in_edges(*jj, cfG); begin != end; begin++)
        {
          MyBasicBlock childmBB = idBBMap[source(*begin, cfG)];
          maxInstCnt = max(maxInstCnt, childmBB.instCnt);
        }
        mBB.instCnt += maxInstCnt;

        if (internalCall != nullptr)
        {
          IRBuilder<> IRB(internalCall);
          if (!internalCall->getNextNode())
          {
            common::print << "[ERR] internal Call is tail of the BB" << endl;
          }

          IRBuilder<> IRB2(internalCall->getNextNode());
          Value *penalty;
          CallInst *ProxyCall;

          penalty = ConstantInt::get(M.getContext(), APInt(32, maxInstCnt, false));

          auto EnterInternalF = IRB.CreateLoad(EnterInternalPtr);
          ProxyCall = IRB.CreateCall(EnterInternalF, {penalty});
          setInsNonSan(ProxyCall);

          auto ExitnternalF = IRB2.CreateLoad(ExitInternalPtr);
          ProxyCall = IRB2.CreateCall(ExitnternalF, {penalty});
          setInsNonSan(ProxyCall);

          // common::print << "[DBG] Internal Call Analysis routine has been inserted!!" << endl;
        }
      }

      // // Loop Compare BasicBlock의 경우 In Loop BB의 InstCnt를 재계산.
      // // ! Recalculate MEIC of Loop BasicBlock
      // if (mBB.ptr->getInstList().begin()->getMetadata(LoopCmpMetaId) != NULL)
      // {
      //   BranchInst *bInst = (BranchInst *)mBB.ptr->getTerminator();
      //   if (bInst->getNumSuccessors() == 2)
      //   {
      //     MyBasicBlock mInSucc(bInst->getSuccessor(0));
      //     MyBasicBlock mOutSucc(bInst->getSuccessor(1));
      //     MyBasicBlock &mInBB = idBBMap[bbIdMap[mInSucc]];
      //     MyBasicBlock &mOutBB = idBBMap[bbIdMap[mOutSucc]];

      //     mInBB.instCnt += mOutBB.instCnt;
      //   }
      // }
    }

    // ####################################################################
    //              Phase 4 : Instrument at CMP and BR instructions
    // ####################################################################

    // ! Iterate all BasicBlocks in this function and analyze BR instructions.
    Graph::vertex_iterator bb, ee;
    for (tie(bb, ee) = vertices(cfG); bb != ee; bb++)
    {
      auto &mBB = idBBMap[*bb];
      BasicBlock &BB = *(mBB.ptr);

      // Value *mId = ConstantInt::get(M.getContext(), APInt(32, moduleId, false)), *cmpId = nullptr, *cmpType = nullptr, *argType = nullptr, *opSize = nullptr;
      if (!isa<BranchInst>(BB.getTerminator()))
      {
        continue;
      }

      BranchInst *bInst = dyn_cast<BranchInst>(BB.getTerminator());

      if (bInst == NULL || bInst->isUnconditional())
      {
        continue;
      }

      if (bInst->getNumSuccessors() != 2)
      {
        common::print << "[ERR] More(Less) than 2 successors!" << endl;
        continue;
      }

      BasicBlock *leftSucc = bInst->getSuccessor(0);
      MyBasicBlock mLeftSucc(leftSucc);
      MyBasicBlock &mLeftBB = idBBMap[bbIdMap[mLeftSucc]];
      int left = mLeftBB.instCnt;

      BasicBlock *rightSucc = bInst->getSuccessor(1);
      MyBasicBlock mRightSucc(rightSucc);
      MyBasicBlock &mRightBB = idBBMap[bbIdMap[mRightSucc]];
      int right = mRightBB.instCnt;

      // * Calculate Loop back MEIC as 0
      if (mBB.ptr->getInstList().begin()->getMetadata(LoopBBMetaId) != NULL && bInst->getNumSuccessors() == 2)
      {
        if (getSimpleNodeNumber(leftSucc, F) > getSimpleNodeNumber(rightSucc, F))
        {
          right = 0;
        }
        else
        {
          left = 0;
        }
      }

      Value *op0 = bInst->getOperand(0);
      string str;
      llvm::raw_string_ostream ss(str);
      op0->print(ss);
      if (ss.str() == "i1 %9")
      {
        continue;
      }

      brvec.push_back(BrInfo(moduleId, brCnt++, left, right));

      // printInstWithTab(bInst, 0, "Br");

      // bInst->print(common::print);

      // common::print << ss.str() << endl;

      // common::print << endl
      //               << op0->getNumUses() << endl;
      visitBrInst(M, op0->use_begin()->get(), brvec.back(), false, 1);

      Value *mId, *brId;

      IRBuilder<> IRB(bInst);

      mId = ConstantInt::get(M.getContext(), APInt(32, moduleId, false));
      brId = ConstantInt::get(M.getContext(), APInt(32, brCnt - 1, false));

      auto VisitBrF = IRB.CreateLoad(VisitBrPtr);
      CallInst *ProxyCall = IRB.CreateCall(VisitBrF, {mId, brId});
      setInsNonSan(ProxyCall);
    }

    // for (auto brinfo : brvec)
    // {
    //   common::print << "BranchID : " << brinfo.id << ",\t";
    //   common::print << "CmpVec : ";
    //   brinfo.print(common::print);
    //   common::print << endl;
    // }

    // // ! Iterate all BasicBlocks in this function and insert runtime function at CMP instructions.
    // for (tie(bb, ee) = vertices(cfG); bb != ee; bb++)
    // {
    //   auto &mBB = idBBMap[*bb];
    //   BasicBlock &BB = *(mBB.ptr);

    //   Value *mId = ConstantInt::get(M.getContext(), APInt(32, moduleId, false)), *cmpId = nullptr, *cmpType = nullptr, *argType = nullptr, *opSize = nullptr;

    //   for (auto insti = BB.rbegin(); insti != BB.rend(); insti++)
    //   {
    //     auto &inst = *insti;
    //     if (isa<CmpInst>(inst) && getCmpId(M, &inst) != -1)
    //     {
    //       CmpInst &cmpInst = dyn_cast<CmpInst>(inst);
    //       int cmpid = getCmpId(M, &inst);

    //       // ! Insert __maxafl_visit_cmp runtime function at cmp instructions
    //       IRBuilder<> IRB(&inst);
    //       Value *OpSArg[2];
    //       Value *OpZArg[2];

    //       OpSArg[0] = inst.getOperand(0);
    //       OpSArg[1] = inst.getOperand(1);
    //       OpSArg[0] = castCmpArgType(IRB, OpSArg[0], &cmpInst, true);
    //       OpSArg[1] = castCmpArgType(IRB, OpSArg[1], &cmpInst, true);
    //       OpZArg[0] = inst.getOperand(0);
    //       OpZArg[1] = inst.getOperand(1);
    //       OpZArg[0] = castCmpArgType(IRB, OpZArg[0], &cmpInst, false);
    //       OpZArg[1] = castCmpArgType(IRB, OpZArg[1], &cmpInst, false);

    //       if (cmpInst.isIntPredicate())
    //       {
    //         cmpId = ConstantInt::get(M.getContext(), APInt(32, cmpid, false));
    //         cmpType = ConstantInt::get(M.getContext(), APInt(32, cmpInst.getPredicate(), false));
    //         argType = ConstantInt::get(M.getContext(), APInt(32, OpSArg[0]->getType()->getTypeID(), false));
    //         opSize = ConstantInt::get(M.getContext(), APInt(32, 32, false));

    //         // CallInst *ProxyCall = IRB.CreateCall(VisitCmpIntFunc, {mId, cmpId, cmpType, argType, opSize, OpSArg[0], OpSArg[1], OpZArg[0], OpZArg[1]});
    //         // setInsNonSan(ProxyCall);

    //         auto VisitCmpIntF = IRB.CreateLoad(VisitCmpIntPtr);
    //         CallInst *ProxyCall = IRB.CreateCall(VisitCmpIntF, {mId, cmpId, cmpType, argType, opSize, OpSArg[0], OpSArg[1], OpZArg[0], OpZArg[1]});
    //         setInsNonSan(ProxyCall);
    //       }
    //       else
    //       {
    //         cmpId = ConstantInt::get(M.getContext(), APInt(32, cmpid, false));
    //         cmpType = ConstantInt::get(M.getContext(), APInt(32, cmpInst.getPredicate(), false));
    //         argType = ConstantInt::get(M.getContext(), APInt(32, OpSArg[0]->getType()->getTypeID(), false));
    //         opSize = ConstantInt::get(M.getContext(), APInt(32, 32, false));

    //         // CallInst *ProxyCall = IRB.CreateCall(VisitCmpFloatFunc, {mId, cmpId, cmpType, argType, opSize, OpSArg[0], OpSArg[1]});
    //         // setInsNonSan(ProxyCall);

    //         auto VisitCmpFloatF = IRB.CreateLoad(VisitCmpFloatPtr);
    //         CallInst *ProxyCall = IRB.CreateCall(VisitCmpFloatF, {mId, cmpId, cmpType, argType, opSize, OpSArg[0], OpSArg[1]});
    //         setInsNonSan(ProxyCall);
    //       }

    //       *infoFile << moduleId << "\t";
    //       *comInfoFile << moduleId << "\t";
    //       if (ConstantInt *CI = dyn_cast<ConstantInt>(cmpId))
    //       {
    //         if (CI->getBitWidth() <= 32)
    //         {
    //           *infoFile << CI->getSExtValue();
    //           *comInfoFile << CI->getSExtValue();
    //         }
    //       }
    //       if (ConstantInt *CI = dyn_cast<ConstantInt>(cmpType))
    //       {
    //         if (CI->getBitWidth() <= 32)
    //         {
    //           *infoFile << "\t" << CI->getSExtValue();
    //           *comInfoFile << "\t" << CI->getSExtValue();
    //         }
    //       }

    //       // *infoFile << "\t" << CI->getSExtValue();
    //       // *comInfoFile << "\t" << CI->getSExtValue();

    //       // for (unsigned int i = 0; i < bInst->getNumSuccessors(); i++)
    //       // {
    //       //   BasicBlock *succ = bInst->getSuccessor(i);
    //       //   MyBasicBlock mSucc(succ);

    //       //   MyBasicBlock &mBB = idBBMap[bbIdMap[mSucc]];

    //       //   *infoFile << "\t" << mBB.instCnt;
    //       //   *comInfoFile << "\t" << mBB.instCnt;
    //       // }
    //       *comInfoFile << endl;

    //       // TODO : break at final cmp instruction?
    //       // break;
    // }
    // }

    MyBasicBlock &mBB = idBBMap[bbIdMap[&(F->getEntryBlock())]];
    if (!F->hasName() || F->getName().empty())
    {
      funcMap["NoNameFunc"] = make_pair(moduleId, mBB.instCnt);
    }
    else
    {
      funcMap[F->getName().str()] = make_pair(moduleId, mBB.instCnt);
    }
  }

  // *comInfoFile << "BEGINCMPINFO" << endl;
  *comInfoFile << moduleId << "\t0\t" << cmpvec.size() << endl;
  for (auto cmpinfo : cmpvec)
  {
    cmpinfo.save(*infoFile);
    cmpinfo.save(*comInfoFile);
  }
  // *comInfoFile << "BEGINBRINFO" << endl;
  *comInfoFile << moduleId << "\t1\t" << brvec.size() << endl;
  for (auto brinfo : brvec)
  {
    brinfo.save(*infoFile);
    brinfo.save(*comInfoFile);
  }

  // ! Save module information at funcMap.map
  ofstream funcMapOStream;
  funcMapOStream.open(common::funcMapFileName);
  funcMapOStream << moduleId + 1 << endl;
  for (map<int, string>::iterator iter = moduleMap.begin(); iter != moduleMap.end(); iter++)
  {
    funcMapOStream << iter->first << "\t" << iter->second << endl;
  }
  for (map<string, pair<int, int>>::iterator iter = funcMap.begin(); iter != funcMap.end(); iter++)
  {
    funcMapOStream << iter->first << "\t" << iter->second.first << "\t" << iter->second.second << endl;
  }
  funcMapOStream.close();

  // ! Save optimized LLVM IR file
  M.print(*resultFile, NULL);

  // ! Free some variables that have used
  freeVariable(M);

  // save analyzed data to fuzz

  return true;
}

// ! Register required Passes
void MaxAFLPass::getAnalysisUsage(AnalysisUsage &AU) const
{
  // AU.setPreservesAll();
  // AU.setPreservesCFG();
  // AU.addPreserved<CFGPrinterPass>();
  AU.addRequiredTransitive<LoopInfoWrapperPass>();
  AU.addRequiredTransitive<CallGraphWrapperPass>();
  // AU.addRequired<TargetTransformInfoWrapperPass>();
  // AU.addRequired<SimplifyCFGPass>();
}

// ! Register my pass to PassManager
static RegisterPass<MaxAFLPass> X("maxafl_pass", "MaxAFLPass", false,
                                  false);

// static RegisterPass<SimplifyCFGPass> X2("simplifycfg", "SimplifyCFGPass", false, false);

static void registerMaxAFLPass(const PassManagerBuilder &,
                               legacy::PassManagerBase &PM)
{
  // PM.add(new SimplifyCFGPass());
  PM.add(createLowerSwitchPass());
  PM.add(new MaxAFLPass());
}

// static RegisterStandardPasses RegisterMaxAFLPass(
//     PassManagerBuilder::EP_EarlyAsPossible, registerMaxAFLPass);

static RegisterStandardPasses RegisterMaxAFLPass(
    PassManagerBuilder::EP_OptimizerLast, registerMaxAFLPass);

// static RegisterStandardPasses RegisterMaxAFLPass(
//     PassManagerBuilder::EP_ModuleOptimizerEarly, registerMaxAFLPass);

static RegisterStandardPasses RegisterMaxAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerMaxAFLPass);