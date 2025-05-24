#include <iostream>
#include <vector>
#include <stack>
#include <unordered_map>
#include <map>
#include <string>
#include <sstream>
#include <variant>
#include <cwctype>
#include <locale>
#include <algorithm>
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

// Interpreter class
class Interpreter {
private:
  Stack stack;
  FunctionDict functions;
  std::unordered_map<String, BuiltInFunc> builtIns;

  // Check if a string is a number (double)
  bool isNumber(const String& token) {
    try {
      std::stod(token);
      return true;
    }
    catch (...) {
      return false;
    }
  }

  // Check if a string is a single wide character (e.g., 'a')
  bool isWChar(const String& token) {
    return token.length() == 1 && !isNumber(token);
  }

  // Check if a string is a string literal (starts and ends with quotes)
  bool isStringLiteral(const String& token) {
    return token.length() >= 2 && token.front() == L'"' && token.back() == L'"';
  }

  // Check if a string is an array literal (starts with [ and ends with ])
  bool isArrayLiteral(const String& token) {
    return token.length() >= 2 && token.front() == L'[' && token.back() == L']';
  }

  // Check if a token is a valid function name (letters, digits, underscore, Unicode)
  bool isFunctionName(const String& token) {
    if (token.empty() || token == L":end" || token == L":dump") return false;
    return std::all_of(token.begin(), token.end(), [](wchar_t c) {
      return std::iswalnum(c) || c == L'_' || c > 127;
      });
  }

  // Parse a string into comma-separated tokens for array literals
  std::vector<String> parseArrayTokens(const String& input) {
    std::vector<String> tokens;
    String current;
    int bracketDepth = 0;
    bool inQuotes = false;

    for (size_t i = 1; i < input.length() - 1; ++i) { // Skip [ and ]
      wchar_t c = input[i];
      if (c == L'"' && (i == 0 || input[i - 1] != L'\\')) {
        inQuotes = !inQuotes;
        current += c;
        continue;
      }
      if (inQuotes) {
        current += c;
        continue;
      }
      if (c == L'[') {
        bracketDepth++;
        current += c;
        continue;
      }
      if (c == L']') {
        bracketDepth--;
        current += c;
        continue;
      }
      if (c == L',' && bracketDepth == 0) {
        // Trim whitespace and add non-empty token
        current.erase(0, current.find_first_not_of(L" \t"));
        current.erase(current.find_last_not_of(L" \t") + 1);
        if (!current.empty()) {
          tokens.push_back(current);
        }
        current.clear();
        continue;
      }
      current += c;
    }
    // Add final token
    current.erase(0, current.find_first_not_of(L" \t"));
    current.erase(current.find_last_not_of(L" \t") + 1);
    if (!current.empty()) {
      tokens.push_back(current);
    }
    return tokens;
  }

  // Parse a token into an Element
  Element parseElement(const String& token) {
    if (isNumber(token)) {
      return std::stod(token);
    }
    else if (isStringLiteral(token)) {
      return token.substr(1, token.length() - 2); // Remove quotes
    }
    else if (isWChar(token)) {
      return token[0];
    }
    else if (isArrayLiteral(token)) {
      return parseArray(token); // Handle array as Element
    }
    return token; // Treat as string (potential function name)
  }

  // Parse an array literal into an Array
  Array parseArray(const String& token) {
    Array result;
    if (!isArrayLiteral(token)) {
      result.push_back(parseElement(token));
      return result;
    }

    std::vector<String> elements = parseArrayTokens(token);
    for (const auto& elem : elements) {
      result.push_back(parseElement(elem)); // Recursive for nested arrays
    }
    return result;
  }

