#pragma once
#include "chunk.hpp"
#include "lexer.hpp"
#include <string>
#include <memory>
#include <vector>
using namespace std;
class ASTnode
{
public:
    int line;
    ASTnode() {}
    virtual ~ASTnode() = default;
    virtual void print() = 0;
    virtual void compile(Chunk *chunk) = 0;
};
class expression : public ASTnode
{
public:
    expression() : ASTnode() {};
};
class statement : public ASTnode
{
public:
    statement() : ASTnode() {};
};
class numbernode : public expression
{
public:
    string value;
    numbernode(string val, int l)
    {
        value = val;
        line = l;
    }
    void print() override
    {
        cout << value;
    }
    void compile(Chunk *chunk) override
    {
        int intvalue = stoi(value);
        int index = chunk->addconstant(intvalue);
        chunk->writechunk((uint8_t)OpCode::OP_CONSTANT);
        chunk->writechunk((uint8_t)index);
    }
};
class identifierNode : public expression
{
public:
    string name;
    identifierNode(string n, int l)
    {
        name = n;
        line = l;
    }
    void print() override
    {
        cout << name;
    }
    void compile(Chunk *chunk) override
    {
        int index = chunk->addvariablename(name);
        chunk->writechunk((uint8_t)OpCode::OP_GET_GLOBAL);
        chunk->writechunk((uint8_t)index);
    }
};
class operatornode : public expression
{
public:
    string op;
    unique_ptr<expression> left;
    unique_ptr<expression> right;
    operatornode(unique_ptr<expression> leftnode, string operat, unique_ptr<expression> rightnode, int l) : expression()
    {
        left = move(leftnode);
        right = move(rightnode);
        op = operat;
        line = l;
    }
    void print() override
    {
        cout << "(";
        left->print();
        cout << " " << op << " ";
        right->print();
        cout << ")";
    }
    void compile(Chunk *chunk) override
    {
        left->compile(chunk);
        right->compile(chunk);
        if (op == "+")
            chunk->writechunk((uint8_t)OpCode::OP_ADD);
        else if (op == "-")
            chunk->writechunk((uint8_t)OpCode::OP_SUBTRACT);
        else if (op == "*")
            chunk->writechunk((uint8_t)OpCode::OP_MULTIPLY);
        else if (op == "/")
            chunk->writechunk((uint8_t)OpCode::OP_DIVIDE);
        else if (op == "==")
            chunk->writechunk((uint8_t)OpCode::OP_EQUAL);
        else if (op == ">")
            chunk->writechunk((uint8_t)OpCode::OP_GREATER);
        else if (op == "<")
            chunk->writechunk((uint8_t)OpCode::OP_LESS);
        else if (op == ">=")
        {
            chunk->writechunk((uint8_t)OpCode::OP_LESS);
            chunk->writechunk((uint8_t)OpCode::OP_NOT);
        }
        else if (op == "<=")
        {
            chunk->writechunk((uint8_t)OpCode::OP_GREATER);
            chunk->writechunk((uint8_t)OpCode::OP_NOT);
        }
    }
};
class Letnode : public statement
{
public:
    string name;
    unique_ptr<expression> value;
    Letnode(string s, unique_ptr<expression> val, int l)
    {
        name = s;
        value = move(val);
        line = l;
    }
    void print() override
    {
        cout << "(LET " << name << " = ";
        value->print();
        cout << ")";
    }
    void compile(Chunk *chunk) override
    {
        value->compile(chunk);
        int index = chunk->addvariablename(name);
        chunk->writechunk((uint8_t)OpCode::OP_DEFINE_GLOBAL);
        chunk->writechunk((uint8_t)index);
    }
};
class PrintNode : public statement
{
public:
    unique_ptr<expression> value;
    PrintNode(unique_ptr<expression> val, int l)
    {
        value = move(val);
        line = l;
    }
    void print() override
    {
        cout << "(PRINT ";
        value->print();
        cout << ")";
    }
    void compile(Chunk *chunk) override
    {
        value->compile(chunk);
        chunk->writechunk((uint8_t)OpCode::OP_PRINT);
    }
};
class ProgramNode : public ASTnode
{
public:
    vector<unique_ptr<statement>> statements;
    ProgramNode()
    {
        line = 1;
    }
    void print() override
    {
        cout << "START\n";
        for (auto &stmt : statements)
        {
            stmt->print();
            cout << "\n";
        }
        cout << "END\n";
    }
    void compile(Chunk *chunk) override
    {
        for (auto &stmt : statements)
        {
            stmt->compile(chunk);
        }
        chunk->writechunk((uint8_t)OpCode::OP_RETURN);
    }
};
class BlockNode : public statement
{
public:
    vector<unique_ptr<statement>> statements;
    BlockNode(int l)
    {
        line = l;
    }
    void print() override
    {
        cout << "{\n";
        for (auto &stmt : statements)
        {
            cout << "  ";
            stmt->print();
            cout << "\n";
        }
        cout << "}";
    }
    void compile(Chunk *chunk) override
    {
        for (auto &stmt : statements)
        {
            stmt->compile(chunk);
        }
    }
};
class WhileNode : public statement
{
public:
    unique_ptr<expression> condition; // holds the condition
    unique_ptr<BlockNode> loop;       // holds the recursive statements to be looped
    WhileNode(unique_ptr<expression> cond, unique_ptr<BlockNode> loopblock, int l)
    {
        condition = move(cond);
        loop = move(loopblock);
        line = l;
    }
    void print() override
    {
        cout << "(WHILE ";
        condition->print();
        cout << "\nDO\n";
        loop->print();
        cout << ")";
    }
    void compile(Chunk *chunk) override
    {
        int loopStart = chunk->code.size();
        condition->compile(chunk);
        chunk->writechunk((uint8_t)OpCode::OP_JUMP_IF_FALSE);
        chunk->writechunk(0xff);
        chunk->writechunk(0xff);
        int jumpIndex = chunk->code.size() - 2;
        loop->compile(chunk);
        chunk->writechunk((uint8_t)OpCode::OP_LOOP);
        chunk->writechunk(0xff);
        chunk->writechunk(0xff);
        int offset = chunk->code.size() - loopStart;
        chunk->code[chunk->code.size() - 2] = (offset >> 8) & 0xff;
        chunk->code[chunk->code.size() - 1] = offset & 0xff;
        int exitOffset = chunk->code.size() - jumpIndex - 2;
        chunk->code[jumpIndex] = (exitOffset >> 8) & 0xff;
        chunk->code[jumpIndex + 1] = exitOffset & 0xff;
    }
};
class IfNode : public statement
{
public:
    unique_ptr<expression> conditon;
    unique_ptr<BlockNode> ifblock;
    unique_ptr<BlockNode> elseblock;
    IfNode(unique_ptr<expression> cond, unique_ptr<BlockNode> ifB, unique_ptr<BlockNode> elseB, int l)
    {
        conditon = move(cond);
        ifblock = move(ifB);
        elseblock = move(elseB);
        line = l;
    }
    void print() override
    {
        cout << "(IF ";
        conditon->print();
        cout << "\nTHEN\n";
        ifblock->print();
        if (elseblock != nullptr)
        {
            cout << "\nELSE\n";
            elseblock->print();
        }
        cout << ")";
    }
    void compile(Chunk *chunk) override
    {
        conditon->compile(chunk);
        chunk->writechunk((uint8_t)OpCode::OP_JUMP_IF_FALSE);
        chunk->writechunk(0xff);
        chunk->writechunk(0xff);
        int jumpIfFalseIndex = chunk->code.size() - 2;
        ifblock->compile(chunk);
        if (elseblock != nullptr)
        {
            chunk->writechunk((uint8_t)OpCode::OP_JUMP);
            chunk->writechunk(0xff);
            chunk->writechunk(0xff);
            int jumpIndex = chunk->code.size() - 2;
            int offset1 = chunk->code.size() - jumpIfFalseIndex - 2;
            chunk->code[jumpIfFalseIndex] = (offset1 >> 8) & 0xff;
            chunk->code[jumpIfFalseIndex + 1] = offset1 & 0xff;
            elseblock->compile(chunk);
            int offset2 = chunk->code.size() - jumpIndex - 2;
            chunk->code[jumpIndex] = (offset2 >> 8) & 0xff;
            chunk->code[jumpIndex + 1] = offset2 & 0xff;
        }
        else
        {
            int offset = chunk->code.size() - jumpIfFalseIndex - 2;
            chunk->code[jumpIfFalseIndex] = (offset >> 8) & 0xff;
            chunk->code[jumpIfFalseIndex + 1] = offset & 0xff;
        }
    }
};
class AssignmentNode : public statement
{
public:
    string name;
    unique_ptr<expression> newValue;
    AssignmentNode(string n, unique_ptr<expression> val, int l)
    {
        name = n;
        newValue = std::move(val);
        line = l;
    }
    void print() override
    {
        cout << "(" << name << " = ";
        newValue->print();
        cout << ")";
    }
    void compile(Chunk *chunk) override
    {
        newValue->compile(chunk);
        int index = chunk->addvariablename(name);
        chunk->writechunk((uint8_t)OpCode::OP_SET_GLOBAL);
        chunk->writechunk((uint8_t)index);
    }
};
class InputExpr : public expression
{
public:
    InputExpr(int l) { line = l; }
    void print() override
    {
        cout << "(INPUT)";
    }
    void compile(Chunk *chunk) override
    {
        chunk->writechunk((uint8_t)OpCode::OP_INPUT);
    }
};