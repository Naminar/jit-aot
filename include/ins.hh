#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <stack>

class Instruction;

struct Operand {
    enum Type { Int, Inst, Undef } type;
    int intVal;
    Instruction* instVal;

    Operand(int v) : type(Int), intVal(v), instVal(nullptr) {}
    Operand(Instruction* i) : type(Inst), intVal(0), instVal(i) {}
    Operand() : type(Undef), intVal(0), instVal(nullptr) {}

    bool operator==(const Operand& other) const {
        if (type != other.type) return false;
        if (type == Int) return intVal == other.intVal;
        if (type == Inst) return instVal == other.instVal;
        return true;
    }

    std::string toString() const;
};

class Instruction {
public:
    enum class Type { Regular, Terminator, Phi };
    std::string name;
    bool erased = false;
    int id = -1;
    int line = -1;

    int allocatedReg = -1;
    int allocatedStackSlot = -1;

    // Dataflow
    std::vector<Instruction*> operands;
    std::vector<Instruction*> users;

    virtual ~Instruction() = default;
    
    virtual Type getInstType() const { return Type::Regular; }
    
    virtual void print() const = 0;
    
    virtual std::vector<std::string> getSuccessorLabels() const { return {}; }

    void addOperand(Instruction* inst) {
        if (inst) {
            operands.push_back(inst);
            inst->users.push_back(this);
        }
    }

    void dropOperands() {
        for (Instruction* op : operands) {
            auto it = std::find(op->users.begin(), op->users.end(), this);
            if (it != op->users.end()) {
                op->users.erase(it);
            }
        }
        operands.clear();
    }

    virtual void replaceOperand(Instruction* oldOp, Operand newOp) {
        auto it = std::find(operands.begin(), operands.end(), oldOp);
        
        if (it != operands.end()) {
            if (newOp.type == Operand::Inst) {
                *it = newOp.instVal;

                newOp.instVal->users.push_back(this);
            } else {
                operands.erase(it);
            }
        }
    }

    std::string loc() const {
        if (allocatedReg != -1) return " [R" + std::to_string(allocatedReg) + "]";
        // if (allocatedStackSlot != -1) return " [S" + std::to_string(allocatedStackSlot) + "]";
        return "";
    }

};

inline std::string Operand::toString() const {
    if (type == Int) return std::to_string(intVal);
    if (type == Inst && instVal) return instVal->name + instVal->loc();
    return "undef";
}

class ParamInst : public Instruction {
public:
    int paramIdx;
    ParamInst(int idx) : paramIdx(idx) {}
    void print() const override {
        std::cout << "  " << name << loc() << " = param " << paramIdx << "\n";
    }
};

class CallInst : public Instruction {
public:
    std::string funcName;
    
    // ptr to IRBuilder
    void* targetFunc;
    std::vector<Operand> args;
    
    CallInst(const std::string& fname, void* target, const std::vector<Operand>& a)
        : funcName(fname), targetFunc(target), args(a) {
        for (auto& arg : args) {
            if (arg.type == Operand::Inst) addOperand(arg.instVal);
        }
    }
    
    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        for (auto& arg : args) {
            if (arg.type == Operand::Inst && arg.instVal == oldOp) {
                arg = newOp;
            }
        }
    }
    
    void print() const override {
        std::cout << "  " << name << loc() << " = call " << funcName << "(";
        for (size_t i = 0; i < args.size(); ++i) {
            std::cout << args[i].toString();
            if (i + 1 < args.size()) std::cout << ", ";
        }
        std::cout << ")\n";
    }
};

class BinaryInst : public Instruction {
public:
    enum Op { Add, Sub, Mul, And, AShr };
    Op op;
    Operand lhs, rhs;

    BinaryInst(Op o, Operand a, Operand b) : op(o), lhs(a), rhs(b) {
        if (a.type == Operand::Inst) addOperand(a.instVal);
        if (b.type == Operand::Inst) addOperand(b.instVal);
    }

    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        if (lhs.type == Operand::Inst && lhs.instVal == oldOp) lhs = newOp;
        if (rhs.type == Operand::Inst && rhs.instVal == oldOp) rhs = newOp;
    }

    void print() const override {
        const char* opStr = "";
        switch(op) {
            case Add: opStr = "add"; break;
            case Sub: opStr = "sub"; break;
            case Mul: opStr = "mul"; break;
            case And: opStr = "and"; break;
            case AShr: opStr = "ashr"; break;
        }
        std::cout << "  " << name << loc() << " = " << opStr << " " << lhs.toString() << ", " << rhs.toString() << "\n";
    }
};

class ICmpInst : public Instruction {
public:
    enum Pred { EQ, NE, SGT, SLT, SGE, SLE };
    Pred pred;
    Operand lhs, rhs;
    
    ICmpInst(Pred p, Operand a, Operand b) : pred(p), lhs(a), rhs(b) {
        if (a.type == Operand::Inst) addOperand(a.instVal);
        if (b.type == Operand::Inst) addOperand(b.instVal);
    }

    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        if (lhs.type == Operand::Inst && lhs.instVal == oldOp) lhs = newOp;
        if (rhs.type == Operand::Inst && rhs.instVal == oldOp) rhs = newOp;
    }

    void print() const override {
        const char* predStr = "";
        switch (pred) {
            case EQ:  predStr = "eq"; break;
            case NE:  predStr = "ne"; break;
            case SGT: predStr = "sgt"; break;
            case SLT: predStr = "slt"; break;
            case SGE: predStr = "sge"; break;
            case SLE: predStr = "sle"; break;
        }
        std::cout << "  " << name << loc() << " = icmp " << predStr << " " << lhs.toString() << ", " << rhs.toString() << "\n";
    }
};

