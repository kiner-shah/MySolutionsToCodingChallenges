#include <iostream>
#include <vector>
#include <stack>
#include <optional>
#include <variant>
#include <tuple>
#include <cmath>
#include <charconv>

namespace
{
// WARNING: do not change the order of this enum elements, it will affect the
// way expression evaluation happens and can lead to unexpected results
enum class TokenType : std::uint8_t
{
    Operand,
    BinaryOperator,
    UnaryOperator,
    Function,
    LeftParentheses,
    RightParentheses
};

std::string token_type_to_str(TokenType t)
{
    switch (t)
    {
        case TokenType::Operand: return "Operand";
        case TokenType::BinaryOperator: return "BinaryOperator";
        case TokenType::UnaryOperator: return "UnaryOperator";
        case TokenType::Function: return "Function";
        case TokenType::LeftParentheses: return "LeftParentheses";
        case TokenType::RightParentheses: return "RightParentheses";
        default: return "Unknown";
    }
}

enum class AssociativityType : std::uint8_t
{
    Left,
    Right,
    None
};

std::string associativity_type_to_str(AssociativityType t)
{
    switch (t)
    {
        case AssociativityType::Left: return "Left";
        case AssociativityType::Right: return "Right";
        case AssociativityType::None: return "None";
        default: return "Unknown";
    }
}

// WARNING: do not change the order of this enum elements, it will affect the
// way expression evaluation happens and can lead to unexpected results
enum class BinaryOperatorType : std::uint8_t
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Exponent
};

int get_precedence_binary_operator(BinaryOperatorType op)
{
    switch (op)
    {
        case BinaryOperatorType::Add:
        case BinaryOperatorType::Subtract:
            return 1;
        case BinaryOperatorType::Multiply:
        case BinaryOperatorType::Divide:
            return 2;
        case BinaryOperatorType::Exponent:
            return 3;
        default:
            return -1;
    }
}

std::string binary_operator_type_to_str(BinaryOperatorType t)
{
    switch (t)
    {
        case BinaryOperatorType::Add: return "+";
        case BinaryOperatorType::Subtract: return "-";
        case BinaryOperatorType::Multiply: return "*";
        case BinaryOperatorType::Divide: return "/";
        case BinaryOperatorType::Exponent: return "^";
        default: return "Unknown";
    }
}

enum class UnaryOperatorType : std::uint8_t
{
    Negate
};

std::string unary_operator_type_to_str(UnaryOperatorType t)
{
    switch (t)
    {
        case UnaryOperatorType::Negate: return "-";
        default: return "Unknown";
    }
}

enum class FunctionType : std::uint8_t
{
    Sine,
    Cosine,
    Tangent,
    Logarithm,
    Exponentiation
};

std::string function_type_to_str(FunctionType t)
{
    switch (t)
    {
        case FunctionType::Sine: return "sin";
        case FunctionType::Cosine: return "cos";
        case FunctionType::Tangent: return "tan";
        case FunctionType::Logarithm: return "log";
        case FunctionType::Exponentiation: return "exp";
        default: return "Unknown";
    }
}

struct Token
{
    using ValueType = std::variant<std::string_view, BinaryOperatorType, UnaryOperatorType, FunctionType>;

    TokenType m_type;
    AssociativityType m_associativity;
    std::optional<ValueType> m_value;

    explicit Token(TokenType type, AssociativityType associativity, std::optional<ValueType> value = std::nullopt)
        : m_type{type}, m_associativity{associativity}, m_value{std::move(value)}
    {
    }

    friend std::ostream& operator<<(std::ostream& os, const Token& token)
    {
        os << token_type_to_str(token.m_type) << ' ' << associativity_type_to_str(token.m_associativity);
        if (token.m_value)
        {
            auto val = token.m_value.value();
            os << ' ';
            if (std::holds_alternative<std::string_view>(val))
            {
                os << std::get<std::string_view>(val);
            }
            else if (std::holds_alternative<BinaryOperatorType>(val))
            {
                os << binary_operator_type_to_str(std::get<BinaryOperatorType>(val));
            }
            else if (std::holds_alternative<UnaryOperatorType>(val))
            {
                os << unary_operator_type_to_str(std::get<UnaryOperatorType>(val));
            }
            else if (std::holds_alternative<FunctionType>(val))
            {
                os << function_type_to_str(std::get<FunctionType>(val));
            }
        }
        return os;
    }
};

