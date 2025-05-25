# SIC Lang - Simple Interpreted Concatenative Lang

SIC Lang is a modern programming language that elegantly combines the stack-based paradigm of Forth with the powerful array operations of APL. It offers a unique blend of concatenative programming and array manipulation, making it both expressive and efficient for numerical computing.

## Features

- **Stack-Based Operations**: Like Forth, SIC Lang uses a stack for data manipulation, making it naturally composable
- **Array Programming**: Inspired by APL, supports powerful array operations and matrix manipulations
- **Dynamic Typing**: Handles numbers, characters, strings, and nested arrays seamlessly
- **User-Defined Functions**: Create your own functions using the `:name` syntax
- **Unicode Support**: Full support for Unicode characters and strings

## Quick Start

```bash
# Build the interpreter
make

# Run the interpreter
./siclang  # On Unix
siclang.exe  # On Windows
```

## Code Examples

### Basic Stack Operations

```forth
1 2 + .        # Push 1, push 2, add them, print result: [3]
"Hello" .      # Push string, print it: [Hello]
```

### Array Operations

```forth
# Create and manipulate arrays
[1 2 3] .      # Create array, print it: [1 2 3]
[1 2 3] dup .  # Duplicate array: [1 2 3] [1 2 3]

# Matrix operations
[[1 2] [3 4]] [[5 6] [7 8]] matmul .  # Matrix multiplication
# Result: [[19 22] [43 50]]

# Array reshaping
[1 2 3 4] [2 2] reshape .  # Reshape to 2x2 matrix
# Result: [[1 2] [3 4]]
```

### Function Definition

```forth
# Define a function to greet world
:greet "Hello " swap cat :end

"world" factorial .  # Hello world
```

### Built-in Functions

#### Arithmetic Operations
```forth
1 2 + .    # Addition: [3]
5 3 - .    # Subtraction: [2]
4 3 * .    # Multiplication: [12]
10 2 / .   # Division: [5]
2 3 ^ .    # Exponentiation: [8]
```

#### Array Operations
```forth
[1 2 3] [4 5 6] + .  # Concatenation: [1 2 3 4 5 6]
[1 2 3] dup .          # Duplicate: [1 2 3] [1 2 3]
[1 2 3] [4 5 6] swap . # Swap: [4 5 6] [1 2 3]
```

#### Matrix Operations
```forth
# Create matrices
[[1 2] [3 4]] [[5 6] [7 8]] matmul .  # Matrix multiplication
# Result: [[19 22] [43 50]]

# Get dimensions
[[1 2] [3 4]] dim .  # Get matrix dimensions: [2 2]
```

#### Utility Functions
```forth
clear           # Clear the stack
:dump           # Show stack contents
5 range .       # Generate range: [0 1 2 3 4]
```

## Building from Source

```bash
# Clone the repository
git clone https://github.com/pvsouravdatta/siclang.git
cd siclang

# Build the interpreter
make

# Run the interpreter
./siclang  # On Unix
siclang.exe  # On Windows
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. 