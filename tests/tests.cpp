#include <slic.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <string>

// ============================================================================
// Test structures
// ============================================================================

struct SimpleOptions {
    bool verbose = false;
    int count = 0;
    std::string_view name;

    static constexpr auto Options = std::make_tuple(
        slic::Option{"-v", "--verbose", &SimpleOptions::verbose, "Enable verbose output"},
        slic::Option{"-c", "--count", &SimpleOptions::count, "Set count"},
        slic::Arg{"name", &SimpleOptions::name, "The name to use"}
    );
};

struct BoolOptions {
    bool flag1 = false;
    bool flag2 = false;
    std::optional<bool> optFlag;

    static constexpr auto Options = std::make_tuple(
        slic::Option{"-a", "--flag1", &BoolOptions::flag1},
        slic::Option{"-b", "--flag2", &BoolOptions::flag2},
        slic::Option{"-o", "--opt-flag", &BoolOptions::optFlag}
    );
};

struct NumericOptions {
    int intVal = 0;
    long longVal = 0;
    unsigned int uintVal = 0;
    float floatVal = 0.0f;
    double doubleVal = 0.0;

    static constexpr auto Options = std::make_tuple(
        slic::Option{"-i", "--int", &NumericOptions::intVal},
        slic::Option{"-l", "--long", &NumericOptions::longVal},
        slic::Option{"-u", "--uint", &NumericOptions::uintVal},
        slic::Option{"-f", "--float", &NumericOptions::floatVal},
        slic::Option{"-d", "--double", &NumericOptions::doubleVal}
    );
};

struct OptionalArgOptions {
    std::string_view required;
    std::optional<std::string_view> optional;

    static constexpr auto Options = std::make_tuple(
        slic::Arg{"required", &OptionalArgOptions::required, "Required argument"},
        slic::Arg{"optional", &OptionalArgOptions::optional, "Optional argument"}
    );
};

struct VarArgsOptions {
    std::string_view command;
    slic::ArgSpan args;

    static constexpr auto Options = std::make_tuple(
        slic::Arg{"command", &VarArgsOptions::command, "Command to run"},
        slic::VarArgs{"Additional arguments", &VarArgsOptions::args}
    );
};

struct MixedOptions {
    bool debug = false;
    std::optional<int> level;
    std::string_view input;
    std::optional<std::string_view> output;

    static constexpr auto Options = std::make_tuple(
        slic::Option{"-d", "--debug", &MixedOptions::debug, "Enable debug mode"},
        slic::Option{"-l", "--level", &MixedOptions::level, "Set level"},
        slic::Arg{"input", &MixedOptions::input, "Input file"},
        slic::Arg{"output", &MixedOptions::output, "Output file"}
    );

    static constexpr auto Description = "A mixed options test program";
};

struct StringViewOption {
    std::string_view value;

    static constexpr auto Options = std::make_tuple(
        slic::Option{"-s", "--string", &StringViewOption::value}
    );
};

// ============================================================================
// Basic Parsing Tests
// ============================================================================

TEST(ArgParserTest, EmptyArgs) {
    const char* argv[] = {"program"};
    slic::ArgParser<BoolOptions> parser(1, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_FALSE(parser.result().flag1);
}

TEST(ArgParserTest, ShortBoolOption) {
    const char* argv[] = {"program", "-a"};
    slic::ArgParser<BoolOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(parser.result().flag1);
    EXPECT_FALSE(parser.result().flag2);
}

TEST(ArgParserTest, LongBoolOption) {
    const char* argv[] = {"program", "--flag2"};
    slic::ArgParser<BoolOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(parser.result().flag2);
}

TEST(ArgParserTest, MultipleBoolOptions) {
    const char* argv[] = {"program", "-a", "-b"};
    slic::ArgParser<BoolOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(parser.result().flag1);
    EXPECT_TRUE(parser.result().flag2);
}

// ============================================================================
// Bool Value Parsing Tests
// ============================================================================

TEST(BoolOptionTest, WithValueTrue) {
    const char* argv[] = {"program", "--flag1=true"};
    slic::ArgParser<BoolOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(parser.result().flag1);
}

TEST(BoolOptionTest, WithValueFalse) {
    const char* argv[] = {"program", "--flag1=false"};
    slic::ArgParser<BoolOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_FALSE(parser.result().flag1);
}

TEST(BoolOptionTest, VariousTrueValues) {
    for (auto val : {"true", "1", "yes", "on", "y"}) {
        std::string arg = std::string("--flag1=") + val;
        const char* argv[] = {"program", arg.c_str()};
        slic::ArgParser<BoolOptions> parser(2, argv);
        auto result = parser.parse();
        EXPECT_TRUE(result.isOk()) << "Failed for value: " << val;
        EXPECT_TRUE(parser.result().flag1) << "Failed for value: " << val;
    }
}

TEST(BoolOptionTest, VariousFalseValues) {
    for (auto val : {"false", "0", "no", "off", "n"}) {
        std::string arg = std::string("--flag1=") + val;
        const char* argv[] = {"program", arg.c_str()};
        slic::ArgParser<BoolOptions> parser(2, argv);
        auto result = parser.parse();
        EXPECT_TRUE(result.isOk()) << "Failed for value: " << val;
        EXPECT_FALSE(parser.result().flag1) << "Failed for value: " << val;
    }
}

// ============================================================================
// Numeric Options Tests
// ============================================================================

TEST(NumericOptionTest, IntShort) {
    const char* argv[] = {"program", "-i", "42"};
    slic::ArgParser<NumericOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().intVal, 42);
}