void print_usage(std::string program_name)
{
    std::cerr << "Usage: " << program_name << " '<expression>'\n";
}

std::tuple<std::size_t, std::size_t> parse_integer_part(std::string_view input, std::size_t index)
{
    if (input[index] == '0')
    {
        return std::make_tuple(index, 1u);
    }
    else if (input[index] >= '1' && input[index] <= '9')
    {
        std::size_t start = index;
        index++;
        while (index < input.size() && std::isdigit(input[index]))
        {
            index++;
        }
        std::size_t len = index - start;
        return std::make_tuple(start, len);
    }
    else
    {
        return std::make_tuple(0, 0);
    }
}

std::tuple<std::size_t, std::size_t> parse_fractional_part(std::string_view input, std::size_t index)
{
    std::size_t start = index;
    while (index < input.size() && std::isdigit(input[index]))
    {
        index++;
    }
    std::size_t len = index - start;
    return std::make_tuple(start, len);
}

std::tuple<std::size_t, std::size_t> parse_exponent_part(std::string_view input, std::size_t index)
{
    std::size_t start = index;
    if (index < input.size() && (input[index] == '-' || input[index] == '+'))
    {
        index++;
    }

    if (index < input.size() && std::isdigit(input[index]))
    {
        index++;
    }
    else
    {
        return std::make_tuple(0, 0);
    }
    while (index < input.size() && std::isdigit(input[index]))
    {
        index++;
    }
    std::size_t len = index - start;
    return std::make_tuple(start, len);
}

std::tuple<std::size_t, std::size_t> parse_number(std::string_view input, std::size_t index)
{
    auto [istart, ilen] = parse_integer_part(input, index);
    if (ilen == 0)
    {
        return std::make_tuple(0, 0);
    }
    if (istart + ilen < input.size() && input[istart + ilen] == '.')
    {
        auto [fstart, flen] = parse_fractional_part(input, istart + ilen + 1);
        if (flen == 0)
        {
            return std::make_tuple(istart, ilen);
        }
        if (fstart + flen < input.size() && (input.at(fstart + flen) == 'e' || input.at(fstart + flen) == 'E'))
        {
            auto [estart, elen] = parse_exponent_part(input, fstart + flen + 1);
            if (elen == 0)
            {
                return std::make_tuple(istart, fstart + flen - istart);
            }
            return std::make_tuple(istart, estart + elen - istart);
        }
        return std::make_tuple(istart, fstart + flen - istart);
    }
    else
    {
        return std::make_tuple(istart, ilen);
    }
}

bool check_subtring(std::string_view input, std::size_t start_index, std::string_view target)
{
    try
    {
        if (input.substr(start_index, target.length()) != target)
        {
            std::cerr << "Error: invalid sequence encountered during tokenization\n";
            return false;
        }
    }
    catch(const std::out_of_range& e)
    {
        std::cerr << "Error: out of range exception encountered during tokenization" << '\n';
        return false;
    }
    return true;
}

