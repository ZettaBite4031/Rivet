#include "rir/RirVerifier.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace {
    using Rivet::RirIssue;
    using Rivet::RirInstruction;
    using Rivet::RirOp;

    bool UsesLeft(RirOp op) { return op != RirOp::Const; }
    bool UsesRight(RirOp op) {
        return op == RirOp::Add || op == RirOp::Subtract || op == RirOp::Multiply || op == RirOp::Divide;
    }

    std::string_view Trim(std::string_view text) {
        const std::size_t start = text.find_first_not_of(" \t\r");
        if (start == std::string_view::npos) return {};
        const std::size_t end = text.find_last_not_of(" \t\r");
        return text.substr(start, end - start + 1);
    }

    bool ParseRegister(std::string_view text, std::size_t& result) {
        if (!text.empty() && text.back() == ',') text.remove_suffix(1);
        if (text.empty() || text.front() != '$') return false;
        text.remove_prefix(1);
        if (text.empty()) return false;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
        return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
    }

    bool ParseI32(std::string_view text, std::int32_t& result) {
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
        return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
    }
}

namespace Rivet {
    std::vector<RirIssue> VerifyRir(const RirModule& module) {
        std::vector<RirIssue> issues;
        std::unordered_set<std::string> names;
        for (const RirFunction& function : module.Functions) {
            if (!names.insert(function.Name).second)
                issues.push_back({ "ICE-RIR-004", "duplicate function '@" + function.Name + "'" });

            std::unordered_set<std::size_t> defined;
            bool terminated = false;
            for (const RirInstruction& instruction : function.Instructions) {
                if (terminated) {
                    issues.push_back({ "ICE-RIR-003", "instruction appears after terminator" });
                    break;
                }
                const auto require_value = [&](std::size_t index) {
                    if (!defined.contains(index))
                        issues.push_back({ "ICE-RIR-002", "instruction references undefined value '$"
                            + std::to_string(index) + "'" });
                };
                if (UsesLeft(instruction.Op)) require_value(instruction.Left);
                if (UsesRight(instruction.Op)) require_value(instruction.Right);
                if (instruction.Op == RirOp::Return) terminated = true;
                else if (!defined.insert(instruction.Result).second)
                    issues.push_back({ "ICE-RIR-005", "value '$" + std::to_string(instruction.Result)
                        + "' is defined twice" });
            }
            if (!terminated)
                issues.push_back({ "ICE-RIR-001", "function '@" + function.Name + "', block 'entry' has no terminator" });
        }
        return issues;
    }

    std::vector<RirIssue> ParseRirText(std::string_view text, RirModule& output) {
        RirModule module{ .Name = "<text>" };
        std::istringstream input{ std::string{ text } };
        std::string line;
        enum class State { Function, OpenBrace, Entry, Instruction };
        State state = State::Function;
        std::size_t line_number = 0;
        const auto invalid = [&](std::string message) {
            return std::vector<RirIssue>{ RirIssue{ "ICE-RIR-000", "line " + std::to_string(line_number) + ": " + std::move(message) } };
        };
        while (std::getline(input, line)) {
            ++line_number;
            const std::string_view text_line = Trim(line);
            if (text_line.empty()) continue;
            switch (state) {
            case State::Function: {
                constexpr std::string_view prefix = "function int @";
                if (!text_line.starts_with(prefix) || !text_line.ends_with("()"))
                    return invalid("expected function signature");
                const std::string_view name = text_line.substr(prefix.size(), text_line.size() - prefix.size() - 2);
                if (name.empty()) return invalid("expected function name");
                module.Functions.push_back(RirFunction{ .Name = std::string{ name } });
                state = State::OpenBrace;
                break;
            }
            case State::OpenBrace:
                if (text_line != "{") return invalid("expected '{'");
                state = State::Entry;
                break;
            case State::Entry:
                if (text_line != "entry:") return invalid("expected 'entry:'");
                state = State::Instruction;
                break;
            case State::Instruction: {
                if (text_line == "}") { state = State::Function; break; }
                std::istringstream parts{ std::string{ text_line } };
                std::string first;
                parts >> first;
                RirInstruction instruction{ .Op = RirOp::Const };
                if (first == "return") {
                    std::string source, extra;
                    if (!(parts >> source) || (parts >> extra) || !ParseRegister(source, instruction.Left))
                        return invalid("malformed return");
                    instruction.Op = RirOp::Return;
                } else {
                    std::string destination, equals, operation, first_operand, second_operand, extra;
                    if (first != "int" || !(parts >> destination >> equals >> operation >> first_operand)
                        || equals != "=" || !ParseRegister(destination, instruction.Result))
                        return invalid("malformed instruction");
                    if (operation == "const") {
                        instruction.Op = RirOp::Const;
                        if (!ParseI32(first_operand, instruction.Immediate)) return invalid("invalid int constant");
                    } else {
                        if (!ParseRegister(first_operand, instruction.Left)) return invalid("invalid first operand");
                        if (operation == "identity") instruction.Op = RirOp::Identity;
                        else if (operation == "negate") instruction.Op = RirOp::Negate;
                        else {
                            if (!(parts >> second_operand) || !ParseRegister(second_operand, instruction.Right))
                                return invalid("invalid second operand");
                            if (operation == "add") instruction.Op = RirOp::Add;
                            else if (operation == "subtract") instruction.Op = RirOp::Subtract;
                            else if (operation == "multiply") instruction.Op = RirOp::Multiply;
                            else if (operation == "divide") instruction.Op = RirOp::Divide;
                            else return invalid("unknown instruction");
                        }
                    }
                    if (parts >> extra) return invalid("extra instruction text");
                }
                module.Functions.back().Instructions.push_back(instruction);
                break;
            }
            }
        }
        if (state != State::Function) return invalid("incomplete function");
        output = std::move(module);
        return {};
    }
}
