#pragma once
#include <iostream>
#include <vector>
#include <cstdint> //for uint8_t
using namespace std;
enum class OpCode : uint8_t
{
    OP_CONSTANT,      // Load a number from the constant pool
    OP_ADD,           // +
    OP_SUBTRACT,      // -
    OP_MULTIPLY,      // *
    OP_DIVIDE,        // /
    OP_EQUAL,         // ==
    OP_GREATER,       // >
    OP_LESS,          // <
    OP_NOT,           // inverts top of the stack
    OP_DEFINE_GLOBAL, // creates variable
    OP_GET_GLOBAL,    // gets the value of the variable
    OP_SET_GLOBAL,    // sets new value of already defined variable
    OP_INPUT,         // for taking input from keyboard
    OP_PRINT,         // for print
    OP_JUMP_IF_FALSE, // if top of stack is 0, skip
    OP_JUMP,          // always skip
    OP_LOOP,          // jump BACKWARD (used to repeat while)
    OP_RETURN         // End of the program
};
class Chunk
{
public:
    vector<uint8_t> code;
    vector<int> constants;
    vector<string> variablenames;
    void writechunk(uint8_t byte)
    {
        code.push_back(byte);
    }
    int addconstant(int value)
    {
        constants.push_back(value);  // pushes the value in constant pool
        return constants.size() - 1; // returns index of the int
    }
    int addvariablename(string name)
    {
        for (int i = 0; i < variablenames.size(); i++)
        {
            // for  reusing the variable if already present than no need to push in the variable names array
            if (variablenames[i] == name)
            {
                return i;
            }
        }
        variablenames.push_back(name);
        return variablenames.size() - 1;
    }
};