  // Print a single array
  void printArray(const Array& arr) {
    std::wcout << L"[ ";
    for (const auto& elem : arr) {
      std::visit([&](const auto& value) {
        if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Array>) {
          printArray(value); // Recursive for nested arrays
        }
        else {
          std::wcout << value << L" ";
        }
        }, elem);
    }
    std::wcout << L"]";
  }

  // Apply binary operation with scalar extension
  void applyBinaryOp(Stack& s, const String& opName, std::function<double(double, double)> op) {
    if (s.size() < 2) {
      std::wcerr << L"Error: Insufficient stack elements for " << opName << std::endl;
      return;
    }
    Array b = s.top(); s.pop();
    Array a = s.top(); s.pop();
    Array result;

    // Check if one operand is a scalar (single-element array with double) and the other is an array
    bool aIsScalar = (a.size() == 1 && std::holds_alternative<double>(a[0]));
    bool bIsScalar = (b.size() == 1 && std::holds_alternative<double>(b[0]));

    if (aIsScalar && !bIsScalar) {
      // Scalar op Array: replicate scalar
      double scalar = std::get<double>(a[0]);
      for (const auto& elem : b) {
        if (!std::holds_alternative<double>(elem)) {
          std::wcerr << L"Error: " << opName << L" requires numeric arguments" << std::endl;
          return;
        }
        double val = std::get<double>(elem);
        if (opName == L"/" && val == 0.0) {
          std::wcerr << L"Error: Division by zero" << std::endl;
          return;
        }
        result.push_back(op(scalar, val));
      }
    }
    else if (bIsScalar && !aIsScalar) {
      // Array op Scalar: replicate scalar
      double scalar = std::get<double>(b[0]);
      if (opName == L"/" && scalar == 0.0) {
        std::wcerr << L"Error: Division by zero" << std::endl;
        return;
      }
      for (const auto& elem : a) {
        if (!std::holds_alternative<double>(elem)) {
          std::wcerr << L"Error: " << opName << L" requires numeric arguments" << std::endl;
          return;
        }
        result.push_back(op(std::get<double>(elem), scalar));
      }
    }
    else if (a.size() == 1 && b.size() == 1 && std::holds_alternative<double>(a[0]) && std::holds_alternative<double>(b[0])) {
      // Scalar op Scalar
      double val_b = std::get<double>(b[0]);
      if (opName == L"/" && val_b == 0.0) {
        std::wcerr << L"Error: Division by zero" << std::endl;
        return;
      }
      result.push_back(op(std::get<double>(a[0]), std::get<double>(b[0])));
    }
    else {
      // Array op Array (must be same length)
      if (a.size() != b.size()) {
        std::wcerr << L"Error: " << opName << L" requires arrays of equal length or one scalar" << std::endl;
        return;
      }
      for (size_t i = 0; i < a.size(); ++i) {
        if (!std::holds_alternative<double>(a[i]) || !std::holds_alternative<double>(b[i])) {
          std::wcerr << L"Error: " << opName << L" requires numeric arguments" << std::endl;
          return;
        }
        double val_b = std::get<double>(b[i]);
        if (opName == L"/" && val_b == 0.0) {
          std::wcerr << L"Error: Division by zero" << std::endl;
          return;
        }
        result.push_back(op(std::get<double>(a[i]), std::get<double>(b[i])));
      }
    }
    s.push(result);
  }

  // Initialize built-in functions
  void initBuiltIns() {
    builtIns[L"+"] = [this](Stack& s) {
      applyBinaryOp(s, L"+", [](double x, double y) { return x + y; });
      };

    builtIns[L"-"] = [this](Stack& s) {
      applyBinaryOp(s, L"-", [](double x, double y) { return x - y; });
      };

    builtIns[L"*"] = [this](Stack& s) {
      applyBinaryOp(s, L"*", [](double x, double y) { return x * y; });
      };

    builtIns[L"/"] = [this](Stack& s) {
      applyBinaryOp(s, L"/", [](double x, double y) { return x / y; });
      };

    builtIns[L"^"] = [this](Stack& s) {
      applyBinaryOp(s, L"^", [](double x, double y) { return std::pow(x, y); });
      };

    builtIns[L"cat"] = [this](Stack& s) {
      if (s.size() < 2) {
        std::wcerr << L"Error: Insufficient stack elements for cat" << std::endl;
        return;
      }
      Array b = s.top(); s.pop();
      Array a = s.top(); s.pop();
      Array result = a;
      result.insert(result.end(), b.begin(), b.end());
      s.push(result);
      };

    builtIns[L"."] = [this](Stack& s) {
      if (s.empty()) {
        std::wcerr << L"Error: Stack empty for ." << std::endl;
        return;
      }
      Array top = s.top(); s.pop();
      printArray(top);
      std::wcout << std::endl;
      };

    builtIns[L"clear"] = [this](Stack& s) {
      while (!s.empty()) {
        s.pop();
      }
      };

    builtIns[L"swap"] = [this](Stack& s) {
      if (s.size() < 2) {
        std::wcerr << L"Error: Insufficient stack elements for swap" << std::endl;
        return;
      }
      Array top = s.top(); s.pop();
      Array second = s.top(); s.pop();
      s.push(top);
      s.push(second);
      };

    builtIns[L"dup"] = [this](Stack& s) {
      if (s.empty()) {
        std::wcerr << L"Error: Stack empty for dup" << std::endl;
        return;
      }
      Array top = s.top();
      s.push(top);
      };

    builtIns[L"range"] = [this](Stack& s) {
      if (s.empty()) {
        std::wcerr << L"Error: Stack empty for range" << std::endl;
        return;
      }
      Array top = s.top(); s.pop();
      if (top.size() != 1 || !std::holds_alternative<double>(top[0])) {
        std::wcerr << L"Error: range requires a scalar numeric argument" << std::endl;
        return;
      }
      double val = std::get<double>(top[0]);
      if (val < 0 || std::floor(val) != val) {
        std::wcerr << L"Error: range requires a non-negative integer" << std::endl;
        return;
      }
      Array result;
      for (int i = 0; i < static_cast<int>(val); ++i) {
        result.push_back(static_cast<double>(i));
      }
      s.push(result);
      };
  }

  // Evaluate a sequence of tokens
  void evaluate(const std::vector<String>& tokens, bool isFunctionBody = false) {
    bool defining = false;
    String funcName;
    std::vector<String> funcBody;

    for (size_t i = 0; i < tokens.size(); ++i) {
      const String& token = tokens[i];

      if (token == L":dump") {
        Stack temp;
        std::wcout << L"Stack:" << std::endl;
        if (stack.empty()) {
          std::wcout << L"(empty)" << std::endl;
        }
        else {
          while (!stack.empty()) {
            temp.push(stack.top());
            printArray(stack.top());
            std::wcout << std::endl;
            stack.pop();
          }
          // Restore stack
          while (!temp.empty()) {
            stack.push(temp.top());
            temp.pop();
          }
        }
        continue;
      }

      if (!isFunctionBody && !defining && token.size() > 1 && (token[0] == L':')) {
        funcName = token.substr(1);

        if (i + 1 < tokens.size() && isFunctionName(funcName)) {
          defining = true;
          continue;
        }
        else {
          std::wcerr << L"Error: Invalid function definition" << std::endl;
          continue;
        }
      }

      if (defining) {
        if (token == L":end") {
          functions[funcName] = funcBody;
          defining = false;
          funcBody.clear();
          continue;
        }
        funcBody.push_back(token);
        continue;
      }

      // Check if token is a function call
      if (functions.find(token) != functions.end()) {
        evaluate(functions[token], true);
        continue;
      }

      // Check for built-in operations
      if (builtIns.find(token) != builtIns.end()) {
        builtIns[token](stack);
        continue;
      }

      // Push parsed token (array or single element) onto stack
      stack.push(parseArray(token));
    }
  }

  // Tokenize input, handling array and string literals
  std::vector<String> tokenize(const String& input) {
    std::vector<String> tokens;
    String current;
    bool inQuotes = false;
    int bracketDepth = 0;

    for (size_t i = 0; i < input.length(); ++i) {
      wchar_t c = input[i];

      if (c == L'"' && (i == 0 || input[i - 1] != L'\\')) {
        inQuotes = !inQuotes;
        current += c;
        continue;
      }
      if (inQuotes) {
        current += c;
        continue;
      }
      if (c == L'[') {
        bracketDepth++;
        current += c;
        continue;
      }
      if (c == L']') {
        bracketDepth--;
        current += c;
        if (bracketDepth == 0 && !current.empty()) {
          tokens.push_back(current);
          current.clear();
        }
        continue;
      }
      if (std::iswspace(c) && bracketDepth == 0 && !inQuotes) {
        if (!current.empty()) {
          tokens.push_back(current);
          current.clear();
        }
        continue;
      }
      current += c;
    }
    // Add final token
    if (!current.empty()) {
      tokens.push_back(current);
    }
    return tokens;
  }

public:
  Interpreter() {
    initBuiltIns();
  }

  // Process a line of input
  void process(const String& input) {
    std::vector<String> tokens = tokenize(input);
    evaluate(tokens);
  }
};

int main() {
  // Attempt to set locale for Unicode support
  try {
    std::locale::global(std::locale("en_US.UTF-8"));
    std::wcout.imbue(std::locale());
    std::wcin.imbue(std::locale());
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Warning: Could not set locale en_US.UTF-8: " << e.what() << "\n";
    std::cerr << "Falling back to default locale.\n";
    // Use default locale
    std::locale::global(std::locale(""));
    std::wcout.imbue(std::locale());
    std::wcin.imbue(std::locale());
  }

  Interpreter interp;
  String line;

  std::wcout << L"Simple Concatenative Language Interpreter\n";
  std::wcout << L"Type 'exit' to quit\n";

  while (true) {
    std::wcout << L"> ";
    std::getline(std::wcin, line);
    if (line == L"exit") break;
    interp.process(line);
  }

  return 0;
}