std::vector<Token> tokenize(std::string_view input_expression)
{
    std::vector<Token> tokens;
    for (std::size_t index = 0; index < input_expression.size(); index++)
    {
        char c = input_expression[index];
        if (std::isspace(c))
        {
            continue;
        }
        switch (c)
        {
            case '(':
                tokens.emplace_back(TokenType::LeftParentheses, AssociativityType::None);
                break;
            case ')':
                tokens.emplace_back(TokenType::RightParentheses, AssociativityType::None);
                break;
            case '+':
                // check for unary +
                if (index == 0 || (!tokens.empty() && (tokens.back().m_type == TokenType::BinaryOperator || tokens.back().m_type == TokenType::LeftParentheses)))
                {
                    // Unary + operator detected, can ignore it since its +
                    continue;
                }
                else
                {
                    tokens.emplace_back(TokenType::BinaryOperator, AssociativityType::Left, BinaryOperatorType::Add);
                }
                break;
            case '-':
                // check for unary -
                if (index == 0 || (!tokens.empty() && (tokens.back().m_type == TokenType::BinaryOperator || tokens.back().m_type == TokenType::LeftParentheses)))
                {
                    // Unary - operator detected
                    tokens.emplace_back(TokenType::UnaryOperator, AssociativityType::Right, UnaryOperatorType::Negate);
                }
                else
                {
                    tokens.emplace_back(TokenType::BinaryOperator, AssociativityType::Left, BinaryOperatorType::Subtract);
                }
                break;
            case '*':
                tokens.emplace_back(TokenType::BinaryOperator, AssociativityType::Left, BinaryOperatorType::Multiply);
                break;
            case '/':
                tokens.emplace_back(TokenType::BinaryOperator, AssociativityType::Left, BinaryOperatorType::Divide);
                break;
            case '^':
                tokens.emplace_back(TokenType::BinaryOperator, AssociativityType::Right, BinaryOperatorType::Exponent);
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                // parse number (integer, fraction, exponent)
                auto [start_index, length] = parse_number(input_expression, index);
                tokens.emplace_back(TokenType::Operand, AssociativityType::None, input_expression.substr(start_index, length));
                index += length - 1;
                break;
            }
            case 's':
                // parse sin
                if (!check_subtring(input_expression, index, "sin"))
                {
                    tokens.clear();
                    return tokens;
                }
                tokens.emplace_back(TokenType::Function, AssociativityType::None, FunctionType::Sine);
                index += 2;
                break;
            case 'c':
                // parse cos
                if (!check_subtring(input_expression, index, "cos"))
                {
                    tokens.clear();
                    return tokens;
                }
                tokens.emplace_back(TokenType::Function, AssociativityType::None, FunctionType::Cosine);
                index += 2;
                break;
            case 't':
                // parse tan
                if (!check_subtring(input_expression, index, "tan"))
                {
                    tokens.clear();
                    return tokens;
                }
                tokens.emplace_back(TokenType::Function, AssociativityType::None, FunctionType::Tangent);
                index += 2;
                break;
            case 'l':
                // parse log
                if (!check_subtring(input_expression, index, "log"))
                {
                    tokens.clear();
                    return tokens;
                }
                tokens.emplace_back(TokenType::Function, AssociativityType::None, FunctionType::Logarithm);
                index += 2;
                break;
            case 'e':
                // parse exp
                if (!check_subtring(input_expression, index, "exp"))
                {
                    tokens.clear();
                    return tokens;
                }
                tokens.emplace_back(TokenType::Function, AssociativityType::None, FunctionType::Exponentiation);
                index += 2;
                break;
            default:
                std::cerr << "Error: invalid character encountered during tokenization\n";
                tokens.clear();
                return tokens;
        }
    }
    return tokens;
}

template<class EnumClass>
constexpr std::underlying_type_t<EnumClass> to_underlying(EnumClass value)
{
    return static_cast<std::underlying_type_t<EnumClass>>(value);
}

