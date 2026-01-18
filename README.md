# slic

Simple C++20 command line argument parser.

## Features

- Single header file
- Zero allocations
- No dependencies
- Compile-time configuration and validation
- Supports positional and named arguments
- Variadic arguments support
- Type-safe parsing of arguments
- Help message generation
- Error handling

## Usage

Here's a simple example of how to use `slic` to parse command line arguments:

```cpp
#include <slic.hpp>

// Define your argument struct
struct MyArgs {
    std::string_view input;
    std::string_view output;
    std::string_view test;
    int number = 0;
    bool version = false;
    slic::ArgSpan args;

    static constexpr std::string_view Description = "My awesome CLI tool";
    static constexpr std::tuple Options = {
        slic::Option{"--number", "-n", &MyArgs::number, "Some number"},
        slic::Option{"--version", "-v", &MyArgs::version, "Show version"},
        slic::Option{"--test", "-t", &MyArgs::test, "Test argument"},
        slic::Arg{"INPUT", &MyArgs::input, "Input file"},
        slic::Arg{"OUTPUT", &MyArgs::output, "Output file"},
        slic::VarArgs{"Extra files", &MyArgs::args}
    };
};

int main(int argc, char** argv) {
    // Create the parser
    slic::ArgParser<MyArgs> parser(argc, argv);
    
    // Parse the arguments and handle errors
    auto result = parser.parse();
    if (!result) {
        result.print();
        // you can also print help message if needed
        parser.printHelp();
        return 1;
    }
    
    MyArgs const& args = parser.result(); // parser holds your struct
    
    // access members directly
    if (args.version) {
        std::cout << "My CLI Tool version 1.0.0\n";
        return 0;
    }
    
    // access variadic arguments
    for (std::string_view arg : args.args) {
        std::cout << "Extra arg: " << arg << "\n";
    }
    
    return 0;
}
```

Help message output for the above example:

```
My awesome CLI tool

Usage: my_program [OPTIONS] <INPUT> <OUTPUT> [...]

Arguments:
  INPUT: Input file
  OUTPUT: Output file
  [...]: Extra files

Options:
  -n, --number <value>: Some number
  -v, --version: Show version
  -t, --test <value>: Test argument
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.