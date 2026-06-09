#pragma once
#include "chunk.hpp"
#include <iostream>
#include <map>
#include <string>
#define stack_max 256
using namespace std;
class VM
{
private:
    Chunk *chunk;
    int ip; // current instruction pointer (index of current byte)
    int stack[stack_max];
    int stacktop; // points to empty slot at top of stack
    map<string, int> globals;

public:
    VM()
    {
        stacktop = 0;
    }
    void push(int value)
    {
        if (stacktop >= stack_max)
        {
            cout << "Runtime Error: Stack Overflow.\n";
            exit(1);
        }
        stack[stacktop] = value;
        stacktop++;
    }
    int pop()
    {
        stacktop--;
        return stack[stacktop];
    }
    void interpret(Chunk *chunkToRun)
    {
        chunk = chunkToRun;
        ip = 0;
        run();
    }
    void run()
    {
        for (;;)
        {
            uint8_t instruction = chunk->code[ip];
            ip++;
            switch ((OpCode)instruction)
            {
            case OpCode::OP_CONSTANT:
            {
                uint8_t constantindex = chunk->code[ip];
                ip++;
                int value = chunk->constants[constantindex];
                push(value);
                break;
            }
            case OpCode::OP_ADD:
            {
                int right = pop();
                int left = pop();
                push(left + right);
                break;
            }
            case OpCode::OP_SUBTRACT:
            {
                int right = pop();
                int left = pop();
                push(left - right);
                break;
            }
            case OpCode::OP_MULTIPLY:
            {
                int right = pop();
                int left = pop();
                push(left * right);
                break;
            }
            case OpCode::OP_DIVIDE:
            {
                int right = pop();
                int left = pop();
                push(left / right);
                break;
            }
            case OpCode::OP_EQUAL:
            {
                int right = pop();
                int left = pop();
                push(left == right ? 1 : 0);
                break;
            }
            case OpCode::OP_GREATER:
            {
                int right = pop();
                int left = pop();
                push(left > right ? 1 : 0);
                break;
            }
            case OpCode::OP_LESS:
            {
                int right = pop();
                int left = pop();
                push(left < right ? 1 : 0);
                break;
            }
            case OpCode::OP_NOT:
            {
                int value = pop();
                push(value == 0 ? 1 : 0);
                break;
            }
            case OpCode::OP_DEFINE_GLOBAL:
            {
                uint8_t index = chunk->code[ip++];
                string name = chunk->variablenames[index];
                globals[name] = pop();
                break;
            }
            case OpCode::OP_GET_GLOBAL:
            {
                uint8_t index = chunk->code[ip++];
                string name = chunk->variablenames[index];
                if (globals.find(name) == globals.end())
                {
                    cout << "Runtime Error: Undefined variable '" << name << "'" << endl;
                    return;
                }
                push(globals[name]);
                break;
            }
            case OpCode::OP_SET_GLOBAL:
            {
                uint8_t index = chunk->code[ip++];
                string name = chunk->variablenames[index];
                if (globals.find(name) == globals.end())
                {
                    cout << "Runtime Error: Cannot assign to undefined variable '" << name << "'" << endl;
                    return;
                }
                globals[name] = pop();
                break;
            }
            case OpCode::OP_PRINT:
            {
                int val = pop();
                cout << val << endl;
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE:
            {
                uint8_t hi = chunk->code[ip++];
                uint8_t lo = chunk->code[ip++];
                int offset = (hi << 8) | lo;
                int condition = pop();
                if (condition == 0)
                    ip += offset; // skip the body if false
                break;
            }
            case OpCode::OP_JUMP:
            {
                uint8_t hi = chunk->code[ip++];
                uint8_t lo = chunk->code[ip++];
                int offset = (hi << 8) | lo;
                ip += offset; // always skip
                break;
            }
            case OpCode::OP_LOOP:
            {
                uint8_t hi = chunk->code[ip++];
                uint8_t lo = chunk->code[ip++];
                int offset = (hi << 8) | lo;
                ip -= offset; // go BACKWARD
                break;
            }
            case OpCode::OP_INPUT:
            {
                int value;
                cout << "> "; //  for asking from user for input
                cin >> value;
                push(value);
                break;
            }
            case OpCode::OP_RETURN:
            {
                return;
            }
            }
        }
    }
};