std::vector<Token> infix_to_postfix(std::vector<Token>&& tokens)
{
    std::vector<Token> output;
    std::stack<Token> s;
    for (const Token& token : tokens)
    {
        switch (token.m_type)
        {
            case TokenType::Operand:
                output.push_back(std::move(token));
                break;
            case TokenType::Function:
                s.push(std::move(token));
                break;
            case TokenType::UnaryOperator:
            case TokenType::BinaryOperator:
            {
                while (!s.empty())
                {
                    Token stack_token = s.top();

                    if (stack_token.m_type == TokenType::UnaryOperator || stack_token.m_type == TokenType::BinaryOperator)
                    {
                        bool should_push_stack_token = false;
                        if (stack_token.m_type == TokenType::UnaryOperator && token.m_type == TokenType::BinaryOperator)
                        {
                            should_push_stack_token = true;
                        }
                        else if (stack_token.m_type == TokenType::BinaryOperator && token.m_type == TokenType::BinaryOperator)
                        {
                            if (!stack_token.m_value || !token.m_value)
                            {
                                std::cerr << "Unexpected situation: value is absent for binary operator\n";
                                return std::vector<Token>{};
                            }
                            // Note: assuming binary operator will have value of type BinaryOperatorType
                            auto stack_token_precedence = get_precedence_binary_operator(std::get<BinaryOperatorType>(stack_token.m_value.value()));
                            auto token_precedence = get_precedence_binary_operator(std::get<BinaryOperatorType>(token.m_value.value()));
                            if (stack_token_precedence > token_precedence
                                || (stack_token_precedence == token_precedence && token.m_associativity == AssociativityType::Left))
                            {
                                should_push_stack_token = true;
                            }
                        }
                        
                        if (!should_push_stack_token)
                        {
                            break;
                        }
                        s.pop();
                        output.push_back(std::move(stack_token));
                    }
                    else
                    {
                        break;
                    }
                }
                s.push(std::move(token));
                break;
            }
            case TokenType::LeftParentheses:
                s.push(std::move(token));
                break;
            case TokenType::RightParentheses:
            {
                bool left_parentheses_found = false;
                while (!s.empty())
                {
                    Token stack_token = s.top();
                    s.pop();

                    if (stack_token.m_type == TokenType::LeftParentheses)
                    {
                        left_parentheses_found = true;
                        break;
                    }
                    output.push_back(std::move(stack_token));
                }
                if (!left_parentheses_found)
                {
                    std::cerr << "Error: missing ')'\n";
                    return std::vector<Token>{};
                }
                if (!s.empty() && s.top().m_type == TokenType::Function)
                {
                    Token stack_token = s.top();
                    s.pop();
                    output.push_back(std::move(stack_token));
                }
                break;
            }
            default:
                std::cerr << "Error: invalid token type\n";
                break;
        }
    }
    while (!s.empty())
    {
        Token stack_token = s.top();
        s.pop();
        output.push_back(std::move(stack_token));
    }
    return output;
}

struct TreeNode
{
    Token m_token;
    TreeNode* m_left;
    TreeNode* m_right;

    explicit TreeNode(Token token, TreeNode* left = nullptr, TreeNode* right = nullptr)
        : m_token{std::move(token)}, m_left{left}, m_right{right}
    {
    }
};

constexpr double pi_value = 3.1415926535897932384626433832795;
double to_radians(double degrees)
{
    return (pi_value * degrees) / 180.0;
}

double evaluate_binary_operator(double value1, double value2, BinaryOperatorType op)
{
    switch (op)
    {
        case BinaryOperatorType::Add: return value1 + value2;
        case BinaryOperatorType::Subtract: return value1 - value2;
        case BinaryOperatorType::Multiply: return value1 * value2;
        case BinaryOperatorType::Divide: return value1 / value2;
        case BinaryOperatorType::Exponent: return std::pow(value1, value2);
        default: return 0.0;
    }
}

double evaluate_unary_operator(double value, UnaryOperatorType op)
{
    switch (op)
    {
        case UnaryOperatorType::Negate: return -value;
        default: return 0.0;
    }
}

double evaluate_function(double value, FunctionType fn)
{
    switch (fn)
    {
        case FunctionType::Sine: return std::sin(to_radians(value));
        case FunctionType::Cosine: return std::cos(to_radians(value));
        case FunctionType::Tangent: return std::tan(to_radians(value));
        case FunctionType::Logarithm: return std::log(value);
        case FunctionType::Exponentiation: return std::exp(value);
        default: return 0.0;
    }
}

class Tree
{
    TreeNode* m_root;

    TreeNode* destroy(TreeNode* node)
    {
        if (!node)
        {
            return nullptr;
        }
        if (node->m_left)
        {
            node->m_left = destroy(node->m_left);
        }
        if (node->m_right)
        {
            node->m_right = destroy(node->m_right);
        }
        if (node->m_left == nullptr && node->m_right == nullptr)
        {
            delete node;
            node = nullptr;
        }
        return node;
    }

