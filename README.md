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

## Benchmarks

Some benchmarks comparing `slic` with other popular C++ command line parsers:

- [**CLI11**](https://github.com/CLIUtils/CLI11) (v2.6.1)
- [**cxxopts**](https://github.com/jarro2783/cxxopts) (v3.3.1)
- [**argparse**](https://github.com/p-ranav/argparse) (v3.2)

Benchmarks were run on an AMD Ryzen 7 7730U using Google Benchmark library in Release mode with `-O3` optimizations.

### 1. Simple case

`program -v --count 42 myfile.txt`

| Library  | Time (ns) | CPU (ns) | Iterations |
|----------|-----------|----------|------------|
| slic     | 51.8      | 51.6     | 13126669   |
| CLI11    | 5752      | 5749     | 117156     |
| cxxopts  | 7951      | 7951     | 71121      |
| argparse | 3531      | 3531     | 200750     |

### 2. Medium case

`program -v --count 42 --name test --level 3 --output out.txt input.txt`

| Library  | Time (ns) | CPU (ns) | Iterations |
|----------|-----------|----------|------------|
| slic     | 135       | 135      | 5237956    |
| CLI11    | 12998     | 12995    | 54427      |
| cxxopts  | 15516     | 15510    | 43210      |
| argparse | 6582      | 6577     | 114666     |

### 3. Complex case

`program -v --count 42 --name test --level 3 --output out.txt --debug --threads 8 --timeout 1000 --retry 3 input1.txt input2.txt`

| Library  | Time (ns) | CPU (ns) | Iterations |
|----------|-----------|----------|------------|
| slic     | 412       | 412      | 2178409    |
| CLI11    | 28640     | 28623    | 25571      |
| cxxopts  | 36981     | 36940    | 22162      |
| argparse | 11283     | 11278    | 59623      |

### 4. Flags only

`program -a -b -c -d -e -f -g -h`

| Library  | Time (ns) | CPU (ns) | Iterations |
|----------|-----------|----------|------------|
| slic     | 358       | 358      | 2089557    |
| CLI11    | 12266     | 12259    | 57548      |
| cxxopts  | 17721     | 17711    | 55006      |
| argparse | 5063      | 5061     | 135891     |

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.