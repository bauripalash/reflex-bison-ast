//
// The parser driver just glues together a parser object
// and a lexer object.
//

#include "lex.yy.h"
#include "ASTNode.h"
#include "EvalContext.h"
#include "Messages.h"
#include <unistd.h>
#include <iostream>
#include <fstream>

class Driver {
public:
    explicit Driver(reflex::Input in) : lexer(in), parser(new yy::parser(lexer, &root))
       { root = nullptr; }
    ~Driver() { delete parser; }
    AST::ASTNode* parse() {
        // parser->set_debug_level(1); // 0 = no debugging, 1 = full tracing
        // std::cout << "Running parser\n";
        int result = parser->parse();
        if (result == 0 && report::ok()) {  // 0 == success, 1 == failure
            // std::cout << "Extracting result\n";
            if (root == nullptr) {
                std::cout << "But I got a null result!  How?!\n";
            }
            return root;
        } else {
            std::cout << "Parse failed, no tree\n";
            return nullptr;
        }
    }
private:
    yy::Lexer   lexer;
    yy::parser *parser;
    AST::ASTNode *root;
};

void generate_code(AST::ASTNode *root) {
    CodegenContext ctx(std::cout);
    // Prologue
    ctx.emit("#include <stdio.h>");
    ctx.emit("int main(int argc, char **argv) {");
    // Body of generated code
    std::string target = ctx.alloc_reg();
    root->gen_rvalue(ctx, target);
    // Coda
    ctx.emit(std::string(R"(printf("-> %d\n",)")
        + target + ");");
    ctx.emit("}");
}

int main(int argc, char **argv)
{
    std::istream *source;
    /* Choices of output */
    int json = 0;
    int codegen = 0;
    int calcmode = 0;
    char opt;
    while ((opt = getopt (argc, argv, "jce")) != -1) {
        if (opt == 'j') { json = 1; }
        if (opt == 'e') { calcmode = 1;}
        if (opt == 'c') { codegen = 1; }
    }
    // The remaining argument should be a file name
    if (optind < argc) {
        std::cout << "Reading from file " << argv[1] << std::endl;
        std::ifstream srcfile;
        srcfile.open(argv[optind], std::ifstream::in);
        source = &srcfile;
        if (! srcfile.is_open()) {
            std::cerr << "Failed to open " << argv[optind] << std::endl;
            exit(5);
        } else {
            std::cerr << "Opened " << argv[optind] << std::endl;
            std::cerr <<  "Abandon hope, I don't seem to be able to read files " << std::endl;
        }
    } else {
        std::cout << "Reading from stdin" << std::endl;
        source = &std::cin;
    }

    Driver driver(source);
    AST::ASTNode* root = driver.parse();
    if (root != nullptr) {
        std::cout << "Parsed!\n";
        if (json) {
            AST::AST_print_context context;
            root->json(std::cout, context);
            std::cout << std::endl;
        }
        if (calcmode) {
            auto ctx = EvalContext();
            std::cout << "Evaluates to " << root->eval(ctx) << std::endl;
        }
        if (codegen) {
            std::cout << "/* BEGIN GENERATED CODE */" << std::endl;
            generate_code(root);
            std::cout << "/* END GENERATED CODE */" << std::endl;
        }
    } else {
        std::cout << "Extracted root was nullptr" << std::endl;
    }
}
