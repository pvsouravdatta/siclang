#include "interpreter.hpp"
#include <cmath>

// Implementation of Interpreter class methods
bool Interpreter::isNumber(const String& token) {
    try {
        std::stod(token);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool Interpreter::isWChar(const String& token) {
    return token.length() == 1 && !isNumber(token);
}

bool Interpreter::isStringLiteral(const String& token) {
    return token.length() >= 2 && token.front() == L'"' && token.back() == L'"';
}

bool Interpreter::isArrayLiteral(const String& token) {
    return token.length() >= 2 && token.front() == L'[' && token.back() == L']';
}

bool Interpreter::isFunctionName(const String& token) {
    if (token.empty() || token == L":end" || token == L":dump") return false;
    return std::all_of(token.begin(), token.end(), [](wchar_t c) {
        return std::iswalnum(c) || c == L'_' || c > 127;
    });
}

std::vector<String> Interpreter::parseArrayTokens(const String& input) {
    std::vector<String> tokens;
    String current;
    int bracketDepth = 0;
    bool inQuotes = false;

    for (size_t i = 1; i < input.length() - 1; ++i) {
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
    current.erase(0, current.find_first_not_of(L" \t"));
    current.erase(current.find_last_not_of(L" \t") + 1);
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

Element Interpreter::parseElement(const String& token) {
    if (isNumber(token)) {
        return std::stod(token);
    }
    else if (isStringLiteral(token)) {
        return token.substr(1, token.length() - 2);
    }
    else if (isWChar(token)) {
        return token[0];
    }
    else if (isArrayLiteral(token)) {
        return parseArray(token);
    }
    return token;
}

Array Interpreter::parseArray(const String& token) {
    Array result;
    if (!isArrayLiteral(token)) {
        result.push_back(parseElement(token));
        return result;
    }

    std::vector<String> elements = parseArrayTokens(token);
    for (const auto& elem : elements) {
        result.push_back(parseElement(elem));
    }
    return result;
}

void Interpreter::printArray(const Array& arr, int indent) {
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
        std::wcout << L"\n";
        for (size_t i = 0; i < arr.size(); ++i) {
            std::visit([&](const auto& value) {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Array>) {
                    printArray(value, indent + 1);
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
        for (size_t i = 0; i < arr.size(); ++i) {
            std::visit([&](const auto& value) {
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Array>) {
                    printArray(value, indent);
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

void Interpreter::getShape(const Array& arr, std::vector<size_t>& shape) {
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

bool Interpreter::shapesEqual(const std::vector<size_t>& shape1, const std::vector<size_t>& shape2) {
    return shape1 == shape2;
}

void Interpreter::applyBinaryOp(Stack& s, const String& opName, std::function<double(double, double)> op) {
    if (s.size() < 2) {
        std::wcerr << L"Error: Insufficient stack elements for " << opName << std::endl;
        return;
    }
    Array b = s.top(); s.pop();
    Array a = s.top(); s.pop();
    Array result;

    bool aIsScalar = (a.size() == 1 && !std::holds_alternative<Array>(a[0]) && std::holds_alternative<double>(a[0]));
    bool bIsScalar = (b.size() == 1 && !std::holds_alternative<Array>(b[0]) && std::holds_alternative<double>(b[0]));

    std::vector<size_t> shapeA, shapeB;
    getShape(a, shapeA);
    getShape(b, shapeB);

    std::function<Array(const Array&, const Array&, const std::vector<size_t>&)> applyOpRecursive =
        [&](const Array& x, const Array& y, const std::vector<size_t>& shape) -> Array {
        Array res;
        if (shape.size() == 1) {
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
            size_t size = shape[0];
            std::vector<size_t> subShape(shape.begin() + 1, shape.end());
            for (size_t i = 0; i < size; ++i) {
                Array xSub = x.size() == 1 && std::holds_alternative<Array>(x[0]) ? std::get<Array>(x[0]) : std::get<Array>(x[i]);
                Array ySub = y.size() == 1 && std::holds_alternative<Array>(y[0]) ? std::get<Array>(y[0]) : std::get<Array>(y[i]);
                Array subRes = applyOpRecursive(xSub, ySub, subShape);
                if (subRes.empty()) {
                    return Array();
                }
                res.push_back(subRes);
            }
        }
        return res;
    };

    if (aIsScalar && !bIsScalar) {
        result = applyOpRecursive(a, b, shapeB);
    }
    else if (bIsScalar && !aIsScalar) {
        result = applyOpRecursive(a, b, shapeA);
    }
    else if (shapesEqual(shapeA, shapeB)) {
        result = applyOpRecursive(a, b, shapeA);
    }
    else {
        std::wcerr << L"Error: " << opName << L" requires a scalar or arrays of equal shape" << std::endl;
        return;
    }

    if (result.empty()) {
        return;
    }
    s.push(result);
}

void Interpreter::initBuiltIns() {
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

        if (data.size() != total_size) {
            std::wcerr << L"Error: Data size does not match shape dimensions" << std::endl;
            return;
        }

        std::function<Array(size_t, const std::vector<size_t>&, size_t&)> buildArray =
            [&](size_t dimIdx, const std::vector<size_t>& dims, size_t& dataIdx) -> Array {
            Array result;
            if (dimIdx == dims.size() - 1) {
                for (size_t i = 0; i < dims[dimIdx]; ++i) {
                    if (dataIdx < data.size()) {
                        result.push_back(data[dataIdx++]);
                    }
                }
            }
            else {
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

        if (arr.size() == 1 && !std::holds_alternative<Array>(arr[0])) {
            s.push(result);
            return;
        }

        std::function<void(const Array&, std::vector<size_t>&)> getDims =
            [&](const Array& current, std::vector<size_t>& dims) {
            if (current.empty()) {
                dims.push_back(0);
                return;
            }
            dims.push_back(current.size());
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
                        s.push(result);
                        return;
                    }
                }
                else if (i == 0) {
                    return;
                }
                else {
                    std::wcerr << L"Error: Non-uniform array for dim" << std::endl;
                    s.push(result);
                    return;
                }
            }
            if (hasArrays) {
                getDims(std::get<Array>(current[0]), dims);
            }
        };

        std::vector<size_t> dims;
        getDims(arr, dims);
        if (dims.empty() && !result.empty()) {
            return;
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

        std::vector<size_t> shapeA, shapeB;
        getShape(a, shapeA);
        getShape(b, shapeB);
        if (shapeA.size() != 2 || shapeB.size() != 2) {
            std::wcerr << L"Error: matmul requires 2D arrays" << std::endl;
            return;
        }

        size_t m = shapeA[0], n = shapeA[1];
        size_t n_b = shapeB[0], p = shapeB[1];
        if (n != n_b) {
            std::wcerr << L"Error: Incompatible dimensions for matmul" << std::endl;
            return;
        }

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

void Interpreter::evaluate(const std::vector<String>& tokens, bool isFunctionBody) {
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

        if (functions.find(token) != functions.end()) {
            evaluate(functions[token], true);
            continue;
        }

        if (builtIns.find(token) != builtIns.end()) {
            builtIns[token](stack);
            continue;
        }

        stack.push(parseArray(token));
    }
}

std::vector<String> Interpreter::tokenize(const String& input) {
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
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

Interpreter::Interpreter() {
    initBuiltIns();
}

void Interpreter::process(const String& input) {
    std::vector<String> tokens = tokenize(input);
    evaluate(tokens);
} 