#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <iostream>

enum Token {
    tok_eof = -1,
    tok_do = -2,
    tok_madame = -3,
    tok_identifier = -4,
    tok_number = -5
};

static std:: string IdentifierStr;
static double NumVal;


static int gettok() {
    static int LastChar = ' ';

    /*Skiping whitespace*/
    while (isspace(LastChar))
        LastChar = getchar();
    

    /*When a letter is encountered*/
    if (isalpha(LastChar)){

        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;
        
        if(IdentifierStr == "Do")
            return tok_do;
        if(IdentifierStr == "Madame")
            return tok_madame;
        
        return tok_identifier;
    }

    /*If number encountered*/
    if (isdigit(LastChar) || LastChar == '.'){
        std::string NumStr;
        int decimal_count = 0;

        do {
            NumStr += LastChar;
            LastChar = getchar();
            if(LastChar == '.') decimal_count++;
        } while (isdigit(LastChar) || decimal_count <= 1);

        if(decimal_count <= 1){
            NumVal = strtod(NumStr.c_str(), nullptr);
            return tok_number;
        }
    }
    
    /*If a comment encountered*/
    if (LastChar == '#') {

        do 
            LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if(LastChar != EOF)
            return gettok();
    }

    /*If End of File*/
    if(LastChar == EOF){
        return tok_eof;
    }

    /*ASCII Character*/
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}


// Parsing //


//Parser AST//
//All expression nodes
//TODO: Add type field
class ExprAST {
    public:
        virtual ~ExprAST() = default;
};

//Expression for numeric literals           
class NumberExprAST : public ExprAST {
    double Val;

    public:
        NumberExprAST(double Val) : Val(Val) {}
};

//Expression class for referencing a variable
class VariableExprAST : public ExprAST {
    std::string Name;

    public:
        VariableExprAST(const std::string &Name) : Name(Name) {}
};

//Expression class for a binary operator
class BinaryExprAST : public ExprAST {
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                        std::unique_ptr<ExprAST> RHS)
            : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

//Expression for function calls
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

    public:
        CallExprAST(const std::string &Callee,
                    std::vector<std::unique_ptr<ExprAST>> Args)
        : Callee(Callee), Args(std::move(Args)) {}
};

//Captures a function name and argument names(by extension also the number of arguments)
class PrototypeAST {

    std::string Name;
    std::vector<std::string> Args;

    public:
        PrototypeAST(const std::string &Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}

        const std::string &getName() const { return Name; }
};

class FunctionAST {

    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

    public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                    std::unique_ptr<ExprAST> Body)
                    : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

//Parser Code//

static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
}

//Routine for error handling
std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

// Number ::= Digit+
//Called when current token is tok_number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

// //paranexpr ::= '(' expression ')'
// static std::unique_ptr<ExprAST> ParseParenExpr() {
//     getNextToken();
//     auto V = ParseExpression();
//     if (!V)
//         return nullptr;
    
//     if (CurTok != ')')
//         return LogError("expected ')'");

//     getNextToken();
//     return V;
// }

