#include <variant>
#include <memory>
#include <cinttypes>
#include <cmath>
#include <tuple>
#include <iostream>
#include <cctype>
#include <cassert>

int64_t powInt(int64_t a, int64_t p)
{
    // assume p >= 0
    if (p == 0)
        return 1;
    else {
        int64_t s = (p & 1) == 0 ? 1 : a;
        auto t = powInt(a, p/2);
        return t * t * s;
    }
}

template <typename... Fs>
struct overload : Fs...
{
    using Fs::operator()...;
    overload(Fs... fs) : Fs(std::forward<Fs>(fs))... {}
};

template <typename... Fs>
overload(Fs&&...) -> overload<Fs...>;

template <typename... Ts>
struct Matcher
{
    std::tuple<Ts...> args;

    constexpr Matcher(Ts... a) : args(a...) {}

    template <typename F, size_t... I>
    constexpr auto call_impl(F&& f, std::index_sequence<I...>)
    {
        return std::visit(f, std::get<I>(args)...);
    }
    
    template <typename... Fs>
    constexpr auto match(Fs&&... fs)
    {
        auto f = overload {std::forward<Fs>(fs)...};
        return call_impl(f, std::make_index_sequence<sizeof...(Ts)>{});
    }

    template <typename... Fs>
    constexpr auto operator()(Fs&&... fs)
    {
        return match(std::forward<Fs>(fs)...);
    }

};

template <typename... Ts>
Matcher(Ts&&...) -> Matcher<Ts...>;

// #define match(...) Matcher(__VAR_ARGS__) ->* overload

// template <typename VariantT, typename... Fs>
// auto match(VariantT&& var, Fs&&... fs)
// {
//     return std::visit(overload{std::forward<Fs>(fs)...},
//                       std::forward<VariantT>(var));
// }

using Val = std::variant<int64_t, double>;

struct ValExpr
{
    Val val;
};

struct AddExpr;
struct SubExpr;
struct MultExpr;
struct DivExpr;
struct ModExpr;
struct PowExpr;
struct NegExpr; // negative

using Expr = std::variant<ValExpr, AddExpr, SubExpr, MultExpr,
                          DivExpr, ModExpr, PowExpr, NegExpr>;
