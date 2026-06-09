#pragma once
#include <iostream>
#include <string>
using namespace std;
enum class TokenType
{
    END_OF_FILE,
    ILLEGAL,
    PLUS,                // +
    MINUS,               // -
    STAR,                // *
    SLASH,               // /
    LESS_THAN,           // <
    GREATER_THAN,        // >
    LESS_THAN_EQUALS,    // <=
    GREATER_THAN_EQUALS, // >=
    EQUALS,              // = (for assignment like let x = 10)
    DOUBLE_EQUALS,       // == (for comparison)
    NUMBER,              // 10, 42
    IDENTIFIER,          // x, myVar
    LET,                 // let
    IF,                  // if
    ELSE,                // else
    WHILE,               // while
    INPUT,               // input
    PRINT,               // print
    SEMICOLON,           // ;
    LEFT_PAREN,          //(
    RIGHT_PAREN,         //)
    LEFT_BRACE,          // {
    RIGHT_BRACE,         // }
};
struct Token
{
    TokenType type;
    string Lexed;
    int line; // for error display
};
class Lexer
{
private:
    int cursor;
    int line;
    int size;
    char current;
    string sourcecode;
    void skipwhitespace()
    {
        while (current == ' ' || current == '\n' || current == '\r' || current == '\t' || current == '/')
        {
            if (current == '/' && peak(1) == '/')
            { // for comments
                while (current != '\n' && current != '\0')
                {
                    advance();
                }
            }
            else if (current == '/')
            {
                break;
            }
            else
            {
                advance();
            }
        }
    }
    bool isdigit(char c)
    {
        if (c >= '0' && c <= '9')
        {
            return true;
        }
        else
            return false;
    }
    bool isalpha(char c)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'))
        {
            return true;
        }
        else
            return false;
    }
    Token readnumber()
    {
        string result = ""; // empty string
        while (isdigit(current) == true)
        {
            result = result + current;
            advance();
        }
        // result="123"
        return Token{TokenType::NUMBER, result, line};
    }
    Token readword()
    {
        string result = "";
        while (isalpha(current) == true || isdigit(current) == true)
        {
            result += current;
            advance();
        } //"a12"
        if (result == "let")
        {
            return Token{TokenType::LET, result, line};
        }
        else if (result == "if")
        {
            return Token{TokenType::IF, result, line};
        }
        else if (result == "else")
        {
            return Token{TokenType::ELSE, result, line};
        }
        else if (result == "while")
        {
            return Token{TokenType::WHILE, result, line};
        }
        else if (result == "input")
        {
            return Token{TokenType::INPUT, result, line};
        }
        else if (result == "print")
        {
            return Token{TokenType::PRINT, result, line};
        }
        return Token{TokenType::IDENTIFIER, result, line};
    }

public:
    Lexer(string source)
    {
        sourcecode = source;
        cursor = 0; // beginning of file
        size = sourcecode.length();
        current = sourcecode.at(cursor);
        line = 0;
    }
    void advance()
    {
        if (current == '\n')
        {
            line++;
        }
        if (cursor < size)
        {
            cursor++;
            current = (cursor < size) ? sourcecode[cursor] : '\0';
        }
        else
        {
            current = '\0';
        }
    }
    char peak(int offset = 0)
    {
        if ((cursor + offset) < size)
        {
            return sourcecode[cursor + offset];
        }
        else
        {
            return '\0';
        }
    }
    Token nextToken()
    {
        // moving step by step
        skipwhitespace();
        if (current == '\0')
        {
            return Token{TokenType::END_OF_FILE, "EOF", line};
        }
        if (isalpha(current) == true)
        {
            return readword();
        }
        if (isdigit(current) == true)
        {
            return readnumber();
        }
        if (current == '+')
        {
            advance();
            return Token{TokenType::PLUS, "+", line};
        }
        else if (current == '-')
        {
            advance();
            return Token{TokenType::MINUS, "-", line};
        }
        else if (current == '*')
        {
            advance();
            return Token{TokenType::STAR, "*", line};
        }
        else if (current == '/')
        {
            advance();
            return Token{TokenType::SLASH, "/", line};
        }
        if (current == ';')
        {
            advance();
            return Token{TokenType::SEMICOLON, ";", line};
        }
        if (current == '(')
        {
            advance();
            return Token{TokenType::LEFT_PAREN, "(", line};
        }
        if (current == ')')
        {
            advance();
            return Token{TokenType::RIGHT_PAREN, ")", line};
        }
        if (current == '{')
        {
            advance();
            return Token{TokenType::LEFT_BRACE, "{", line};
        }
        if (current == '}')
        {
            advance();
            return Token{TokenType::RIGHT_BRACE, "}", line};
        }
        if (current == '=')
        {
            if (peak(1) == '=')
            {
                advance();
                advance();
                return Token{TokenType::DOUBLE_EQUALS, "==", line};
            }
            else
            {
                advance();
                return Token{TokenType::EQUALS, "=", line};
            }
        }
        if (current == '>')
        {
            if (peak(1) == '=')
            {
                advance();
                advance();
                return Token{TokenType::GREATER_THAN_EQUALS, ">=", line};
            }
            else
            {
                advance();
                return Token{TokenType::GREATER_THAN, ">", line};
            }
        }
        if (current == '<')
        {
            if (peak(1) == '=')
            {
                advance();
                advance();
                return Token{TokenType::LESS_THAN_EQUALS, "<=", line};
            }
            else
            {
                advance();
                return Token{TokenType::LESS_THAN, "<", line};
            }
        }
        string illegal(1, current); // creates a one char long string
        advance();
        return Token{TokenType::ILLEGAL, illegal, line};
    }
};