
#include "irbuilder.hh"

int main() {
    IRBuilder builder;

    auto* entry = builder.createBasicBlock("entry");
    builder.createInstruction("%x = alloca i32");
    builder.createInstruction("store i32 10, i32* %x");
    builder.createInstruction("%cmp = icmp sgt i32 10, 0");
    builder.createBr("%cmp", "then", "else");

    auto* thenBB = builder.createBasicBlock("then");
    builder.createInstruction("%a = add i32 10, 1");
    builder.createBr("merge");

    auto* elseBB = builder.createBasicBlock("else");
    builder.createInstruction("%b = sub i32 10, 1");
    builder.createBr("merge");

    auto* mergeBB = builder.createBasicBlock("merge");
    auto* phi = builder.createPHI();
    phi->addIncoming("%a", "then");
    phi->addIncoming("%b", "else");
    builder.createRet("%phi");

    builder.dump();

    return 0;
}