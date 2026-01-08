#include <llvm/Support/raw_ostream.h>
#include <onyx/lexer/Lexer.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        llvm::errs() << "Usage: onyxc <input file>\n";
        return 1;
    }
    
    std::string filename = argv[1];

    llvm::SourceMgr srcMgr;
    auto bufferOrErr = llvm::MemoryBuffer::getFile(filename);
    
    if (std::error_code ec = bufferOrErr.getError()) {
        llvm::errs() << "Could not open file: " << ec.message() << '\n';
        return 1;
    }
    srcMgr.AddNewSourceBuffer(std::move(*bufferOrErr), llvm::SMLoc());

    onyx::Lexer lex(srcMgr);

    while (1) {
        onyx::Token tok = lex.NextToken();
        if (tok.Is(onyx::TkEof)) {
            break;
        }
        llvm::outs() << tok.GetKind() << " '" << tok.GetText() << "' " << tok.GetLoc().getPointer() << '\n';
    }
    return 0;
}