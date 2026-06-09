#include <iostream>
#include <fstream>
#include <sstream>
#include "headers/lexer.hpp"
#include "headers/parser.hpp"
#include "headers/vm.hpp"
using namespace std;
string read_file(const string &path)
{
    ifstream file(path);
    if (!file.is_open())
    {
        cout << "Error: Cannot open file '" << path << "'\n";
        exit(1);
    }
    stringstream buffer;
    char temp;
    while (file.get(temp))
        buffer << temp;
    return buffer.str();
}
// for printing tokens
void print_tokens(const string &source)
{
    cout << "\n TOKENS \n";
    Lexer lexer(source);
    Token tok = lexer.nextToken();
    while (tok.type != TokenType::END_OF_FILE)
    {
        cout << "  [line " << tok.line << "] " << tok.Lexed << "\n";
        tok = lexer.nextToken();
    }
    cout << "  [END_OF_FILE]\n";
}
// for printing bytecode
void print_bytecode(Chunk *chunk)
{
    cout << "\nBYTECODE\n";
    cout << "  Constants:  ";
    for (int i = 0; i < (int)chunk->constants.size(); i++)
        cout << "[" << i << "]=" << chunk->constants[i] << " ";
    cout << "\n";
    if (!chunk->variablenames.empty())
    {
        cout << "  Variables:  ";
        for (int i = 0; i < (int)chunk->variablenames.size(); i++)
            cout << "[" << i << "]=" << chunk->variablenames[i] << " ";
        cout << "\n";
    }
    cout << "  Instructions:\n";
    int i = 0;
    while (i < (int)chunk->code.size())
    {
        OpCode op = (OpCode)chunk->code[i];
        cout << "    " << i << ": ";
        switch (op)
        {
        case OpCode::OP_CONSTANT:
            cout << "OP_CONSTANT     index=" << (int)chunk->code[i + 1]
                 << "  (value=" << chunk->constants[chunk->code[i + 1]] << ")";
            i += 2;
            break;
        case OpCode::OP_DEFINE_GLOBAL:
            cout << "OP_DEFINE_GLOBAL  '" << chunk->variablenames[chunk->code[i + 1]] << "'";
            i += 2;
            break;
        case OpCode::OP_GET_GLOBAL:
            cout << "OP_GET_GLOBAL     '" << chunk->variablenames[chunk->code[i + 1]] << "'";
            i += 2;
            break;
        case OpCode::OP_SET_GLOBAL:
            cout << "OP_SET_GLOBAL     '" << chunk->variablenames[chunk->code[i + 1]] << "'";
            i += 2;
            break;
        case OpCode::OP_ADD:
            cout << "OP_ADD";
            i++;
            break;
        case OpCode::OP_SUBTRACT:
            cout << "OP_SUBTRACT";
            i++;
            break;
        case OpCode::OP_MULTIPLY:
            cout << "OP_MULTIPLY";
            i++;
            break;
        case OpCode::OP_DIVIDE:
            cout << "OP_DIVIDE";
            i++;
            break;
        case OpCode::OP_EQUAL:
            cout << "OP_EQUAL";
            i++;
            break;
        case OpCode::OP_GREATER:
            cout << "OP_GREATER";
            i++;
            break;
        case OpCode::OP_LESS:
            cout << "OP_LESS";
            i++;
            break;
        case OpCode::OP_NOT:
            cout << "OP_NOT";
            i++;
            break;
        case OpCode::OP_PRINT:
            cout << "OP_PRINT";
            i++;
            break;
        case OpCode::OP_INPUT:
            cout << "OP_INPUT";
            i++;
            break;
        case OpCode::OP_JUMP_IF_FALSE:
        {
            uint8_t hi = chunk->code[i + 1], lo = chunk->code[i + 2];
            cout << "OP_JUMP_IF_FALSE  offset=" << ((hi << 8) | lo);
            i += 3;
            break;
        }
        case OpCode::OP_JUMP:
        {
            uint8_t hi = chunk->code[i + 1], lo = chunk->code[i + 2];
            cout << "OP_JUMP           offset=" << ((hi << 8) | lo);
            i += 3;
            break;
        }
        case OpCode::OP_LOOP:
        {
            uint8_t hi = chunk->code[i + 1], lo = chunk->code[i + 2];
            cout << "OP_LOOP           offset=" << ((hi << 8) | lo);
            i += 3;
            break;
        }
        case OpCode::OP_RETURN:
            cout << "OP_RETURN";
            i++;
            break;
        default: // a safety check
            cout << "UNKNOWN(" << (int)chunk->code[i] << ")";
            i++;
            break;
        }
        cout << "\n";
    }
}
void run_source(const string &source, bool show_tokens, bool show_AST, bool show_bytecode)
{
    if (show_tokens)
        print_tokens(source);

    Parser parser(source);
    unique_ptr<ProgramNode> program = parser.parseprogram();
    if (!program)
    {
        cout << "Parse failed.\n";
        return;
    }

    if (show_AST)
    {
        cout << "\n--- AST ---\n";
        program->print();
    }
    Chunk chunk;
    program->compile(&chunk);
    if (show_bytecode)
        print_bytecode(&chunk);

    VM vm;
    vm.interpret(&chunk);
}
void run_REPL()
{
    cout << "CVM++ REPL  (type 'exit' to quit)\n";
    cout << "Each line must be a complete statement ending in ;\n\n";
    VM vm; // one VM instance so that globals persist across line 
    // VM made outside the loop so that globals remain otherwise each time new VM created
    string line;
    while (true)
    {
        cout << "cvm> ";
        if (!getline(cin, line))
            break;
        if (line == "exit" || line == "quit")
            break;
        if (line.empty())
            continue;
        //single line in a minimal program
        Parser parser(line);
        unique_ptr<ProgramNode> program = parser.parseprogram();
        if (!program)
            continue;
        Chunk chunk;
        program->compile(&chunk);
        vm.interpret(&chunk);
    }
}
int main(int argc, char **argv)
{
    // No arguments means start the REPL
    if (argc == 1)
    {
        run_REPL();
        return 0;
    }
    // first argument is a file
    bool show_tokens = false;
    bool show_AST = false;
    bool show_bytecode = false;
    for (int i = 2; i < argc; i++)
    {
        string flag = argv[i];
        if (flag == "--tokens" || flag == "--all")
            show_tokens = true;
        if (flag == "--ast" || flag == "--all")
            show_AST = true;
        if (flag == "--bytecode" || flag == "--all")
            show_bytecode = true;
    }
    string source = read_file(argv[1]);
    cout << "Running: " << argv[1] << "\n\n";
    run_source(source, show_tokens, show_AST, show_bytecode);
    return 0;
}
