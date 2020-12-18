#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "types.h"
#include "../types.h"
#include <streambuf>
#include <iostream>

#define endl "\n"

// #define LLVM_ATTRIBUTE_LIST AttributeSet

namespace common
{
// #define _MAXAFL_DEBUG
#define _MAXAFL_RELEASE

    // #ifdef _MAXAFL_DEBUG
    llvm::raw_ostream &print = llvm::outs();
    // #endif

    const std::string funcMapFileName = "funcMap.map";

#ifdef _MAXAFL_RELEASE
// std::error_code EC;
// llvm::StringRef fileName("/dev/null");
// llvm::raw_fd_ostream print(fileName, EC);
// print.set
// std::ofstream print;
// common::print.setstate(std::ios_base::badbit);
#endif

    // class prefixbuf
    //     : public std::streambuf
    // {
    //     std::string prefix;
    //     std::streambuf *sbuf;
    //     bool need_prefix;

    //     int sync()
    //     {
    //         return this->sbuf->pubsync();
    //     }
    //     int overflow(int c)
    //     {
    //         if (c != std::char_traits<char>::eof())
    //         {
    //             if (this->need_prefix && !this->prefix.empty() && this->prefix.size() != this->sbuf->sputn(&this->prefix[0], this->prefix.size()))
    //             {
    //                 return std::char_traits<char>::eof();
    //             }
    //             this->need_prefix = c == '\n';
    //         }
    //         return this->sbuf->sputc(c);
    //     }

    // public:
    //     prefixbuf(std::string const &prefix, std::streambuf *sbuf)
    //         : prefix(prefix), sbuf(sbuf), need_prefix(true)
    //     {
    //     }
    // };

    // class oprefixstream
    //     : private virtual prefixbuf,
    //       public llvm::raw_ostream
    // {
    // public:
    //     oprefixstream(std::string const &prefix, llvm::raw_ostream &out)
    //         : prefixbuf(prefix, out.rdbuf()), std::ios(static_cast<std::streambuf *>(this)), std::ostream(static_cast<std::streambuf *>(this))
    //     {
    //     }
    // };

    // oprefixstream dout("[DBG]\t", llvm::outs());
    // oprefixstream wout("[WARN]\t", llvm::outs());
    // oprefixstream eout("[ERR]\t", llvm::outs());

} // namespace common

std::string getSimpleNodeLabel(const llvm::BasicBlock *Node, const llvm::Function *)
{
    if (!Node->getName().empty())
        return Node->getName().str();

    std::string Str;
    llvm::raw_string_ostream OS(Str);

    Node->printAsOperand(OS, false);
    return OS.str();
};