#pragma once

#include <vector>
#include <stack>
#include <unordered_map>
#include <map>
#include <string>
#include <variant>
#include <functional>

// Forward declaration of Array
struct Array;

// Data type for array elements: wchar_t, double, string (wstring), or Array
using String = std::wstring;
using Element = std::variant<wchar_t, double, String, Array>;

// Array type: a vector of elements
struct Array : std::vector<Element> {};

// Stack of arrays
using Stack = std::stack<Array>;

// Function dictionary: maps function name to its body (sequence of tokens)
using FunctionDict = std::map<String, std::vector<String>>;

// Built-in function type
using BuiltInFunc = std::function<void(Stack&)>; 