TEST(NumericOptionTest, IntLong) {
    const char* argv[] = {"program", "--int", "123"};
    slic::ArgParser<NumericOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().intVal, 123);
}

TEST(NumericOptionTest, IntEqualsSyntax) {
    const char* argv[] = {"program", "--int=999"};
    slic::ArgParser<NumericOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().intVal, 999);
}

TEST(NumericOptionTest, NegativeInt) {
    const char* argv[] = {"program", "--int=-50"};
    slic::ArgParser<NumericOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().intVal, -50);
}

TEST(NumericOptionTest, LongValue) {
    const char* argv[] = {"program", "--long", "9999999999"};
    slic::ArgParser<NumericOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().longVal, 9999999999L);
}

TEST(NumericOptionTest, UnsignedInt) {
    const char* argv[] = {"program", "--uint", "4294967295"};
    slic::ArgParser<NumericOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().uintVal, 4294967295U);
}

TEST(NumericOptionTest, FloatValue) {
    const char* argv[] = {"program", "--float", "3.14"};
    slic::ArgParser<NumericOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_NEAR(parser.result().floatVal, 3.14f, 0.01f);
}

TEST(NumericOptionTest, DoubleValue) {
    const char* argv[] = {"program", "--double", "3.14159265359"};
    slic::ArgParser<NumericOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_NEAR(parser.result().doubleVal, 3.14159265359, 0.0001);
}

// ============================================================================
// String Options Tests
// ============================================================================

TEST(StringOptionTest, Basic) {
    const char* argv[] = {"program", "--string", "hello world"};
    slic::ArgParser<StringViewOption> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().value, "hello world");
}

TEST(StringOptionTest, EqualsSyntax) {
    const char* argv[] = {"program", "--string=test value"};
    slic::ArgParser<StringViewOption> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().value, "test value");
}

// ============================================================================
// Positional Arguments Tests
// ============================================================================

TEST(PositionalArgTest, Basic) {
    const char* argv[] = {"program", "myname"};
    slic::ArgParser<SimpleOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().name, "myname");
}

TEST(PositionalArgTest, WithOptionBefore) {
    const char* argv[] = {"program", "-v", "myname"};
    slic::ArgParser<SimpleOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(parser.result().verbose);
    EXPECT_EQ(parser.result().name, "myname");
}

TEST(PositionalArgTest, WithOptionAfter) {
    const char* argv[] = {"program", "myname", "-v"};
    slic::ArgParser<SimpleOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(parser.result().verbose);
    EXPECT_EQ(parser.result().name, "myname");
}

TEST(PositionalArgTest, OptionWithValueThenPositional) {
    const char* argv[] = {"program", "-c", "5", "myname"};
    slic::ArgParser<SimpleOptions> parser(4, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().count, 5);
    EXPECT_EQ(parser.result().name, "myname");
}

TEST(PositionalArgTest, OptionalPresent) {
    const char* argv[] = {"program", "required_val", "optional_val"};
    slic::ArgParser<OptionalArgOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().required, "required_val");
    ASSERT_TRUE(parser.result().optional.has_value());
    EXPECT_EQ(*parser.result().optional, "optional_val");
}

TEST(PositionalArgTest, OptionalMissing) {
    const char* argv[] = {"program", "required_val"};
    slic::ArgParser<OptionalArgOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().required, "required_val");
    EXPECT_FALSE(parser.result().optional.has_value());
}

// ============================================================================
// VarArgs Tests
// ============================================================================

TEST(VarArgsTest, Basic) {
    const char* argv[] = {"program", "cmd", "arg1", "arg2", "arg3"};
    slic::ArgParser<VarArgsOptions> parser(5, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().command, "cmd");
    EXPECT_EQ(parser.result().args.size(), 3u);
    EXPECT_EQ(parser.result().args[0], "arg1");
    EXPECT_EQ(parser.result().args[1], "arg2");
    EXPECT_EQ(parser.result().args[2], "arg3");
}

