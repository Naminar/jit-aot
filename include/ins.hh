#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <stdexcept>

class Instruction {
public:
    enum class Type { Regular, Terminator, Phi };
    std::string name;

    virtual ~Instruction() = default;
    
    virtual Type getInstType() const { return Type::Regular; }
    
    virtual void print() const = 0;
    
    virtual std::vector<std::string> getSuccessorLabels() const { return {}; }
};

class BinaryInst : public Instruction {
public:
    enum Op { Add, Sub, Mul };
    Op op;
    std::string lhs, rhs;

    BinaryInst(Op o, const std::string& a, const std::string& b) : op(o), lhs(a), rhs(b) {}

    void print() const override {
        const char* opStr = (op == Add) ? "add" : (op == Sub) ? "sub" : "mul";
        std::cout << "  " << name << " = " << opStr << " " << lhs << ", " << rhs << "\n";
    }
};

class ICmpInst : public Instruction {
public:
    enum Pred { EQ, NE, SGT, SLT, SGE, SLE };
    Pred pred;
    std::string lhs, rhs;
    
    ICmpInst(Pred p, const std::string& a, const std::string& b) : pred(p), lhs(a), rhs(b) {}

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
        std::cout << "  " << name << " = icmp " << predStr << " " << lhs << ", " << rhs << "\n";
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
    std::string ptr;

public:    
    LoadInst(const std::string& p) : ptr(p) {}

    void print() const override {
        std::cout << "  " << name << " = load " << ptr << "\n";
    }
};

class StoreInst : public Instruction {
public:
    std::string val, ptr;
    
    StoreInst(const std::string& v, const std::string& p) : val(v), ptr(p) {}
    
    void print() const override {
        std::cout << "  store " << val << ", " << ptr << "\n";
    }
};

class TerminatorInst : public Instruction {
    std::string repr;
    std::vector<std::string> succLabels;

public:
    TerminatorInst(const std::string& r, const std::vector<std::string> labels)
        : repr(r), succLabels(std::move(labels)) {}

    Type getInstType() const override { return Type::Terminator; }

    std::vector<std::string> getSuccessorLabels() const override { return succLabels; }
    
    void print() const override { std::cout << "  " << repr << "  ; <terminator>\n"; }
};

class PhiInst : public Instruction {
public:
    struct Incoming {
        std::string value;
        std::string blockName;
    };
    std::vector<Incoming> incomings;

    void addIncoming(const std::string& val, const std::string& block) {
        incomings.push_back({val, block});
    }

    Type getInstType() const override { return Type::Phi; }

    void print() const override {
        std::cout << "  " << name << " = phi";
        for (size_t i = 0; i < incomings.size(); ++i) {
            if (i == 0) 
                std::cout << " [ " << incomings[i].value << ", %" << incomings[i].blockName << " ]";
            else
                std::cout << ", [ " << incomings[i].value << ", %" << incomings[i].blockName << " ]";
        }
        std::cout << "\n";
    }
};