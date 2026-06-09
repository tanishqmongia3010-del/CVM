#pragma once
#include "chunk.hpp"
#include "lexer.hpp"
#include <string>
#include <memory>
#include <vector>
//               ASTnode (Master: Holds the line number)
//                         /                  \
//             expression                     statement
// (Things with values)                     (Things that do actions)
//       /         |         \                        \
// numbernode  identifierNode  operatornode           (We will add LetNode and PrintNode here later!)
using namespace std;
class ASTnode
{
public:
    int line;// "line" is a global variable 
    ASTnode() {} // empty constructor
    virtual ~ASTnode() = default;
    // The '= 0' means this is a "Pure Virtual Function".
    // It forces every child node to create their own version of this function!
    virtual void print() = 0;
    virtual void compile(Chunk *chunk) = 0;
    // Since you already have virtual void compile(Chunk* chunk) = 0; inside your ASTnodE IT FORCES EVERY CHILDS= TO HAVE IT
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
// now lets make leaves and branches expression can be of diff types
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
    // An object must know its exact byte-size when you compile.
    // If you try to put an Expression inside another Expression,
    // the compiler will panic because it doesn't know how deep the tree goes.
    // so we will use pointer here
    // UNIQUE PTR BECAUSE WE WANT IT TO AUTOMATICALLY DELTE WE HAVE A PREDEFINED FUNC FOR IT
    unique_ptr<expression> left;
    unique_ptr<expression> right;
    operatornode(unique_ptr<expression> leftnode, string operat, unique_ptr<expression> rightnode, int l) : expression()
    {
        left = move(leftnode); // UNIQUE PTR CANT BE COPIED SO YOU NEED TO SNATCH THE MEMORY FROM ONE PTR AND MOVE IT TO OTHER
        right = move(rightnode);
        op = operat;
        line = l;
    }
    void print() override
    {
        cout << "(";
        left->print(); // Tell the left branch to print!
        cout << " " << op << " ";
        right->print(); // Tell the right branch to print!
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
// for let statement
class Letnode : public statement
{
public:
//let x=2+3
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
        value->print(); // Tell the math equation to print itself
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
// print x 
//print 3+5
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
    // yeh debug wala print hai above 
    void compile(Chunk *chunk) override
    {
        value->compile(chunk);
        chunk->writechunk((uint8_t)OpCode::OP_PRINT);
    }
};
class ProgramNode : public ASTnode
{
    // let x=3;// statement 
    // print (5+3);// statement
    // 5+3 
    // if (4==5){
    //      print 4; 
    // } // statement 
    // while (x<5){
    //      print x;
    //      x=x+1;
    //  }// statements 
public:
    vector<unique_ptr<statement>> statements;
    ProgramNode()
    {
        line = 1;   //initialized as 1 or anything
    }
    void print() override
    {
        cout << "--- PROGRAM START ---\n";
        for (auto &stmt : statements)
        {
            stmt->print();
            cout << "\n";
        }
        cout << "--- PROGRAM END ---\n";
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
    /*{ statement 1 iska pointer 
    statement 2 
    statement 3 
    statemnet 4} */
public:
    vector<unique_ptr<statement>> statements;
    BlockNode(int l)
    {
        line = l;
    }
    void print() override
    {
        cout << "{\n";
        // Loop through and print every statement inside the block
        for (auto &stmt : statements)
        {
            cout << "  "; // Add some indenting so it looks pretty
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
 // while(condition){block } => condition: expression ; block: blocknode 
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

    // WhileNode::compile
    void compile(Chunk *chunk) override
    {
        // 1. save position BEFORE condition — we loop back here
        int loopStart = chunk->code.size();
        // 2. compile condition
        condition->compile(chunk);
        // 3. jump out of loop if false ke liye func likh rha
        chunk->writechunk((uint8_t)OpCode::OP_JUMP_IF_FALSE);
        chunk->writechunk(0xff);
        chunk->writechunk(0xff);
        int jumpIndex = chunk->code.size() - 2;
        // 4. compile loop body
        loop->compile(chunk);
        // 5. emit OP_LOOP — jumps backward to loopStart
        chunk->writechunk((uint8_t)OpCode::OP_LOOP);
        chunk->writechunk(0xff);
        chunk->writechunk(0xff);
        int offset = chunk->code.size() - loopStart;
        chunk->code[chunk->code.size() - 2] = (offset >> 8) & 0xff;
        chunk->code[chunk->code.size() - 1] = offset & 0xff;

        // 6. patch jump-if-false to here (after the loop)
        int exitOffset = chunk->code.size() - jumpIndex - 2;
        chunk->code[jumpIndex] = (exitOffset >> 8) & 0xff;
        chunk->code[jumpIndex + 1] = exitOffset & 0xff;
    }
};
class IfNode : public statement
{
    // if(conditon) {block} else {block}
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

        // 1. push the condition result onto stack
        conditon->compile(chunk);

        // 2. emit jump-if-false with fake offset
        chunk->writechunk((uint8_t)OpCode::OP_JUMP_IF_FALSE);
        chunk->writechunk(0xff);
        chunk->writechunk(0xff);
        int jumpIfFalseIndex = chunk->code.size() - 2; // remember where

        // 3. compile if-body
        ifblock->compile(chunk);

        if (elseblock != nullptr)
        {
            // 4. emit unconditional jump to skip else, fake offset
            chunk->writechunk((uint8_t)OpCode::OP_JUMP);
            chunk->writechunk(0xff);
            chunk->writechunk(0xff);
            int jumpIndex = chunk->code.size() - 2;

            // 5. NOW patch jump-if-false — it should land at start of else
            int offset1 = chunk->code.size() - jumpIfFalseIndex - 2;
            chunk->code[jumpIfFalseIndex] = (offset1 >> 8) & 0xff;
            chunk->code[jumpIfFalseIndex + 1] = offset1 & 0xff;

            // 6. compile else-body
            elseblock->compile(chunk);

            // 7. patch the unconditional jump — lands after else
            int offset2 = chunk->code.size() - jumpIndex - 2;
            chunk->code[jumpIndex] = (offset2 >> 8) & 0xff;
            chunk->code[jumpIndex + 1] = offset2 & 0xff;
        }
        else
        {
            // no else — just patch jump-if-false to here
            int offset = chunk->code.size() - jumpIfFalseIndex - 2;
            chunk->code[jumpIfFalseIndex] = (offset >> 8) & 0xff;
            chunk->code[jumpIfFalseIndex + 1] = offset & 0xff;
        }
    }
};
class AssignmentNode : public statement
{
    // Value updation node 
    // x = 6, x=3+4
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
    // Syntax assumed here:  let x=input
    // input x 
    // expression node because it produces expression 
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