#include "interpreter.hpp"
#include <iostream>

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

    std::wcout << L"SIC Lang - Simple Interpreted Concatenative Lang\n";
    std::wcout << L"Type 'exit' to quit\n";

    while (true) {
        std::wcout << L"> ";
        std::getline(std::wcin, line);
        if (line == L"exit") break;
        interp.process(line);
    }

    return 0;
}
