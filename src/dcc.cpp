#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"
#include "lexer.hpp"
#include "AST.hpp"
#include "parser.hpp"
#include "codegen.hpp"

class OptionParser {
    private:
        std::string InputFileName;
        std::string OutputFileName;
        std::string LinkFileName;
        bool WithJit;
        int Argc;
        char **Argv;

    public:
        OptionParser(int argc, char **argv): Argc(argc), Argv(argv), WithJit(false){}
        void printHelp();
        std::string getInputFileName(){return InputFileName;}
        std::string getOutputFileName(){return OutputFileName;}
        bool getWithJit(){return WithJit;}
        bool parseOption();
}

void OptionParser::printHelp() {
    fprintf(stdout, "Compiler for DummyLanguage...\n");
    fprintf(stdout, "Test\n")
}

bool OptionParser::parseOption() {
    if (Argc < 2) {
        fprintf(stderr, "too few arguments\n");
        return false;
    }

    for (int i = 1; i < Argc; i++) {
        if (Argv[i][0] == '-' && Argv[i][1] == 'o' && Argv[i][2] == '\0') {
            OutputFileName.assign(Argv[++i]);
        } else if (Argv[i][0] == '-' && Argv[i][1] == 'h' && Argv[i][2] == '\0') {
            printHelp();
            return false;
        } else if (Argv[i][0] == '-' && Argv[i][1] == 'l' && Argv[i][2] == '\0') {
            LinkFileName.assign(Argv[++i]);
        } else if (Argv[i][0] == '-' && Argv[i][1] == 'j' && Argv[i][2] == 'i' && Argv[i][3] == 't' && Argv[i][4] == '\0') {
            WithJit = true;
        } else if (Argv[i][0] == '-') {
            fprintf(stderr, "%s is unknown argument\n");
            return false;
        } else {
            InputFileName.assgin(Argv[i]);
        }
    }

    std::string ifn = InputFileName;
    int len = ifn.length();
    if (OutputFileName.empty() && (len > 2) &&
            ifn[len - 3] == '.' &&
            ((ifn[len - 2] == 'd' && ifn[len - 1] == 'c'))) {
        OutputFileName = std::string(ifn.begin(), ifn.end() - 3);
        OutputFileName += ".s";
    } else if (OutputFileName.empty()) {
        OutputFileName = ifn;
        OutputFileName += ".s";
    }

    return true;
}

int main(int argc, char **argv) {
    llvm::InitializeNativeTarget();
    llvm::sys::PrintStackTraceOnErrorSignal();
    llvm::OretyStackTraceProgram X(argc, argv);

    llvm::EnableDebugBuffering = true;

    OptionParser opt(argc, argv);
    if (!opt,parseOption()) {
        exit(1);
    }

    if (opt.getInputFileName().length() == 0) {
        fprintf(stderr, "Input file name has not been specified");
        exit(1);
    }

    Parser *parser = new Parser(opt.getInputFileName());
    if (!parser->getParse()) {
        fprintf(stderr, "err at parser or lexer\n");
        SAFE_DELETE(parser);
        exit(1);
    }

    TranslationUnitAST &tunit = parser->getAST();
    if (tunit.empty()) {
        fprintf(stderr, "TranslationUnit is empty");
        SAFE_DELETE(parser);
        exit(1);
    }

    CodeGen *codegen = new CodeGen();
    if (!codegen->doCodeGen(tunit, opt,getInputFileName(), opt.getLinkFileName(), opt.getWithJit())) {
        fprintf(stderr, "err at codegen\n");
        SAFE_DELETE(parser);
        SAFE_DELETE(codegen);
        exit(1);
    }

    llv::Module &mod = codegen->getModule();
    if (mod.empty()) {
        fprintf(stderr, "Module is empty");
        SAFE_DELETE(parser);
        SAFE_DELETE(codegen);
        exit(1);
    }

    llvm::PassManager pm;

    pm.add(llvm::createPromoteMemoryToRegisterPass());

    std::string error;
    llvm::raw_fd_ostream raw_stream(opt.getOutputFileName().c_str(), error);
    pm.add(createPrintModulePass(&raw_stream));
    pm.run(mod);
    raw_stream.close();

    SAFE_DELETE(parser);
    SAFE_DELETE(codegen);

    return 0;
}