class AllocaInst : public Instruction {
public:
    AllocaInst() {}
    void print() const override {
        std::cout << "  " << name << loc() << " = alloca\n";
    }
};

class LoadInst : public Instruction {
public:    
    Operand ptr; 
    LoadInst(Operand p) : ptr(p) {
        if (p.type == Operand::Inst) addOperand(p.instVal);
    }

    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        if (ptr.type == Operand::Inst && ptr.instVal == oldOp) ptr = newOp;
    }

    void print() const override {
        std::cout << "  " << name << loc() << " = load " << ptr.toString() << "\n";
    }
};

class StoreInst : public Instruction {
public:
    Operand val, ptr;
    
    StoreInst(Operand v, Operand p) : val(v), ptr(p) {
        if (v.type == Operand::Inst) addOperand(v.instVal);
        if (p.type == Operand::Inst) addOperand(p.instVal);
    }
    
    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        if (val.type == Operand::Inst && val.instVal == oldOp) val = newOp;
        if (ptr.type == Operand::Inst && ptr.instVal == oldOp) ptr = newOp;
    }

    void print() const override {
        std::cout << "  store " << val.toString() << ", " << ptr.toString() << "\n";
    }
};

class NullCheckInst : public Instruction {
public:
    Operand ptr;
    NullCheckInst(Operand p) : ptr(p) {
        if (p.type == Operand::Inst) addOperand(p.instVal);
    }

    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        if (ptr.type == Operand::Inst && ptr.instVal == oldOp) ptr = newOp;
    }

    void print() const override {
        std::cout << "  check.null " << ptr.toString() << "\n";
    }
};

class BoundsCheckInst : public Instruction {
public:
    Operand ptr, idx;
    BoundsCheckInst(Operand p, Operand i) : ptr(p), idx(i) {
        if (p.type == Operand::Inst) addOperand(p.instVal);
        if (i.type == Operand::Inst) addOperand(i.instVal);
    }

    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        if (ptr.type == Operand::Inst && ptr.instVal == oldOp) ptr = newOp;
        if (idx.type == Operand::Inst && idx.instVal == oldOp) idx = newOp;
    }

    void print() const override {
        std::cout << "  check.bounds " << ptr.toString() << ", " << idx.toString() << "\n";
    }
};


class SpillInst : public Instruction {
public:
    Operand val;
    int stackSlot;
    
    SpillInst(Operand v, int slot) : val(v), stackSlot(slot) {
        if (v.type == Operand::Inst) addOperand(v.instVal);
    }

    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        if (val.type == Operand::Inst && val.instVal == oldOp) val = newOp;
    }

    void print() const override {
        std::cout << "  spill " << val.toString() << " -> [S" << stackSlot << "]\n";
    }
};

class FillInst : public Instruction {
public:
    int stackSlot;
    
    FillInst(int slot) : stackSlot(slot) {}

    void print() const override {
        std::cout << "  " << name << loc() << " = fill [S" << stackSlot << "]\n";
    }
};


class TerminatorInst : public Instruction {
public:
    enum TermType { Ret, Br, BrCond };
    TermType type;
    Operand val;
    std::vector<std::string> succLabels;

    TerminatorInst(TermType t, Operand v, std::vector<std::string> lbls) 
        : type(t), val(v), succLabels(std::move(lbls)) {
        if (val.type == Operand::Inst) addOperand(val.instVal);
    }

    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        if (val.type == Operand::Inst && val.instVal == oldOp) val = newOp;
    }

    Type getInstType() const override { return Type::Terminator; }

    std::vector<std::string> getSuccessorLabels() const override { return succLabels; }
    
    void print() const override {
        std::cout << "  ";
        if (type == Ret) {
            if (val.type == Operand::Undef) std::cout << "ret void";
            else std::cout << "ret i32 " << val.toString();
        } else if (type == Br) {
             std::cout << "br label %" << succLabels[0];
        } else if (type == BrCond) {
             std::cout << "br " << val.toString() << ", label %" << succLabels[0] << ", label %" << succLabels[1];
        }
        std::cout << "\n";
    }
};

class PhiInst : public Instruction {
public:
    struct Incoming {
        Operand val;
        std::string blockName;
    };
    std::vector<Incoming> incomings;

    void addIncoming(Operand val, const std::string& block) {
        incomings.push_back({val, block});
        if (val.type == Operand::Inst) addOperand(val.instVal);
    }

    void replaceOperand(Instruction* oldOp, Operand newOp) override {
        Instruction::replaceOperand(oldOp, newOp);
        for (auto& inc : incomings) {
            if (inc.val.type == Operand::Inst && inc.val.instVal == oldOp) {
                inc.val = newOp;
            }
        }
    }

    Type getInstType() const override { return Type::Phi; }

    void print() const override {
        std::cout << "  " << name << loc() << " = phi";
        for (size_t i = 0; i < incomings.size(); ++i) {
            if (i == 0) 
                std::cout << " [ " << incomings[i].val.toString() << ", %" << incomings[i].blockName << " ]";
            else
                std::cout << ", [ " << incomings[i].val.toString() << ", %" << incomings[i].blockName << " ]";
        }
        std::cout << "\n";
    }
};