struct AddExpr
{
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct SubExpr
{
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct MultExpr
{
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct DivExpr
{
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct ModExpr
{
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct PowExpr
{
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct NegExpr
{
    std::unique_ptr<Expr> e;
};

Val eval(Expr const& expr)
{
    return Matcher(expr)(
        [](ValExpr const& a) {
            return a.val;
        },
        
        [](AddExpr const& a) {
            return Matcher(eval(*a.lhs), eval(*a.rhs)) (
                [](auto l, auto r) { return Val(l + r); }
                );
        },
        
        [](SubExpr const& a) {
            return Matcher(eval(*a.lhs), eval(*a.rhs)) (
                [](auto l, auto r) { return Val(l - r); }
                );
        },

        [](MultExpr const& a) {
            return Matcher(eval(*a.lhs), eval(*a.rhs)) (
                [] (auto l, auto r) { return Val(l * r); }
                );
        },

        [](DivExpr const& a) {
            return Matcher(eval(*a.lhs), eval(*a.rhs)) (
                [](auto l, auto r) { return Val(l / r); }
                );
        },

        [](ModExpr const& a) {
            return Matcher(eval(*a.lhs), eval(*a.rhs)) (
                [] (int64_t l, int64_t r) {
                    return Val(l % r);
                },
                
                [] (auto l, auto r) {
                    std::cerr << "Cannot do mod on double\n";
                    std::exit(1);
                    return Val(0.0); // dummy, to supress error
                }
                );
        },

        [](PowExpr const& a) {
            return Matcher(eval(*a.lhs), eval(*a.rhs)) (
                [](int64_t l, int64_t r) {
                    if (r >= 0) {
                        return Val(powInt(l, r));
                    } else {
                        return Val(std::pow(l, r));
                    }
                },                
                [](auto l, auto r) {
                    return Val(std::pow(l, r));
                }
                );
        },

        [](NegExpr const& a) {
            return Matcher(eval(*a.e)) (
                [](auto l) { return Val(-l); }
                );
        }
        
        );
}

template <typename It>
class Parser
{
public:

    Parser(It begin, It end)
        : beg_(begin), cur_(begin), end_(end)
    {        
    }
    
    class Error
    {
        
    };
    
    std::unique_ptr<Expr> parse();
    

private:
    It beg_;
    It cur_;
    It end_;

    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parsePrefix();
    std::unique_ptr<Expr> parseAdditive();
    std::unique_ptr<Expr> parseMultiplicative();
    std::unique_ptr<Expr> parsePower();
};

template <typename It>
Parser(It, It) -> Parser<It>; // by value


int main(int argc, char* argv[])
{
    // auto e = Expr {
    //     AddExpr {
    //         std::make_unique<Expr>(ValExpr{1l}),
    //         std::make_unique<Expr>(ValExpr{2.2})
    //     }
    // };

    // Val v = eval(e);

    // Matcher(v).match(
    //     [](auto d) { std::cout << d << std::endl; }
    //     );

    if (argc <= 1) {
        std::cerr << "Too few arguments\n";
    } else if (argc >= 3) {
        std::cerr << "Too much arguments\n";
    } else {
        std::string expr = argv[1];
        try {
            auto parser = Parser(expr.cbegin(), expr.cend());
            auto expr = parser.parse();
            Matcher(eval(*expr)).match(
                [](auto i) { std::cout << i << '\n'; }
                );
        } catch (Parser<std::string::const_iterator>::Error) {
            // throw; // temporary
            std::cerr << "Invalid syntax\n";
        }
    }
}

template <typename It>
std::unique_ptr<Expr> Parser<It>::parse()
{
    auto expr = parseExpr();
    if (cur_ != end_) {
        // extra characters
        throw Error{};
    }
    return expr;
}

template <typename It>
std::unique_ptr<Expr> Parser<It>::parseExpr()
{
    if (cur_ == end_) {
        throw Error{};
    }

    return parseAdditive();
}

template <typename It>
std::unique_ptr<Expr> Parser<It>::parseAdditive()
{
    if (cur_ == end_) {
        throw Error{};
    }

    auto root = parseMultiplicative();
    while (cur_ != end_
           && (*cur_ == '+' || *cur_ == '-')) {
        char op = *cur_;
        ++cur_;

        auto rhs = parseMultiplicative();
        if (op == '+') {
            root = std::make_unique<Expr>(
                AddExpr{std::move(root), std::move(rhs)});
        } else {
            // op == '-'
            root = std::make_unique<Expr>(
                SubExpr{std::move(root), std::move(rhs)});
        }
    }
    return root;
}

template <typename It>
std::unique_ptr<Expr> Parser<It>::parseMultiplicative()
{
    if (cur_ == end_) {
        throw Error{};
    }

    auto root = parsePower();
    while (cur_ != end_ &&
           (*cur_ == '*' || *cur_ == '/' || *cur_ == '%')) {
        char op = *cur_;
        ++cur_;
        auto rhs = parsePower();
        if (op == '*') {
            root = std::make_unique<Expr>(
                MultExpr{std::move(root), std::move(rhs)}
                );
        } else if (op == '/') {
            // op == '/'
            root = std::make_unique<Expr>(
                DivExpr{std::move(root), std::move(rhs)}
                );
        } else {
            assert(op == '%');
            root = std::make_unique<Expr>(
                ModExpr{std::move(root), std::move(rhs)}
                );
        }
    }

    return root;
}

template <typename It>
std::unique_ptr<Expr> Parser<It>::parsePower()
{
    if (cur_ == end_) {
        throw Error{};
    }

    auto root = parsePrefix();
    while (cur_ != end_ && *cur_ == '^') {
        ++cur_;
        auto rhs = parsePrefix();
        root = std::make_unique<Expr>(PowExpr{
                std::move(root), std::move(rhs)
            });
    }

    return root;
}

template <typename It>
std::unique_ptr<Expr> Parser<It>::parsePrefix()
{
    if (cur_ == end_) {
        throw Error{};
    }

    if (*cur_ == '+') {
        ++cur_;
        return parsePrefix();
    } else if (*cur_ == '-') {
        ++cur_;
        return std::make_unique<Expr>(
            NegExpr{parsePrefix()}
            );
    } else {
        return parsePrimary();
    }
}

template <typename It>
std::unique_ptr<Expr> Parser<It>::parsePrimary()
{    
    if (std::isdigit(*cur_)) {
        std::string lexeme;
        bool integer = true;
        do {
            lexeme.push_back(*cur_);
            ++cur_;
        } while (cur_ != end_ && std::isdigit(*cur_));
        
        if (cur_ != end_ && *cur_ == '.') {
            integer = false;
            lexeme.push_back(*cur_);
            ++cur_;
            
            if (cur_ == end_ || !std::isdigit(*cur_)) {
                throw Error{};
            }
            do {
                lexeme.push_back(*cur_);
                ++cur_;
            } while (cur_ != end_ && std::isdigit(*cur_));
        }

        if (cur_ != end_ && *cur_ == 'e') {
            integer = false;
            lexeme.push_back(*cur_);
            ++cur_;
            if (cur_ == end_ || !std::isdigit(*cur_)) {
                throw Error{};
            }
            do {
                lexeme.push_back(*cur_);
                ++cur_;
            } while (cur_ != end_ && std::isdigit(*cur_));
        }

        if (integer) {
            return std::make_unique<Expr>(
                ValExpr{std::stol(lexeme)}
                );
        } else {
            return std::make_unique<Expr>(
                ValExpr{std::stod(lexeme)}
                );
        }
    } else if (*cur_ == '(') {
        ++cur_;
        auto expr = parseExpr();
        if (cur_ == end_ || *cur_ != ')') {
            throw Error{};
        }
        ++cur_;

        return expr;
    } else {
        throw Error{};
    }
}