TEST(VarArgsTest, WithSeparator) {
    const char* argv[] = {"program", "cmd", "--", "arg1", "arg2"};
    slic::ArgParser<VarArgsOptions> parser(5, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().command, "cmd");
    EXPECT_EQ(parser.result().args.size(), 2u);
    EXPECT_EQ(parser.result().args[0], "arg1");
}

TEST(VarArgsTest, Empty) {
    const char* argv[] = {"program", "cmd"};
    slic::ArgParser<VarArgsOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(parser.result().command, "cmd");
    EXPECT_TRUE(parser.result().args.empty());
}

TEST(VarArgsTest, Iteration) {
    const char* argv[] = {"program", "cmd", "a", "b", "c"};
    slic::ArgParser<VarArgsOptions> parser(5, argv);
    (void) parser.parse();

    std::vector<std::string_view> collected;
    for (auto arg : parser.result().args) {
        collected.push_back(arg);
    }

    ASSERT_EQ(collected.size(), 3u);
    EXPECT_EQ(collected[0], "a");
    EXPECT_EQ(collected[1], "b");
    EXPECT_EQ(collected[2], "c");
}

TEST(VarArgsTest, FrontBack) {
    const char* argv[] = {"program", "cmd", "first", "middle", "last"};
    slic::ArgParser<VarArgsOptions> parser(5, argv);
    (void) parser.parse();

    EXPECT_EQ(parser.result().args.front(), "first");
    EXPECT_EQ(parser.result().args.back(), "last");
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(ErrorTest, MissingValue) {
    const char* argv[] = {"program", "--int"};
    slic::ArgParser<NumericOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error, slic::ParseError::MissingValue);
    EXPECT_EQ(result.context, "--int");
}

TEST(ErrorTest, InvalidValue) {
    const char* argv[] = {"program", "--int", "not_a_number"};
    slic::ArgParser<NumericOptions> parser(3, argv);
    auto result = parser.parse();
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error, slic::ParseError::InvalidValue);
}

TEST(ErrorTest, UnknownOption) {
    const char* argv[] = {"program", "--unknown"};
    slic::ArgParser<BoolOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error, slic::ParseError::UnknownOption);
    EXPECT_EQ(result.context, "--unknown");
}

TEST(ErrorTest, MissingRequiredArg) {
    const char* argv[] = {"program"};
    slic::ArgParser<SimpleOptions> parser(1, argv);
    auto result = parser.parse();
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error, slic::ParseError::MissingRequiredArg);
    EXPECT_EQ(result.context, "name");
}

TEST(ErrorTest, TooManyArgs) {
    const char* argv[] = {"program", "arg1", "arg2", "arg3"};
    slic::ArgParser<OptionalArgOptions> parser(4, argv);
    auto result = parser.parse();
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error, slic::ParseError::TooManyArgs);
}

TEST(ErrorTest, InvalidBoolValue) {
    const char* argv[] = {"program", "--flag1=invalid"};
    slic::ArgParser<BoolOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_FALSE(result.isOk());
    EXPECT_EQ(result.error, slic::ParseError::InvalidValue);
}

// ============================================================================
// ParseResult Tests
// ============================================================================

TEST(ParseResultTest, BoolConversion) {
    slic::ParseResult success = slic::ParseResult::success();
    EXPECT_TRUE(static_cast<bool>(success));

    slic::ParseResult failure = slic::ParseResult::failure(slic::ParseError::UnknownOption);
    EXPECT_FALSE(static_cast<bool>(failure));
}

TEST(ParseResultTest, ErrorMessages) {
    EXPECT_EQ(slic::ParseResult::success().errorMessage(), "Success");
    EXPECT_EQ(slic::ParseResult::failure(slic::ParseError::MissingValue).errorMessage(), "Missing value for option");
    EXPECT_EQ(slic::ParseResult::failure(slic::ParseError::InvalidValue).errorMessage(), "Invalid value");
    EXPECT_EQ(slic::ParseResult::failure(slic::ParseError::UnknownOption).errorMessage(), "Unknown option");
    EXPECT_EQ(slic::ParseResult::failure(slic::ParseError::MissingRequiredArg).errorMessage(), "Missing required argument");
    EXPECT_EQ(slic::ParseResult::failure(slic::ParseError::TooManyArgs).errorMessage(), "Too many arguments");
}

// ============================================================================
// Program Name Tests
// ============================================================================

TEST(ProgramNameTest, ExtractFromPath) {
    const char* argv[] = {"/usr/bin/myprogram"};
    slic::ArgParser<BoolOptions> parser(1, argv);
    EXPECT_EQ(parser.programName(), "myprogram");
}

TEST(ProgramNameTest, NoPath) {
    const char* argv[] = {"myprogram"};
    slic::ArgParser<BoolOptions> parser(1, argv);
    EXPECT_EQ(parser.programName(), "myprogram");
}

