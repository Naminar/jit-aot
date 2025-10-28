
#include "irbuilder.hh"

// int main() {
//     IRBuilder builder;

//     auto* entry = builder.createBasicBlock("entry");
//     auto x = builder.createAlloca();
//     builder.createStore("42", x);
//     auto val = builder.createLoad(x);
//     auto cmp = builder.createICmp(ICmpInst::SGT, val, "0");
//     builder.createBr(cmp, "then", "else");

//     auto* thenBB = builder.createBasicBlock("then");
//     auto a = builder.createAdd(val, "1");
//     builder.createBr("merge");

//     auto* elseBB = builder.createBasicBlock("else");
//     auto b = builder.createSub(val, "1");
//     builder.createBr("merge");

//     auto* mergeBB = builder.createBasicBlock("merge");
//     auto* phi = builder.createPHI();
//     phi->addIncoming(a, "then");
//     phi->addIncoming(b, "else");
//     builder.createRet(phi->name);

//     builder.dump();

//     builder.printDominators();

//     return 0;
// }

// int main() {
//     IRBuilder builder;

//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr(" ", "C", "F");

//     builder.createBasicBlock("C");
//     builder.createBr("D");

//     builder.createBasicBlock("D");
//     builder.createRet(" ");

//     builder.createBasicBlock("E");
//     builder.createBr("D");

//     builder.createBasicBlock("F");
//     builder.createBr(" ", "E", "G");

//     builder.createBasicBlock("G");
//     builder.createBr("D");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }


// int main() {
//     IRBuilder builder;
//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr(" ", "C", "J");

//     builder.createBasicBlock("C");
//     builder.createBr("D");

//     builder.createBasicBlock("D");
//     builder.createBr(" ", "C", "E");

//     builder.createBasicBlock("E");
//     builder.createBr("F");

//     builder.createBasicBlock("F");
//     builder.createBr(" ", "E", "G");

//     builder.createBasicBlock("J");
//     builder.createBr("C");

//     builder.createBasicBlock("G");
//     builder.createBr(" ", "H", "I");

//     builder.createBasicBlock("H");
//     builder.createBr("B");

//     builder.createBasicBlock("I");
//     builder.createBr("K");

//     builder.createBasicBlock("K");
//     builder.createRet(" ");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }


int main() {
    IRBuilder builder;
    builder.createBasicBlock("entry");
    builder.createBr("B");

    builder.createBasicBlock("B");
    builder.createBr(" ", "E", "C");

    builder.createBasicBlock("C");
    builder.createBr("D");

    builder.createBasicBlock("D");
    builder.createBr("G");

    builder.createBasicBlock("E");
    builder.createBr(" ", "F", "D");

    builder.createBasicBlock("F");
    builder.createBr(" ", "B", "H");

    builder.createBasicBlock("G");
    builder.createBr(" ", "C", "I");

    builder.createBasicBlock("H");
    builder.createBr(" ", "G", "I");

    builder.createBasicBlock("I");
    builder.createRet(" ");

    builder.dump();

    builder.printDominators();

    builder.analyzeLoops();

    return 0;
}

//---------------------------- special for loop test ----------------------------------------

// int main () {
//     IRBuilder builder;
//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr(" ", "C", "D");

//     builder.createBasicBlock("C");
//     builder.createRet(" ");

//     builder.createBasicBlock("D");
//     builder.createBr("E");

//     builder.createBasicBlock("E");
//     builder.createBr("B");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }


// int main () {
//     IRBuilder builder;
//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr("C");

//     builder.createBasicBlock("C");
//     builder.createBr(" ", "D", "E");

//     builder.createBasicBlock("D");
//     builder.createRet(" ");

//     builder.createBasicBlock("E");
//     builder.createBr(" ", "D", "F");

//     builder.createBasicBlock("F");
//     builder.createBr("B");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }

// int main () {
//     IRBuilder builder;
//     builder.createBasicBlock("entry");
//     builder.createBr("B");

//     builder.createBasicBlock("B");
//     builder.createBr(" ", "C", "G");

//     builder.createBasicBlock("G");
//     builder.createBr(" ", "D", "I");

//     builder.createBasicBlock("I");
//     builder.createRet(" ");

//     builder.createBasicBlock("C");
//     builder.createBr("D");

//     builder.createBasicBlock("D");
//     builder.createBr("E");

//     builder.createBasicBlock("E");
//     builder.createBr(" ", "B", "F");

//     builder.createBasicBlock("F");
//     builder.createBr("entry");

//     builder.dump();

//     builder.printDominators();

//     builder.analyzeLoops();

//     return 0;
// }