    double evaluate(TreeNode* node)
    {
        if (!node)
        {
            return 0.0;
        }
        if (node->m_left == nullptr && node->m_right == nullptr)
        {
            auto val_str = std::get<std::string_view>(node->m_token.m_value.value());
            double val = 0.0;
            std::from_chars(val_str.data(), val_str.data() + val_str.size(), val);
            return val;
        }
        switch (node->m_token.m_type)
        {
            case TokenType::BinaryOperator:
            {
                double operand1 = evaluate(node->m_left);
                double operand2 = evaluate(node->m_right);
                double result = evaluate_binary_operator(operand1, operand2, std::get<BinaryOperatorType>(node->m_token.m_value.value()));
                return result;
            }
            case TokenType::UnaryOperator:
            {
                double operand = evaluate(node->m_left);
                double result = evaluate_unary_operator(operand, std::get<UnaryOperatorType>(node->m_token.m_value.value()));
                return result;
            }
            case TokenType::Function:
            {
                double operand = evaluate(node->m_left);
                double result = evaluate_function(operand, std::get<FunctionType>(node->m_token.m_value.value()));
                return result;
            }
            case TokenType::Operand:
            case TokenType::LeftParentheses:
            case TokenType::RightParentheses:
            default:
                return 0.0;
        }
    }
public:
    explicit Tree(std::vector<Token>&& tokens)
    {
        std::stack<TreeNode*> s;
        auto cleanup_stack = [this, &s]()
        {
            while (!s.empty())
            {
                auto node = s.top();
                s.pop();
                node = destroy(node);
            }
        };

        for (const auto& token : tokens)
        {
            switch (token.m_type)
            {
                case TokenType::BinaryOperator:
                {
                    if (s.size() < 2)
                    {
                        cleanup_stack();
                        throw std::length_error("Invalid expression - expected two operands for binary operation");
                    }
                    auto right_operand = s.top();
                    s.pop();
                    auto left_operand = s.top();
                    s.pop();
                    TreeNode* node = new TreeNode(std::move(token), left_operand, right_operand);
                    s.push(node);
                    break;
                }
                case TokenType::UnaryOperator:
                case TokenType::Function:
                {
                    if (s.size() < 1)
                    {
                        cleanup_stack();
                        throw std::length_error("Invalid expression - expected one operand for function/unary operator");
                    }
                    auto operand = s.top();
                    s.pop();
                    TreeNode* node = new TreeNode(std::move(token), operand, nullptr);
                    s.push(node);
                    break;
                }
                case TokenType::Operand:
                case TokenType::LeftParentheses:    // case won't occur
                case TokenType::RightParentheses:   // case won't occur
                default:
                    s.push(new TreeNode(std::move(token), nullptr, nullptr));
                    break;
            }
        }
        if (s.size() != 1)
        {
            cleanup_stack();
            throw std::length_error("Expected stack to contain only one element");
        }
        m_root = s.top();
        s.pop();
    }

    ~Tree()
    {
        m_root = destroy(m_root);
    }

    std::optional<double> evaluate()
    {
        if (!m_root)
        {
            return std::nullopt;
        }
        return evaluate(m_root);
    }
};

std::optional<double> evaluate(std::vector<Token>&& tokens)
{
    std::size_t tokens_size = tokens.size();
    auto postfix_token_sequence = infix_to_postfix(std::move(tokens));
    if (tokens_size > 0 && postfix_token_sequence.empty())
    {
        return std::nullopt;
    }
    try
    {
        Tree evaluation_tree(std::move(postfix_token_sequence));
        std::optional<double> result = evaluation_tree.evaluate();
        return result;
    }
    catch(const std::length_error& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return std::nullopt;
    }
}
}   // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        print_usage(argv[0]);
        return 1;
    }
    std::string_view input_expression{argv[1]};
    std::cout << "Input expression: " << input_expression << '\n';

    auto tokens = tokenize(input_expression);

    std::optional<double> result = evaluate(std::move(tokens));
    if (result)
    {
        std::cout << "Result: " << result.value() << '\n';
    }
}