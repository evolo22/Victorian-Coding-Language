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

        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
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

    /*ASCIII Character*/
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}


int main() {
    while (true) {
        int tok = gettok();

        if (tok == tok_eof) {
            std::cout << "tok_eof\n";
            break;
        }

        switch (tok) {
            case tok_do:
                std::cout << "tok_do\n";
                break;

            case tok_madame:
                std::cout << "tok_madame\n";
                break;

            case tok_identifier:
                std::cout << "tok_identifier: " << IdentifierStr << "\n";
                break;

            case tok_number:
                std::cout << "tok_number: " << NumVal << "\n";
                break;

            default:
                // ASCII character
                std::cout << "char: '" << static_cast<char>(tok) << "'\n";
                break;
        }
    }

    return 0;
}
