#pragma once
#ifndef SLIC_ARG_PARSER_HPP
#define SLIC_ARG_PARSER_HPP

#include <array>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace slic {
    namespace detail {
        template <typename T>
        struct unwrap_optional { using type = T; };

        template <typename T>
        struct unwrap_optional<std::optional<T>> { using type = T; };

        template <typename T>
        using unwrap_optional_t = unwrap_optional<T>::type;

        template <typename T>
        constexpr bool is_optional_v = false;

        template <typename T>
        constexpr bool is_optional_v<std::optional<T>> = true;
    } // namespace detail

    enum class ParseError : uint8_t {
        None = 0,
        MissingValue,
        InvalidValue,
        UnknownOption,
        MissingRequiredArg,
        TooManyArgs
    };

    /// @brief Result of a parsing operation with error information.
    struct ParseResult {
        ParseError error = ParseError::None;
        std::string_view context{};

        [[nodiscard]] constexpr bool isOk() const noexcept { return error == ParseError::None; }
        [[nodiscard]] constexpr explicit operator bool() const noexcept { return isOk(); }

        static constexpr ParseResult success() noexcept { return {}; }
        static constexpr ParseResult failure(ParseError err, std::string_view ctx = {}) noexcept {
            return {err, ctx};
        }

        [[nodiscard]] constexpr std::string_view errorMessage() const noexcept {
            switch (error) {
                case ParseError::None: return "Success";
                case ParseError::MissingValue: return "Missing value for option";
                case ParseError::InvalidValue: return "Invalid value";
                case ParseError::UnknownOption: return "Unknown option";
                case ParseError::MissingRequiredArg: return "Missing required argument";
                case ParseError::TooManyArgs: return "Too many arguments";
            }
            return "Unknown error";
        }

        /// @brief Prints the error message to stderr.
        void print() const noexcept {
            if (isOk()) return;
            if (context.empty()) {
                std::cerr << "Error: " << errorMessage() << std::endl;
            } else {
                std::cerr << "Error: " << errorMessage() << " '" << context << '\'' << std::endl;
            }
        }
    };

    template <typename T>
    struct ValueParser {
        static constexpr std::optional<T> parse(std::string_view input) noexcept {
            if constexpr (std::is_same_v<T, bool>) {
                if (input == "true" || input == "1" || input == "yes" || input == "on" || input == "y") {
                    return true;
                }
                if (input == "false" || input == "0" || input == "no" || input == "off" || input == "n") {
                    return false;
                }
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, std::string_view>) {
                return input;
            } else if constexpr (std::is_arithmetic_v<T>) {
                T value{};
                auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value);
                if (ec == std::errc{} && ptr == input.data() + input.size()) {
                    return value;
                }
                return std::nullopt;
            } else {
                return std::nullopt;
            }
        }
    };

    /// @brief Span-like view over variadic arguments.
    struct ArgSpan {
        using value_type = std::string_view;

        struct iterator {
            char const* const* ptr;

            constexpr std::string_view operator*() const noexcept { return *ptr; }
            constexpr iterator& operator++() noexcept { ++ptr; return *this; }
            constexpr iterator operator++(int) noexcept { auto tmp = *this; ++ptr; return tmp; }
            constexpr bool operator==(iterator const&) const noexcept = default;
        };

        constexpr ArgSpan() noexcept = default;
        constexpr ArgSpan(std::span<char const* const> args) noexcept : m_args(args) {}

        [[nodiscard]] constexpr iterator begin() const noexcept { return {m_args.data()}; }
        [[nodiscard]] constexpr iterator end() const noexcept { return {m_args.data() + m_args.size()}; }
        [[nodiscard]] constexpr size_t size() const noexcept { return m_args.size(); }
        [[nodiscard]] constexpr bool empty() const noexcept { return m_args.empty(); }
        [[nodiscard]] constexpr std::string_view operator[](size_t idx) const noexcept { return m_args[idx]; }
        [[nodiscard]] constexpr std::string_view front() const noexcept { return m_args.front(); }
        [[nodiscard]] constexpr std::string_view back() const noexcept { return m_args.back(); }

    private:
        std::span<char const* const> m_args{};
    };

    /// @brief Represents a command-line option. (e.g., --option or -o)
    template <typename T, typename S>
    struct Option {
        using Type = T;
        using Parent = S;

        constexpr Option(std::string_view name, T S::* field) noexcept
            : m_name(name), m_field(field) {}

        constexpr Option(std::string_view name, std::string_view altName, T S::* field) noexcept
            : m_name(name), m_altName(altName), m_field(field) {}

        constexpr Option(std::string_view name, T S::* field, std::string_view description) noexcept
            : m_name(name), m_description(description), m_field(field) {}

        constexpr Option(std::string_view name, std::string_view altName, T S::* field, std::string_view description) noexcept
            : m_name(name), m_altName(altName), m_description(description), m_field(field) {}

        [[nodiscard]] constexpr std::string_view name() const noexcept { return m_name; }
        [[nodiscard]] constexpr std::string_view altName() const noexcept { return m_altName; }
        [[nodiscard]] constexpr std::string_view description() const noexcept { return m_description; }
        [[nodiscard]] constexpr Type Parent::* field() const noexcept { return m_field; }

        [[nodiscard]] constexpr bool matches(std::string_view arg) const noexcept {
            return arg == m_name || arg == m_altName;
        }

        [[nodiscard]] static constexpr bool needsValue() noexcept {
            using Inner = detail::unwrap_optional_t<T>;
            return !std::is_same_v<Inner, bool>;
        }

    private:
        std::string_view m_name{};
        std::string_view m_altName{};
        std::string_view m_description{};
        T S::* m_field{};
    };

    /// @brief Represents a positional argument (e.g., filename).
    template <typename T, typename S>
    struct Arg {
        using Type = T;
        using Parent = S;

        constexpr Arg(std::string_view name, T S::* field) noexcept
            : m_name(name), m_field(field) {}

        constexpr Arg(std::string_view name, T S::* field, std::string_view description) noexcept
            : m_name(name), m_description(description), m_field(field) {}

        [[nodiscard]] constexpr std::string_view name() const noexcept { return m_name; }
        [[nodiscard]] constexpr std::string_view description() const noexcept { return m_description; }
        [[nodiscard]] constexpr Type Parent::* field() const noexcept { return m_field; }

        [[nodiscard]] static consteval bool isOptional() noexcept {
            return detail::is_optional_v<T>;
        }

    private:
        std::string_view m_name{};
        std::string_view m_description{};
        T S::* m_field{};
    };

    /// @brief Represents variadic arguments, that attaches to ArgSpan.
    template <typename S>
    struct VarArgs {
        using Type = ArgSpan;
        using Parent = S;

        constexpr VarArgs(ArgSpan S::* field) noexcept
            : m_field(field) {}

        constexpr VarArgs(std::string_view description, ArgSpan S::* field) noexcept
            : m_description(description), m_field(field) {}

        [[nodiscard]] constexpr std::string_view description() const { return m_description; }
        [[nodiscard]] constexpr Type Parent::* field() const noexcept { return m_field; }

    private:
        std::string_view m_description{};
        ArgSpan S::* m_field;
    };

    namespace detail {
        template <typename T>
        struct is_option : std::false_type {};

        template <typename T, typename S>
        struct is_option<Option<T, S>> : std::true_type {};

        template <typename T>
        constexpr bool is_option_v = is_option<std::remove_cvref_t<T>>::value;

        template <typename T>
        struct is_arg : std::false_type {};

        template <typename T, typename S>
        struct is_arg<Arg<T, S>> : std::true_type {};

        template <typename T>
        constexpr bool is_arg_v = is_arg<std::remove_cvref_t<T>>::value;

        template <typename T>
        struct is_varargs : std::false_type {};

        template <typename S>
        struct is_varargs<VarArgs<S>> : std::true_type {};

        template <typename T>
        constexpr bool is_varargs_v = is_varargs<std::remove_cvref_t<T>>::value;
    } // namespace detail

    /// @brief Command-line argument parser for a given options struct T.
    template <class T>
    class ArgParser {
    private:
        using OptsT = decltype(T::Options);
        static constexpr size_t TupleSize = std::tuple_size_v<OptsT>;

    public:
        constexpr ArgParser(int argc, char const* const* argv) noexcept : m_argc(argc), m_argv(argv) {
            if (argc > 0) {
                m_programName = argv[0];
                auto slash = m_programName.find_last_of('/');
                if (slash != std::string_view::npos) {
                    m_programName = m_programName.substr(slash + 1);
                }
            }
        }

        static consteval size_t optionCount() noexcept {
            size_t count = 0;
            [&]<size_t... I>(std::index_sequence<I...>) {
                ((detail::is_option_v<std::tuple_element_t<I, OptsT>> ? ++count : 0), ...);
            }(std::make_index_sequence<TupleSize>());
            return count;
        }

        static consteval size_t argumentCount() noexcept {
            size_t count = 0;
            [&]<size_t... I>(std::index_sequence<I...>) {
                ((detail::is_arg_v<std::tuple_element_t<I, OptsT>> ? ++count : 0), ...);
            }(std::make_index_sequence<TupleSize>());
            return count;
        }

        static consteval bool hasVarArgs() noexcept {
            bool found = false;
            [&]<size_t... I>(std::index_sequence<I...>) {
                ((detail::is_varargs_v<std::tuple_element_t<I, OptsT>> ? (found = true) : false), ...);
            }(std::make_index_sequence<TupleSize>());
            return found;
        }

        static consteval size_t varArgsIndex() noexcept {
            static_assert(hasVarArgs(), "Index is only accessible if varargs exist");
            size_t count = 0;
            [&]<size_t... I>(std::index_sequence<I...>) {
                ((detail::is_varargs_v<std::tuple_element_t<I, OptsT>> ? (count = I) : false), ...);
            }(std::make_index_sequence<TupleSize>());
            return count;
        }

        [[nodiscard]] constexpr T const& result() const noexcept { return m_options; }
        [[nodiscard]] constexpr T& result() noexcept { return m_options; }
        [[nodiscard]] constexpr std::string_view programName() const noexcept { return m_programName; }

        [[nodiscard]] constexpr ParseResult parse() noexcept {
            size_t positionalIndex = 0;
            int varArgsStart = -1;

            for (int i = 1; i < m_argc; ++i) {
                std::string_view arg = m_argv[i];

                // vararg separator
                if (arg == "--") {
                    if (i + 1 < m_argc) {
                        varArgsStart = i + 1;
                    }
                    break;
                }

                // check option
                if (arg.starts_with('-')) {
                    auto result = tryParseOption(arg, i);
                    if (!result.isOk()) {
                        return result;
                    }
                } else {
                    // positional argument
                    auto result = tryParsePositional(arg, positionalIndex);
                    if (result.isOk()) {
                        ++positionalIndex;
                    } else if (result.error == ParseError::TooManyArgs) {
                        if constexpr (hasVarArgs()) {
                            varArgsStart = i;
                            break;
                        } else {
                            return result;
                        }
                    } else {
                        return result;
                    }
                }
            }

            if (varArgsStart >= 0) {
                setVarArgs(varArgsStart);
            }

            return checkRequired(positionalIndex);
        }

        /// @brief Prints an ANSI-formatted help message to stdout.
        void printHelp() const noexcept {
            #define ANSI_BOLD "\x1b[1m"
            #define ANSI_UNDERLINE "\x1b[4m"
            #define ANSI_RESET "\x1b[0m"
            #define ANSI_BOLDLINE ANSI_BOLD ANSI_UNDERLINE

            if constexpr (requires { T::Description; }) {
                std::cout << T::Description << '\n';
            }

            std::cout << ANSI_BOLDLINE "Usage:" ANSI_RESET " " << m_programName;

            if constexpr (optionCount() > 0) {
                std::cout << " [OPTIONS]";
            }

            forEachArg([](auto const& arg) {
                if constexpr (arg.isOptional()) {
                    std::cout << " [" << arg.name() << ']';
                } else {
                    std::cout << " <" << arg.name() << '>';
                }
            });

            if constexpr (hasVarArgs()) {
                std::cout << " [...]";
            }

            std::cout << '\n';

            if constexpr (argumentCount() > 0 || hasVarArgs()) {
                std::cout << "\n" ANSI_BOLDLINE "Arguments:" ANSI_RESET "\n";

                forEachArg([](auto const& arg) {
                    std::cout << "  " ANSI_BOLD << arg.name() << ANSI_RESET ": " << arg.description() << '\n';
                });

                if constexpr (hasVarArgs()) {
                    std::cout << "  " ANSI_BOLD "[...]" ANSI_RESET ": " << std::get<varArgsIndex()>(T::Options).description() << '\n';
                }
            }

            if constexpr (optionCount() > 0) {
                std::cout << "\n" ANSI_BOLDLINE "Options:" ANSI_RESET "\n";

                forEachOption([](auto const& opt) {
                    if (opt.altName().empty()) {
                        std::cout << "  " ANSI_BOLD << opt.name() << ANSI_RESET;
                    } else {
                        std::cout << "  " ANSI_BOLD << opt.altName() << ", " << opt.name() << ANSI_RESET;
                    }

                    if constexpr (opt.needsValue()) {
                        std::cout << " <value>";
                    }

                    if (!opt.description().empty()) {
                        std::cout << ": " << opt.description();
                    }
                    std::cout << '\n';
                });
            }

            #undef ANSI_BOLD
            #undef ANSI_UNDERLINE
            #undef ANSI_RESET
            #undef ANSI_BOLDLINE
        }

    private:
        template <typename F>
        static constexpr void forEachOption(F&& func) {
            [&]<size_t... I>(std::index_sequence<I...>){
                ([&] {
                    if constexpr (detail::is_option_v<std::tuple_element_t<I, OptsT>>) {
                        func(std::get<I>(T::Options));
                    }
                }(), ...);
            }(std::make_index_sequence<TupleSize>());
        }

        template <typename F>
        static constexpr void forEachArg(F&& func) {
            [&]<size_t... I>(std::index_sequence<I...>){
                ([&] {
                    if constexpr (detail::is_arg_v<std::tuple_element_t<I, OptsT>>) {
                        func(std::get<I>(T::Options));
                    }
                }(), ...);
            }(std::make_index_sequence<TupleSize>());
        }

        enum class IterResult { Continue, Break };

        template <typename F>
        static constexpr IterResult forEachOptionUntil(F&& func) {
            IterResult result = IterResult::Continue;
            [&]<size_t... I>(std::index_sequence<I...>){
                ([&] {
                    if (result == IterResult::Break) return;
                    if constexpr (detail::is_option_v<std::tuple_element_t<I, OptsT>>) {
                        result = func(std::get<I>(T::Options));
                    }
                }(), ...);
            }(std::make_index_sequence<TupleSize>());
            return result;
        }

        template <typename F>
        static constexpr IterResult forEachArgIndexed(F&& func) {
            IterResult result = IterResult::Continue;
            size_t idx = 0;
            [&]<size_t... I>(std::index_sequence<I...>){
                ([&] {
                    if (result == IterResult::Break) return;
                    if constexpr (detail::is_arg_v<std::tuple_element_t<I, OptsT>>) {
                        result = func(std::get<I>(T::Options), idx++);
                    }
                }(), ...);
            }(std::make_index_sequence<TupleSize>());
            return result;
        }

        constexpr ParseResult tryParseOption(std::string_view arg, int& index) {
            // handle --option=value syntax
            auto eqPos = arg.find('=');
            std::string_view optName = (eqPos != std::string_view::npos) ? arg.substr(0, eqPos) : arg;
            std::optional<std::string_view> inlineValue = (eqPos != std::string_view::npos)
                ? std::optional{arg.substr(eqPos + 1)}
                : std::nullopt;

            ParseResult result = ParseResult::failure(ParseError::UnknownOption, optName);

            forEachOptionUntil([&]<typename O>(O const& opt) -> IterResult {
                if (!opt.matches(optName)) {
                    return IterResult::Continue;
                }

                using FieldType = std::remove_cvref_t<O>::Type;
                using InnerType = detail::unwrap_optional_t<FieldType>;

                if constexpr (std::is_same_v<InnerType, bool>) {
                    if (inlineValue) {
                        auto parsed = ValueParser<bool>::parse(*inlineValue);
                        if (!parsed) {
                            result = ParseResult::failure(ParseError::InvalidValue, arg);
                            return IterResult::Break;
                        }
                        m_options.*opt.field() = *parsed;
                    } else {
                        m_options.*opt.field() = true;
                    }
                    result = ParseResult::success();
                } else {
                    std::string_view value;
                    if (inlineValue) {
                        value = *inlineValue;
                    } else if (index + 1 < m_argc) {
                        value = m_argv[++index];
                    } else {
                        result = ParseResult::failure(ParseError::MissingValue, optName);
                        return IterResult::Break;
                    }

                    auto parsed = ValueParser<InnerType>::parse(value);
                    if (!parsed) {
                        result = ParseResult::failure(ParseError::InvalidValue, arg);
                        return IterResult::Break;
                    }

                    m_options.*opt.field() = *parsed;
                    result = ParseResult::success();
                }

                return IterResult::Break;
            });

            return result;
        }

        constexpr ParseResult tryParsePositional(std::string_view value, size_t targetIndex) noexcept {
            ParseResult result = ParseResult::failure(ParseError::TooManyArgs, value);

            forEachArgIndexed([&]<typename A>(A const& arg, size_t idx) -> IterResult {
                if (idx != targetIndex) {
                    return IterResult::Continue;
                }

                using FieldType = std::remove_cvref_t<A>::Type;
                using InnerType = detail::unwrap_optional_t<FieldType>;

                auto parsed = ValueParser<InnerType>::parse(value);
                if (!parsed) {
                    result = ParseResult::failure(ParseError::InvalidValue, value);
                    return IterResult::Break;
                }

                m_options.*arg.field() = *parsed;
                result = ParseResult::success();

                return IterResult::Break;
            });

            return result;
        }

        constexpr void setVarArgs(int startIndex) noexcept {
            if constexpr (hasVarArgs()) {
                m_options.*std::get<varArgsIndex()>(T::Options).field() = std::span{
                    m_argv + startIndex,
                    static_cast<size_t>(m_argc - startIndex)
                };
            }
        }

        [[nodiscard]] constexpr ParseResult checkRequired(size_t count) const noexcept {
            std::string_view missingArg;

            forEachArgIndexed([&]<typename A>(A const& arg, size_t idx) -> IterResult {
                if constexpr (!A::isOptional()) {
                    if (idx >= count && missingArg.empty()) {
                        missingArg = arg.name();
                        return IterResult::Break;
                    }
                }
                return IterResult::Continue;
            });

            if (!missingArg.empty()) {
                return ParseResult::failure(ParseError::MissingRequiredArg, missingArg);
            }

            return ParseResult::success();
        }

    private:
        T m_options{};
        int m_argc{};
        char const* const* m_argv{};
        std::string_view m_programName{};
    };
} // namespace slic

#endif // SLIC_ARG_PARSER_HPP