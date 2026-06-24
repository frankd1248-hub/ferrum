#include "lexer.h"

std::vector<Token> Lexer::tokenize() {
    tokens = {};
    start = 0;
    current = 0;
    lineStart = 0;
    line = 1;

    while (!isAtEnd()) {
        skipWhitespace();
        start = current;
        if (isAtEnd()) break;

        char c = advance();
        if (isAlpha(c)) {
            while (isAlpha(peek()) || isDigit(peek())) advance();
            std::string lexeme = src.substr(start, current - start);
            addToken(identifierType(lexeme), lexeme);
        } else if (isDigit(c)) {
            while (isDigit(peek())) advance();
            if (peek() == '.' && isDigit(peekNext())) {
                advance(); // consume '.'
                while (isDigit(peek())) advance();
                addToken(TK_FLOAT, src.substr(start, current - start));
            } else if (peek() == 'l' || peek() == 'L') {
                advance();
                addToken(TK_LNUMBER, src.substr(start, current - start - 1));
            } else {
                addToken(TK_NUMBER, src.substr(start, current - start));
            }
        } else {
            switch (c) {
                case '(': addToken(TK_LEFT_PAREN, "("); break;
                case ')': addToken(TK_RIGHT_PAREN, ")"); break;
                case '[': addToken(TK_LEFT_BRACKET,  "["); break;
                case ']': addToken(TK_RIGHT_BRACKET, "]"); break;
                case '{': addToken(TK_LEFT_BRACE, "{"); break;
                case '}': addToken(TK_RIGHT_BRACE, "}"); break;
                case ';': addToken(TK_SEMICOLON, ";"); break;
                case ':': addToken(TK_COLON, ":"); break;
                case ',': addToken(TK_COMMA, ","); break;
                case '.': addToken(TK_DOT, "."); break;
                case '-': addToken(TK_MINUS, "-"); break;
                case '+': addToken(TK_PLUS, "+"); break;
                case '*': addToken(TK_STAR, "*"); break;
                case '/': 
                    if (!match('/')) {
                        addToken(TK_SLASH, "/"); break;
                    }

                    while (!isAtEnd() && peek() != '\n')
                        advance();

                    if (!isAtEnd()) {
                        advance();
                        line++;
                        lineStart = current;
                    }
                    break;
                case '!': match('=') ? addToken(TK_BANG_EQUAL, "!=") : addToken(TK_BANG, "!"); break;
                case '<': match('=') ? addToken(TK_LESS_EQUAL, "<=") : addToken(TK_LESS, "<"); break;
                case '>': match('=') ? addToken(TK_GREATER_EQUAL, ">=") : addToken(TK_GREATER, ">"); break;
                case '=': 
                    if (match('>'))      addToken(TK_ARROW, "=>");
                    else if (match('=')) addToken(TK_EQUAL_EQUAL, "==");
                    else                 addToken(TK_EQUAL, "=");
                    break;
                case '"':  scanString(); break;
                case '\'': scanChar(); break;
                default:
                    err.report(line, current - lineStart, "Unexpected character.");
            }
        }
    }
    
    tokens.push_back({ TK_EOF, "", line, 0 });
    return tokens;
}

void Lexer::scanChar() {
    char value;
    if (peek() == '\\') {
        advance();
        switch (peek()) {
            case 'n':  value = '\n'; break;
            case 't':  value = '\t'; break;
            case '\\': value = '\\'; break;
            case '\'': value = '\''; break;
            default:   value = peek(); break;
        }
    } else {
        value = peek();
    }
    advance();
    if (peek() != '\'') { err.report(line, current - lineStart, "Unterminated char literal."); return; }
    advance();
    addToken(TK_CHAR_LIT, std::string(1, value));
}

void Lexer::scanString() {
    std::string value;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\\') {
            advance();
            switch (peek()) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case '\\': value += '\\'; break;
                case '"':  value += '"';  break;
                default:   value += peek(); break;
            }
        } else {
            if (peek() == '\n') { 
                line++; 
                lineStart = current + 1; 
            }
            value += peek();
        }
        advance();
    }
    if (isAtEnd()) { err.report(line, current - lineStart, "Unterminated string."); return; }
    advance();;
    addToken(TK_STRING_LIT, value);
}