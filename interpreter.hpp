#pragma once

#include "types.hpp"
#include <iostream>
#include <sstream>
#include <cwctype>
#include <locale>
#include <algorithm>

class Interpreter {
private:
    Stack stack;
    FunctionDict functions;
    std::unordered_map<String, BuiltInFunc> builtIns;

    // Helper functions
    bool isNumber(const String& token);
    bool isWChar(const String& token);
    bool isStringLiteral(const String& token);
    bool isArrayLiteral(const String& token);
    bool isFunctionName(const String& token);
    std::vector<String> parseArrayTokens(const String& input);
    Element parseElement(const String& token);
    Array parseArray(const String& token);
    void printArray(const Array& arr, int indent = 0);
    void getShape(const Array& arr, std::vector<size_t>& shape);
    bool shapesEqual(const std::vector<size_t>& shape1, const std::vector<size_t>& shape2);
    void applyBinaryOp(Stack& s, const String& opName, std::function<double(double, double)> op);
    void initBuiltIns();
    void evaluate(const std::vector<String>& tokens, bool isFunctionBody = false);
    std::vector<String> tokenize(const String& input);

public:
    Interpreter();
    void process(const String& input);
}; 