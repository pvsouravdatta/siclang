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

  // Print a single array with pretty formatting
  void printArray(const Array& arr, int indent = 0) {
    // Print indentation
    std::wcout << std::wstring(indent * 2, L' ');
    std::wcout << L"[";

    if (arr.empty()) {
      std::wcout << L"]";
      return;
    }

    bool isNested = false;
    for (const auto& elem : arr) {
      if (std::holds_alternative<Array>(elem)) {
        isNested = true;
        break;
      }
    }

    if (isNested) {
      // Nested array: print each sub-array on a new line
      std::wcout << L"\n";
      for (size_t i = 0; i < arr.size(); ++i) {
        std::visit([&](const auto& value) {
          if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Array>) {
            printArray(value, indent + 1); // Recursive call with increased indent
          }
          else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double>) {
            std::wcout << std::wstring((indent + 1) * 2, L' ') << value;
          }
          else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, wchar_t>) {
            std::wcout << std::wstring((indent + 1) * 2, L' ') << value;
          }
          else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, String>) {
            std::wcout << std::wstring((indent + 1) * 2, L' ') << L"\"" << value << L"\"";
          }
          }, arr[i]);
        if (i < arr.size() - 1) {
          std::wcout << L",\n";
        }
        else {
          std::wcout << L"\n";
        }
      }
      std::wcout << std::wstring(indent * 2, L' ') << L"]";
    }
    else {
      // Flat array: print elements on the same line
      for (size_t i = 0; i < arr.size(); ++i) {
        std::visit([&](const auto& value) {
          if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Array>) {
            printArray(value, indent); // Should not happen, but handle safely
          }
          else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double>) {
            std::wcout << value;
          }
          else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, wchar_t>) {
            std::wcout << value;
          }
          else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, String>) {
            std::wcout << L"\"" << value << L"\"";
          }
          }, arr[i]);
        if (i < arr.size() - 1) {
          std::wcout << L" ";
        }
      }
      std::wcout << L"]";
    }
  }

  // Helper function to compute the shape of an array
  void getShape(const Array& arr, std::vector<size_t>& shape) {
    shape.push_back(arr.size());
    if (!arr.empty() && std::holds_alternative<Array>(arr[0])) {
      bool isUniform = true;
      size_t firstSize = std::get<Array>(arr[0]).size();
      for (size_t i = 1; i < arr.size(); ++i) {
        if (!std::holds_alternative<Array>(arr[i]) || std::get<Array>(arr[i]).size() != firstSize) {
          isUniform = false;
          break;
        }
      }
      if (isUniform) {
        getShape(std::get<Array>(arr[0]), shape);
      }
    }
  }

  // Helper function to compare shapes
  bool shapesEqual(const std::vector<size_t>& shape1, const std::vector<size_t>& shape2) {
    return shape1 == shape2;
  }

  // Apply binary operation with scalar extension for N-D arrays
  void applyBinaryOp(Stack& s, const String& opName, std::function<double(double, double)> op) {
    if (s.size() < 2) {
      std::wcerr << L"Error: Insufficient stack elements for " << opName << std::endl;
      return;
    }
    Array b = s.top(); s.pop();
    Array a = s.top(); s.pop();
    Array result;

    // Check if operands are scalars
    bool aIsScalar = (a.size() == 1 && !std::holds_alternative<Array>(a[0]) && std::holds_alternative<double>(a[0]));
    bool bIsScalar = (b.size() == 1 && !std::holds_alternative<Array>(b[0]) && std::holds_alternative<double>(b[0]));

    // Get shapes of operands
    std::vector<size_t> shapeA, shapeB;
    getShape(a, shapeA);
    getShape(b, shapeB);

    // Recursive helper to apply operation
    std::function<Array(const Array&, const Array&, const std::vector<size_t>&)> applyOpRecursive =
      [&](const Array& x, const Array& y, const std::vector<size_t>& shape) -> Array {
      Array res;
      if (shape.size() == 1) {
        // Leaf level: apply operation element-wise
        for (size_t i = 0; i < shape[0]; ++i) {
          Element xElem = x.size() == 1 ? x[0] : x[i];
          Element yElem = y.size() == 1 ? y[0] : y[i];
          if (!std::holds_alternative<double>(xElem) || !std::holds_alternative<double>(yElem)) {
            std::wcerr << L"Error: " << opName << L" requires numeric arguments" << std::endl;
            return Array();
          }
          double yVal = std::get<double>(yElem);
          if (opName == L"/" && yVal == 0.0) {
            std::wcerr << L"Error: Division by zero" << std::endl;
            return Array();
          }
          res.push_back(op(std::get<double>(xElem), yVal));
        }
      }
      else {
        // Nested level: recurse into sub-arrays
        size_t size = shape[0];
        std::vector<size_t> subShape(shape.begin() + 1, shape.end());
        for (size_t i = 0; i < size; ++i) {
          Array xSub = x.size() == 1 && std::holds_alternative<Array>(x[0]) ? std::get<Array>(x[0]) : std::get<Array>(x[i]);
          Array ySub = y.size() == 1 && std::holds_alternative<Array>(y[0]) ? std::get<Array>(y[0]) : std::get<Array>(y[i]);
          Array subRes = applyOpRecursive(xSub, ySub, subShape);
          if (subRes.empty()) {
            return Array(); // Propagate error
          }
          res.push_back(subRes);
        }
      }
      return res;
      };

    if (aIsScalar && !bIsScalar) {
      // Scalar op Array: replicate scalar to match b's shape
      result = applyOpRecursive(a, b, shapeB);
    }
    else if (bIsScalar && !aIsScalar) {
      // Array op Scalar: replicate scalar to match a's shape
      result = applyOpRecursive(a, b, shapeA);
    }
    else if (shapesEqual(shapeA, shapeB)) {
      // Array op Array: must have same shape
      result = applyOpRecursive(a, b, shapeA);
    }
    else {
      std::wcerr << L"Error: " << opName << L" requires a scalar or arrays of equal shape" << std::endl;
      return;
    }

    if (result.empty()) {
      return; // Error already reported
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

    builtIns[L"reshape"] = [this](Stack& s) {
      if (s.size() < 2) {
        std::wcerr << L"Error: Insufficient stack elements for reshape" << std::endl;
        return;
      }
      Array shape = s.top(); s.pop();
      Array data = s.top(); s.pop();

      // Validate shape array
      if (shape.empty()) {
        std::wcerr << L"Error: reshape requires a non-empty shape array" << std::endl;
        return;
      }
      std::vector<size_t> dims;
      size_t total_size = 1;
      for (const auto& elem : shape) {
        if (!std::holds_alternative<double>(elem)) {
          std::wcerr << L"Error: reshape shape must contain numeric values" << std::endl;
          return;
        }
        double val = std::get<double>(elem);
        if (val <= 0 || std::floor(val) != val) {
          std::wcerr << L"Error: reshape dimensions must be positive integers" << std::endl;
          return;
        }
        size_t dim = static_cast<size_t>(val);
        dims.push_back(dim);
        total_size *= dim;
      }

      // Check if data size matches the product of shape dimensions
      if (data.size() != total_size) {
        std::wcerr << L"Error: Data size does not match shape dimensions" << std::endl;
        return;
      }

      // Recursive helper to build nested arrays
      std::function<Array(size_t, const std::vector<size_t>&, size_t&)> buildArray =
        [&](size_t dimIdx, const std::vector<size_t>& dims, size_t& dataIdx) -> Array {
        Array result;
        if (dimIdx == dims.size() - 1) {
          // Leaf level: add elements directly
          for (size_t i = 0; i < dims[dimIdx]; ++i) {
            if (dataIdx < data.size()) {
              result.push_back(data[dataIdx++]);
            }
          }
        }
        else {
          // Nested level: create sub-arrays
          for (size_t i = 0; i < dims[dimIdx]; ++i) {
            result.push_back(buildArray(dimIdx + 1, dims, dataIdx));
          }
        }
        return result;
        };

      size_t dataIdx = 0;
      Array result = buildArray(0, dims, dataIdx);
      s.push(result);
      };

    builtIns[L"dim"] = [this](Stack& s) {
      if (s.empty()) {
        std::wcerr << L"Error: Stack empty for dim" << std::endl;
        return;
      }
      Array arr = s.top(); s.pop();
      Array result;

      // Scalar: single-element array with non-Array element
      if (arr.size() == 1 && !std::holds_alternative<Array>(arr[0])) {
        // Empty array for scalar
        s.push(result);
        return;
      }

      // Recursive helper to compute dimensions
      std::function<void(const Array&, std::vector<size_t>&)> getDims =
        [&](const Array& current, std::vector<size_t>& dims) {
        if (current.empty()) {
          dims.push_back(0);
          return;
        }
        dims.push_back(current.size());
        // Check if elements are arrays and uniform
        bool hasArrays = false;
        size_t subSize = 0;
        for (size_t i = 0; i < current.size(); ++i) {
          if (std::holds_alternative<Array>(current[i])) {
            Array subArr = std::get<Array>(current[i]);
            if (i == 0) {
              subSize = subArr.size();
              hasArrays = true;
            }
            else if (subArr.size() != subSize) {
              std::wcerr << L"Error: Non-uniform array for dim" << std::endl;
              s.push(result); // Push empty result before returning
              return;
            }
          }
          else if (i == 0) {
            // First element is not an array, treat as 1D
            return;
          }
          else {
            // Mixed types (array and non-array), treat as non-uniform
            std::wcerr << L"Error: Non-uniform array for dim" << std::endl;
            s.push(result); // Push empty result before returning
            return;
          }
        }
        if (hasArrays) {
          // Recurse into the first sub-array
          getDims(std::get<Array>(current[0]), dims);
        }
        };

      std::vector<size_t> dims;
      getDims(arr, dims);
      // Check if an error occurred (dims unchanged due to early return)
      if (dims.empty() && !result.empty()) {
        return; // Error already handled
      }
      for (size_t dim : dims) {
        result.push_back(static_cast<double>(dim));
      }
      s.push(result);
      };

    builtIns[L"matmul"] = [this](Stack& s) {
      if (s.size() < 2) {
        std::wcerr << L"Error: Insufficient stack elements for matmul" << std::endl;
        return;
      }
      Array b = s.top(); s.pop();
      Array a = s.top(); s.pop();

      // Validate that inputs are 2D arrays
      std::vector<size_t> shapeA, shapeB;
      getShape(a, shapeA);
      getShape(b, shapeB);
      if (shapeA.size() != 2 || shapeB.size() != 2) {
        std::wcerr << L"Error: matmul requires 2D arrays" << std::endl;
        return;
      }

      // Validate dimensions: a is [m, n], b is [n, p]
      size_t m = shapeA[0], n = shapeA[1];
      size_t n_b = shapeB[0], p = shapeB[1];
      if (n != n_b) {
        std::wcerr << L"Error: Incompatible dimensions for matmul" << std::endl;
        return;
      }

      // Validate that all elements are numeric
      for (const auto& row : a) {
        if (!std::holds_alternative<Array>(row)) {
          std::wcerr << L"Error: matmul requires 2D numeric arrays" << std::endl;
          return;
        }
        for (const auto& elem : std::get<Array>(row)) {
          if (!std::holds_alternative<double>(elem)) {
            std::wcerr << L"Error: matmul requires numeric elements" << std::endl;
            return;
          }
        }
      }
      for (const auto& row : b) {
        if (!std::holds_alternative<Array>(row)) {
          std::wcerr << L"Error: matmul requires 2D numeric arrays" << std::endl;
          return;
        }
        for (const auto& elem : std::get<Array>(row)) {
          if (!std::holds_alternative<double>(elem)) {
            std::wcerr << L"Error: matmul requires numeric elements" << std::endl;
            return;
          }
        }
      }

      // Perform matrix multiplication
      Array result;
      for (size_t i = 0; i < m; ++i) {
        Array row;
        for (size_t j = 0; j < p; ++j) {
          double sum = 0.0;
          for (size_t k = 0; k < n; ++k) {
            double a_val = std::get<double>(std::get<Array>(a[i])[k]);
            double b_val = std::get<double>(std::get<Array>(b[k])[j]);
            sum += a_val * b_val;
          }
          row.push_back(sum);
        }
        result.push_back(row);
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

int main()
{
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
