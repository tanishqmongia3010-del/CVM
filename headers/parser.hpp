// for checking syntax of language
#pragma once
#include "lexer.hpp"
#include "ast.hpp"
#include <iostream>
#include <memory> // for unique ptr
using namespace std;
class Parser
{
private:
    Lexer lexer;
    Token currenttoken;

public:
    void advance()
    {
        currenttoken = lexer.nextToken();
    }
    Parser(string sourcecode) : lexer(sourcecode)
    {
        advance();
    }
    // Staircase of Precedence for BODMAS
    unique_ptr<expression> parsePrimary()
    {
        // for supporting negative numbers
        if (currenttoken.type == TokenType::MINUS)
        {
            int l = currenttoken.line;
            advance(); // step over the -
            unique_ptr<expression> right = parsePrimary();
            auto zero = make_unique<numbernode>("0", l);
            return make_unique<operatornode>(move(zero), "-", move(right), l);
        }
        if (currenttoken.type == TokenType::NUMBER)
        {
            string val = currenttoken.Lexed;
            int l = currenttoken.line;
            advance();
            return make_unique<numbernode>(val, l);
        }
        else if (currenttoken.type == TokenType::IDENTIFIER)
        {
            string name = currenttoken.Lexed;
            int l = currenttoken.line;
            advance();
            return make_unique<identifierNode>(name, l);
        }
        else if (currenttoken.type == TokenType::INPUT)
        {
            int l = currenttoken.line;
            advance();
            return make_unique<InputExpr>(l);
        }
        else if (currenttoken.type == TokenType::LEFT_PAREN)
        {
            advance();
            unique_ptr<expression> innerMath = parsecomparison();
            if (currenttoken.type == TokenType::RIGHT_PAREN)
            {
                advance();
                return innerMath;
            }
            else
            {
                cout << "Syntax Error: Missing ')' on line " << currenttoken.line << endl;
                return nullptr;
            }
        }
        else
        {
            cout << "Syntax Error on line " << currenttoken.line << ": Unexpected token '" << currenttoken.Lexed << "'" << endl;
            return nullptr;
        }
    }
    unique_ptr<expression> parseTerm()
    {
        // for multiplication and division
        unique_ptr<expression> left = parsePrimary();
        // while loop to handle chained multiplication and division
        while (currenttoken.type == TokenType::STAR || currenttoken.type == TokenType::SLASH)
        {
            string op = currenttoken.Lexed;
            int l = currenttoken.line;
            advance();
            unique_ptr<expression> right = parsePrimary();
            left = make_unique<operatornode>(move(left), op, move(right), l);
        }
        return left;
    }
    unique_ptr<expression> parseexpression()
    {
        // for addition and subtraction
        unique_ptr<expression> left = parseTerm();
        while (currenttoken.type == TokenType::PLUS || currenttoken.type == TokenType::MINUS)
        {
            string op = currenttoken.Lexed;
            int l = currenttoken.line;
            advance();
            unique_ptr<expression> right = parseTerm();
            left = make_unique<operatornode>(move(left), op, move(right), l);
        }
        return left;
    }
    unique_ptr<expression> parsecomparison()
    {
        unique_ptr<expression> left = parseexpression();
        while (currenttoken.type == TokenType::DOUBLE_EQUALS ||
               currenttoken.type == TokenType::LESS_THAN ||
               currenttoken.type == TokenType::GREATER_THAN ||
               currenttoken.type == TokenType::LESS_THAN_EQUALS ||
               currenttoken.type == TokenType::GREATER_THAN_EQUALS)
        {
            string op = currenttoken.Lexed;
            int l = currenttoken.line;
            advance();
            unique_ptr<expression> right = parseexpression();
            left = make_unique<operatornode>(move(left), op, move(right), l);
        }
        return left;
    }
    unique_ptr<BlockNode> parseblock()
    {
        // function that returns unique ptr to block
        int l = currenttoken.line;
        advance();
        unique_ptr<BlockNode> block = make_unique<BlockNode>(l);
        while (currenttoken.type != TokenType::RIGHT_BRACE && currenttoken.type != TokenType::END_OF_FILE)
        {
            unique_ptr<statement> stmt = parsestatement();
            if (stmt != nullptr)
            {
                block->statements.push_back(move(stmt));
            }
            else
            {
                break;
            }
        }
        // After Exiting loop two cases can be there why loop was exited
        if (currenttoken.type == TokenType::RIGHT_BRACE)
        {
            advance();
            return block;
        }
        else
        {
            cout << "syntax error : right brace missing on the line  " << currenttoken.line << endl;
            return nullptr;
        }
    }
    unique_ptr<statement> parsestatement()
    {
        // func that returns unique ptr to statements
        // Syntax for let
        // [LET] [IDENTIFIER] [EQUALS] [EXPRESSION] [SEMICOLON]
        if (currenttoken.type == TokenType::LET)
        {
            int line = currenttoken.line;
            advance();
            if (currenttoken.type == TokenType::IDENTIFIER)
            {
                string varname = currenttoken.Lexed;
                advance();
                if (currenttoken.type == TokenType::EQUALS)
                {
                    advance();
                    unique_ptr<expression> maths = parsecomparison();
                    if (currenttoken.type == TokenType::SEMICOLON)
                    {
                        advance();
                        return make_unique<Letnode>(varname, move(maths), line);
                    }
                    else
                    {
                        cout << "Syntax Error on line " << currenttoken.line << "; missing" << endl;
                        return nullptr;
                    }
                }
                else
                {
                    cout << "Syntax Error on line " << currenttoken.line << ": Unexpected token '" << currenttoken.Lexed << "'" << endl;
                    return nullptr;
                }
            }
            else
            {
                cout << "Syntax Error on line " << currenttoken.line << ": Unexpected token '" << currenttoken.Lexed << "'" << endl;
                return nullptr;
            }
        }
        else if (currenttoken.type == TokenType::PRINT)
        {
            // syntax for print
            //// [PRINT] [EXPRESSION] [SEMICOLON]
            int l = currenttoken.line;
            advance();
            unique_ptr<expression> value = parsecomparison();
            if (currenttoken.type == TokenType::SEMICOLON)
            {
                advance();
                return make_unique<PrintNode>(move(value), l);
            }
            else
            {
                cout << "Syntax Error: Missing ';' after print on line " << currenttoken.line << endl;
                return nullptr;
            }
        }
        else if (currenttoken.type == TokenType::IF)
        {
            return parseif();
        }
        else if (currenttoken.type == TokenType::WHILE)
        {
            return parseWhile();
        }
        else if (currenttoken.type == TokenType::IDENTIFIER)
        {
            return parseAssignment();
        }
        else
        {
            cout << "Syntax Error on line " << currenttoken.line
                 << ": Unexpected token '" << currenttoken.Lexed << "'" << endl;
            return nullptr;
        }
    }
    unique_ptr<IfNode> parseif()
    {
        int l = currenttoken.line;
        advance();
        if (currenttoken.type == TokenType::LEFT_PAREN)
        {
            advance();
            unique_ptr<expression> cond = parsecomparison();
            if (currenttoken.type == TokenType::RIGHT_PAREN)
            {
                advance();
                unique_ptr<BlockNode> ifblock = parseblock();
                if (ifblock == nullptr)
                    return nullptr;
                unique_ptr<BlockNode> elseblock = nullptr;
                if (currenttoken.type == TokenType::ELSE)
                {
                    advance();
                    elseblock = parseblock();
                    return make_unique<IfNode>(move(cond), move(ifblock), move(elseblock), l);
                }
                else
                {
                    return make_unique<IfNode>(move(cond), move(ifblock), move(elseblock), l);
                }
            }
            else
            {
                cout << "syntax error on the line" << l << " right paren missing" << endl;
                return nullptr;
            }
        }
        else
        {
            cout << "syntax error on the line" << l << " left paren missing" << endl;
            return nullptr;
        }
    }
    unique_ptr<statement> parseWhile()
    {
        int l = currenttoken.line;
        advance();
        if (currenttoken.type == TokenType::LEFT_PAREN)
        {
            advance();
            unique_ptr<expression> cond = parsecomparison();
            if (currenttoken.type == TokenType::RIGHT_PAREN)
            {
                advance();
                unique_ptr<BlockNode> loopBlock = parseblock();
                if (loopBlock == nullptr)
                {
                    return nullptr;
                }
                return make_unique<WhileNode>(move(cond), move(loopBlock), l);
            }
            else
            {
                cout << "Syntax error on line " << currenttoken.line << ": right paren missing" << endl;
                return nullptr;
            }
        }
        else
        {
            cout << "Syntax error on line " << currenttoken.line << ": left paren missing" << endl;
            return nullptr;
        }
    }
    unique_ptr<statement> parseAssignment()
    {
        int l = currenttoken.line;
        string varname = currenttoken.Lexed;
        advance();
        if (currenttoken.type == TokenType::EQUALS)
        {
            advance();
            unique_ptr<expression> newValue = parsecomparison();
            if (currenttoken.type == TokenType::SEMICOLON)
            {
                advance();
                return make_unique<AssignmentNode>(varname, move(newValue), l);
            }
            else
            {
                cout << "Syntax Error on line " << currenttoken.line << ": Missing ';' after assignment" << endl;
                return nullptr;
            }
        }
        else
        {
            cout << "Syntax Error on line " << currenttoken.line << ": Expected '=' after identifier" << endl;
            return nullptr;
        }
    }
    unique_ptr<ProgramNode> parseprogram()
    {
        unique_ptr<ProgramNode> program = make_unique<ProgramNode>();
        while (currenttoken.type != TokenType::END_OF_FILE)
        {
            unique_ptr<statement> stmt = parsestatement();
            if (stmt != nullptr)
            {
                program->statements.push_back(move(stmt));
            }
            else
            {
                cout << "Fatal Error: Halting the compiler." << endl;
                break;
            }
        }
        return program;
    }
};
