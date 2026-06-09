#pragma once
#include <iostream>
#include <vector>
#include <cstdint>
// why enum instead of enum class
// more safe
// enum class Color { RED, GREEN, BLUE };
// enum class Alert { RED, YELLOW }; // ✅ Perfectly fine!

// // You must call them by their full name:
// Color myColor = Color::RED;
// Alert myAlert = Alert::RED;
using namespace std;
enum class OpCode:uint8_t{
    OP_CONSTANT, // Load a number from the constant pool
    OP_ADD,      // +
    OP_SUBTRACT, // -
    OP_MULTIPLY, // *
    OP_DIVIDE,   // /
    OP_EQUAL,    // ==
    OP_GREATER,  // >
    OP_LESS,     // <
    OP_NOT,      // ! (Inverts the top of the stack: 1 becomes 0, 0 becomes 1)
    OP_DEFINE_GLOBAL,  // let x = ...  (creates variable)
    OP_GET_GLOBAL,    
    OP_SET_GLOBAL,
    OP_INPUT, 
    OP_PRINT,
    OP_JUMP_IF_FALSE,  // if top of stack is 0, skip forward
    OP_JUMP,           // always skip forward (used to skip else block)
    OP_LOOP,           // jump BACKWARD (used to repeat while)
    OP_RETURN    // End of the program
};
class Chunk{
    public: 
    vector<uint8_t> code;
    vector<int> constants;
    vector<string> variablenames;   
    void writechunk(uint8_t byte){
     code.push_back(byte);
    }
    int addconstant(int value){
        constants.push_back(value);// pushes the value in constant pool
        return constants.size()-1;// returns index of the int 
    }
    int addvariablename(string name){
    for(int i=0;i<variablenames.size();i++){
        if(variablenames[i]==name){
            // reusing the variable
            return i;
                }
    }
    variablenames.push_back(name);
    return variablenames.size()-1;
    }
};