// ============================================================================
// Mixed/Complex Scenarios Tests
// ============================================================================

TEST(MixedOptionsTest, Full) {
    const char* argv[] = {"program", "-d", "--level=5", "input.txt", "output.txt"};
    slic::ArgParser<MixedOptions> parser(5, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_TRUE(parser.result().debug);
    ASSERT_TRUE(parser.result().level.has_value());
    EXPECT_EQ(*parser.result().level, 5);
    EXPECT_EQ(parser.result().input, "input.txt");
    ASSERT_TRUE(parser.result().output.has_value());
    EXPECT_EQ(*parser.result().output, "output.txt");
}

TEST(MixedOptionsTest, Partial) {
    const char* argv[] = {"program", "input.txt"};
    slic::ArgParser<MixedOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    EXPECT_FALSE(parser.result().debug);
    EXPECT_FALSE(parser.result().level.has_value());
    EXPECT_EQ(parser.result().input, "input.txt");
    EXPECT_FALSE(parser.result().output.has_value());
}

TEST(MixedOptionsTest, OptionalBoolOption) {
    const char* argv[] = {"program", "--opt-flag=true"};
    slic::ArgParser<BoolOptions> parser(2, argv);
    auto result = parser.parse();
    EXPECT_TRUE(result.isOk());
    ASSERT_TRUE(parser.result().optFlag.has_value());
    EXPECT_TRUE(*parser.result().optFlag);
}

// ============================================================================
// Static Compile-Time Tests
// ============================================================================

TEST(StaticTest, OptionCount) {
    constexpr auto count = slic::ArgParser<SimpleOptions>::optionCount();
    EXPECT_EQ(count, 2u);
}

TEST(StaticTest, ArgumentCount) {
    constexpr auto count = slic::ArgParser<SimpleOptions>::argumentCount();
    EXPECT_EQ(count, 1u);
}

TEST(StaticTest, HasVarArgs) {
    constexpr bool hasVarArgs = slic::ArgParser<VarArgsOptions>::hasVarArgs();
    EXPECT_TRUE(hasVarArgs);

    constexpr bool noVarArgs = slic::ArgParser<SimpleOptions>::hasVarArgs();
    EXPECT_FALSE(noVarArgs);
}

// ============================================================================
// ValueParser Direct Tests
// ============================================================================

TEST(ValueParserTest, BoolTrue) {
    EXPECT_EQ(slic::ValueParser<bool>::parse("true"), true);
    EXPECT_EQ(slic::ValueParser<bool>::parse("1"), true);
    EXPECT_EQ(slic::ValueParser<bool>::parse("yes"), true);
    EXPECT_EQ(slic::ValueParser<bool>::parse("on"), true);
    EXPECT_EQ(slic::ValueParser<bool>::parse("y"), true);
}

TEST(ValueParserTest, BoolFalse) {
    EXPECT_EQ(slic::ValueParser<bool>::parse("false"), false);
    EXPECT_EQ(slic::ValueParser<bool>::parse("0"), false);
    EXPECT_EQ(slic::ValueParser<bool>::parse("no"), false);
    EXPECT_EQ(slic::ValueParser<bool>::parse("off"), false);
    EXPECT_EQ(slic::ValueParser<bool>::parse("n"), false);
}

TEST(ValueParserTest, BoolInvalid) {
    EXPECT_FALSE(slic::ValueParser<bool>::parse("invalid").has_value());
}

TEST(ValueParserTest, Int) {
    EXPECT_EQ(slic::ValueParser<int>::parse("42"), 42);
    EXPECT_EQ(slic::ValueParser<int>::parse("-10"), -10);
    EXPECT_EQ(slic::ValueParser<int>::parse("0"), 0);
}

TEST(ValueParserTest, IntInvalid) {
    EXPECT_FALSE(slic::ValueParser<int>::parse("abc").has_value());
    EXPECT_FALSE(slic::ValueParser<int>::parse("12abc").has_value());
    EXPECT_FALSE(slic::ValueParser<int>::parse("").has_value());
}

TEST(ValueParserTest, StringView) {
    EXPECT_EQ(slic::ValueParser<std::string_view>::parse("hello"), "hello");
    EXPECT_EQ(slic::ValueParser<std::string_view>::parse(""), "");
    EXPECT_EQ(slic::ValueParser<std::string_view>::parse("with spaces"), "with spaces");
}

// ============================================================================
// Misc Tests
// ============================================================================

TEST(MiscTest, ResultModification) {
    const char* argv[] = {"program"};
    slic::ArgParser<BoolOptions> parser(1, argv);
    (void) parser.parse();

    // Modify result through non-const reference
    parser.result().flag1 = true;
    EXPECT_TRUE(parser.result().flag1);
}
