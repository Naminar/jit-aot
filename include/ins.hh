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

    std::string toString() const;
};

class Instruction {
public:
    enum class Type { Regular, Terminator, Phi };
    std::string name;

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
};

inline std::string Operand::toString() const {
    if (type == Int) return std::to_string(intVal);
    if (type == Inst && instVal) return instVal->name;
    return "undef";
}

class BinaryInst : public Instruction {
public:
    enum Op { Add, Sub, Mul };
    Op op;
    Operand lhs, rhs;

    BinaryInst(Op o, Operand a, Operand b) : op(o), lhs(a), rhs(b) {

        if (a.type == Operand::Inst) addOperand(a.instVal);
        if (b.type == Operand::Inst) addOperand(b.instVal);
    }

    void print() const override {
        const char* opStr = (op == Add) ? "add" : (op == Sub) ? "sub" : "mul";
        std::cout << "  " << name << " = " << opStr << " " << lhs.toString() << ", " << rhs.toString() << "\n";
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
        std::cout << "  " << name << " = icmp " << predStr << " " << lhs.toString() << ", " << rhs.toString() << "\n";
    }
};

class AllocaInst : public Instruction {
public:
    AllocaInst() {}
    
    void print() const override {
        std::cout << "  " << name << " = alloca\n";
    }
};

class LoadInst : public Instruction {
    Operand ptr;

public:    
    LoadInst(Operand p) : ptr(p) {
        if (p.type == Operand::Inst) addOperand(p.instVal);
    }

    void print() const override {
        std::cout << "  " << name << " = load " << ptr.toString() << "\n";
    }
};

class StoreInst : public Instruction {
public:
    Operand val, ptr;
    
    StoreInst(Operand v, Operand p) : val(v), ptr(p) {
        if (v.type == Operand::Inst) addOperand(v.instVal);
        if (p.type == Operand::Inst) addOperand(p.instVal);
    }
    
    void print() const override {
        std::cout << "  store " << val.toString() << ", " << ptr.toString() << "\n";
    }
};

class TerminatorInst : public Instruction {
    std::string repr;
    std::vector<std::string> succLabels;

public:
    // Regular terminator (ret void, unconditional br)
    TerminatorInst(const std::string& r, const std::vector<std::string> labels)
        : repr(r), succLabels(std::move(labels)) {}

    // Terminator with dependency (ret val, conditional br)
    TerminatorInst(const std::string& r, const std::vector<std::string> labels, Operand op)
        : repr(r), succLabels(std::move(labels)) {
        if (op.type == Operand::Inst) addOperand(op.instVal);
    }

    Type getInstType() const override { return Type::Terminator; }

    std::vector<std::string> getSuccessorLabels() const override { return succLabels; }
    
    void print() const override { std::cout << "  " << repr << "  ; <terminator>\n"; }
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

    Type getInstType() const override { return Type::Phi; }

    void print() const override {
        std::cout << "  " << name << " = phi";
        for (size_t i = 0; i < incomings.size(); ++i) {
            if (i == 0) 
                std::cout << " [ " << incomings[i].val.toString() << ", %" << incomings[i].blockName << " ]";
            else
                std::cout << ", [ " << incomings[i].val.toString() << ", %" << incomings[i].blockName << " ]";
        }
        std::cout << "\n";
    }
};