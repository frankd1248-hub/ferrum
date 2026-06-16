#ifndef cuprum_lexer_h
#define cuprum_lexer_h

#include "common.h"
#include "error_reporter.h"
#include "token.h"

class Lexer {
public:
    Lexer(const std::string& src, ErrorReporter& err) : src(src), err(err) {}

    std::vector<Token> tokenize();

private:
    bool isAtEnd() const {
        return current >= (int) src.size();
    }

    bool isAlpha(char c) const {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    bool isDigit(char c) const {
        return c >= '0' && c <= '9';
    }

    char advance() {
        return src[current++];
    }

    char peek() const {
        if (isAtEnd()) return '\0';
        return src[current];
    }

    char peekNext() const {
        if (current + 1 >= (int) src.size()) return '\0';
        return src[current + 1];
    }

    char match(char expected) {
        if (isAtEnd()) return false;
        if (src[current] != expected) return false;
        current++;
        return true;
    }

    void addToken(TokenType type, const std::string& lexeme) {
        tokens.push_back({type, lexeme, line, current - lineStart});
    }

    void skipWhitespace() {
        while (!isAtEnd()) {
            char c = peek();
            if (c == ' ' || c == '\r' || c == '\t') {
                advance();
            } else if (c == '\n') {
                line++;
                lineStart = current;
                advance();
            } else {
                break;
            }
        }
    }

    TokenType identifierType(std::string lexeme) {
        if      (lexeme == "break")    return TK_BREAK;
        else if (lexeme == "bool")     return TK_BOOL;
        else if (lexeme == "char")     return TK_CHAR;
        else if (lexeme == "const")    return TK_CONST;
        else if (lexeme == "continue") return TK_CONTINUE;
        else if (lexeme == "else")     return TK_ELSE;
        else if (lexeme == "f32")      return TK_F32;
        else if (lexeme == "false")    return TK_FALSE;
        else if (lexeme == "fn")       return TK_FN;
        else if (lexeme == "for")      return TK_FOR;
        else if (lexeme == "i32")      return TK_I32;
        else if (lexeme == "if")       return TK_IF;
        else if (lexeme == "let")      return TK_LET;
        else if (lexeme == "native")   return TK_NATIVE;
        else if (lexeme == "return")   return TK_RETURN;
        else if (lexeme == "String")   return TK_STRING;
        else if (lexeme == "true")     return TK_TRUE;
        else if (lexeme == "void")     return TK_VOID;
        else if (lexeme == "while")    return TK_WHILE;

        return TK_IDENTIFIER;
    }

    std::vector<Token> tokens;

    std::string src;
    ErrorReporter& err;

    int start;
    int current;
    int lineStart;
    int line;

    void scanString();
    void scanChar();

};

#endif