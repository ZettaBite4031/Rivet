#include "rir/RirVerifier.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {
    using Rivet::RirIssue;
    using Rivet::RirInstruction;
    using Rivet::RirOp;

    bool UsesLeft(RirOp op) {
        return op == RirOp::Identity || op == RirOp::Negate || op == RirOp::Not ||
            op == RirOp::Return || op == RirOp::Branch || op == RirOp::StoreLocal ||
            op == RirOp::Add || op == RirOp::Subtract || op == RirOp::Multiply ||
            op == RirOp::Divide || op == RirOp::Less || op == RirOp::LessEquals ||
            op == RirOp::Greater || op == RirOp::GreaterEquals ||
            op == RirOp::Equals || op == RirOp::NotEquals;
    }
    bool UsesRight(RirOp op) {
        return op == RirOp::Add || op == RirOp::Subtract || op == RirOp::Multiply ||
            op == RirOp::Divide || op == RirOp::Less || op == RirOp::LessEquals ||
            op == RirOp::Greater || op == RirOp::GreaterEquals ||
            op == RirOp::Equals || op == RirOp::NotEquals;
    }
    bool Defines(RirOp op) {
        return op != RirOp::Return && op != RirOp::Branch && op != RirOp::Jump &&
            op != RirOp::Label && op != RirOp::StoreLocal;
    }
    bool Terminates(RirOp op) {
        return op == RirOp::Return || op == RirOp::Jump || op == RirOp::Branch;
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

    bool ParseIndex(std::string_view text, std::string_view prefix, std::size_t& result) {
        if (!text.empty() && text.back() == ',') text.remove_suffix(1);
        if (!text.starts_with(prefix)) return false;
        text.remove_prefix(prefix.size());
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
        std::unordered_map<std::string, std::size_t> signatures;
        for (const RirFunction& function : module.Functions)
            signatures.emplace(function.Name, function.ParameterCount);
        for (const RirFunction& function : module.Functions) {
            if (!names.insert(function.Name).second)
                issues.push_back({ "ICE-RIR-004", "duplicate function '@" + function.Name + "'" });
            std::unordered_map<std::size_t, std::size_t> labels;
            for (std::size_t i = 0; i < function.Instructions.size(); ++i) {
                const auto& instruction = function.Instructions[i];
                if (instruction.Op == RirOp::Label && !labels.emplace(instruction.Target, i).second)
                    issues.push_back({ "ICE-RIR-008", "duplicate block label" });
            }
            std::unordered_map<std::size_t, RirValueType> defined;
            std::unordered_set<std::size_t> all_definitions;
            for (std::size_t i = 0; i < function.ParameterCount; ++i) {
                defined.emplace(i, RirValueType::Int);
                all_definitions.insert(i);
            }
            bool terminated = false;
            std::string block = "entry";
            for (const RirInstruction& instruction : function.Instructions) {
                if (instruction.Op == RirOp::Label) {
                    if (!terminated)
                        issues.push_back({ "ICE-RIR-001", "function '@" + function.Name
                            + "', block '" + block + "' has no terminator" });
                    defined.clear();
                    terminated = false;
                    block = "block" + std::to_string(instruction.Target);
                    continue;
                }
                if (terminated) {
                    issues.push_back({ "ICE-RIR-003", "instruction appears after terminator" });
                    break;
                }
                const auto require_value = [&](std::size_t id) -> std::optional<RirValueType> {
                    const auto found = defined.find(id);
                    if (found == defined.end()) {
                        issues.push_back({ "ICE-RIR-002", "instruction references undefined value '$"
                            + std::to_string(id) + "'" });
                        return std::nullopt;
                    }
                    return found->second;
                };
                std::optional<RirValueType> left, right;
                if (UsesLeft(instruction.Op)) left = require_value(instruction.Left);
                if (UsesRight(instruction.Op)) right = require_value(instruction.Right);
                const auto require_type = [&](std::optional<RirValueType> actual, RirValueType expected) {
                    if (actual && *actual != expected)
                        issues.push_back({ "ICE-RIR-010", "instruction operand has wrong type" });
                };
                if (instruction.Op == RirOp::Call) {
                    for (std::size_t argument : instruction.Arguments)
                        require_type(require_value(argument), RirValueType::Int);
                    const auto callee = signatures.find(instruction.Callee);
                    if (callee == signatures.end())
                        issues.push_back({ "ICE-RIR-006", "call references unknown function '@"
                            + instruction.Callee + "'" });
                    else if (callee->second != instruction.Arguments.size())
                        issues.push_back({ "ICE-RIR-007", "call to '@" + instruction.Callee
                            + "' has wrong argument count" });
                }
                if (instruction.Op == RirOp::LoadLocal || instruction.Op == RirOp::StoreLocal) {
                    if (instruction.Slot >= function.LocalTypes.size())
                        issues.push_back({ "ICE-RIR-009", "invalid local slot" });
                    else if (instruction.Op == RirOp::LoadLocal &&
                        instruction.Type != function.LocalTypes[instruction.Slot])
                        issues.push_back({ "ICE-RIR-010", "loaded local has wrong type" });
                    else if (instruction.Op == RirOp::StoreLocal)
                        require_type(left, function.LocalTypes[instruction.Slot]);
                }
                if (instruction.Op == RirOp::Return) require_type(left, RirValueType::Int);
                if (instruction.Op == RirOp::Branch) require_type(left, RirValueType::Bool);
                if (instruction.Op == RirOp::Not) require_type(left, RirValueType::Bool);
                if (instruction.Op == RirOp::Identity || instruction.Op == RirOp::Negate)
                    require_type(left, RirValueType::Int);
                if (UsesRight(instruction.Op)) {
                    const auto type = instruction.Op == RirOp::Equals || instruction.Op == RirOp::NotEquals
                        ? left.value_or(RirValueType::Int) : RirValueType::Int;
                    require_type(left, type);
                    require_type(right, type);
                }
                if (instruction.Op == RirOp::Const && instruction.Type == RirValueType::Bool &&
                    instruction.Immediate != 0 && instruction.Immediate != 1)
                    issues.push_back({ "ICE-RIR-010", "invalid bool constant" });
                const bool boolean_result = instruction.Op == RirOp::Not ||
                    instruction.Op == RirOp::Less || instruction.Op == RirOp::LessEquals ||
                    instruction.Op == RirOp::Greater || instruction.Op == RirOp::GreaterEquals ||
                    instruction.Op == RirOp::Equals || instruction.Op == RirOp::NotEquals;
                if (Defines(instruction.Op) && instruction.Op != RirOp::Const &&
                    instruction.Op != RirOp::LoadLocal &&
                    instruction.Type != (boolean_result ? RirValueType::Bool : RirValueType::Int))
                    issues.push_back({ "ICE-RIR-010", "instruction result has wrong type" });
                if (instruction.Op == RirOp::Jump || instruction.Op == RirOp::Branch) {
                    if (!labels.contains(instruction.Target) ||
                        (instruction.Op == RirOp::Branch && !labels.contains(instruction.FalseTarget)))
                        issues.push_back({ "ICE-RIR-008", "branch references missing block" });
                }
                if (Defines(instruction.Op)) {
                    if (!all_definitions.insert(instruction.Result).second)
                        issues.push_back({ "ICE-RIR-005", "value '$" + std::to_string(instruction.Result)
                            + "' is defined twice" });
                    defined.emplace(instruction.Result, instruction.Type);
                }
                terminated = Terminates(instruction.Op);
            }
            if (!terminated)
                issues.push_back({ "ICE-RIR-001", "function '@" + function.Name + "', block '"
                    + block + "' has no terminator" });
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
                if (!text_line.starts_with(prefix) || !text_line.ends_with(")"))
                    return invalid("expected function signature");
                const std::size_t open = text_line.find('(', prefix.size());
                if (open == std::string_view::npos) return invalid("expected function parameters");
                const std::string_view name = text_line.substr(prefix.size(), open - prefix.size());
                if (name.empty()) return invalid("expected function name");
                const std::string_view parameters = text_line.substr(open + 1, text_line.size() - open - 2);
                std::size_t count = 0;
                std::size_t begin = 0;
                while (begin < parameters.size()) {
                    const std::size_t comma = parameters.find(',', begin);
                    const std::string_view item = Trim(parameters.substr(begin,
                        comma == std::string_view::npos ? comma : comma - begin));
                    std::size_t register_id = 0;
                    if (!item.starts_with("int ") || !ParseRegister(item.substr(4), register_id)
                        || register_id != count) return invalid("invalid parameter register");
                    ++count;
                    if (comma == std::string_view::npos) break;
                    begin = comma + 1;
                    if (begin == parameters.size()) return invalid("trailing parameter comma");
                }
                module.Functions.push_back(RirFunction{ .Name = std::string{ name },
                    .ParameterCount = count });
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
                if (text_line.starts_with("block") && text_line.ends_with(":")) {
                    std::size_t id = 0;
                    if (!ParseIndex(text_line.substr(0, text_line.size()-1), "block", id))
                        return invalid("malformed block label");
                    module.Functions.back().Instructions.push_back(RirInstruction{.Op=RirOp::Label,.Target=id});
                    break;
                }
                std::istringstream parts{ std::string{ text_line } };
                std::string first;
                parts >> first;
                RirInstruction instruction{ .Op = RirOp::Const };
                if (first == "slot") {
                    std::string type, id, extra;
                    std::size_t slot = 0;
                    if (!(parts >> type >> id) || (parts >> extra) ||
                        !ParseIndex(id, "", slot) ||
                        (type != "int" && type != "bool") ||
                        slot != module.Functions.back().LocalTypes.size())
                        return invalid("malformed local slot");
                    module.Functions.back().LocalTypes.push_back(type == "bool" ?
                        RirValueType::Bool : RirValueType::Int);
                    break;
                }
                if (first == "return") {
                    std::string source, extra;
                    if (!(parts >> source) || (parts >> extra) || !ParseRegister(source, instruction.Left))
                        return invalid("malformed return");
                    instruction.Op = RirOp::Return;
                } else if (first == "jump") {
                    std::string target, extra;
                    if (!(parts >> target) || (parts >> extra) ||
                        !ParseIndex(target, "block", instruction.Target)) return invalid("malformed jump");
                    instruction.Op = RirOp::Jump;
                } else if (first == "branch") {
                    std::string value, yes, no, extra;
                    if (!(parts >> value >> yes >> no) || (parts >> extra) ||
                        !ParseRegister(value, instruction.Left) ||
                        !ParseIndex(yes, "block", instruction.Target) ||
                        !ParseIndex(no, "block", instruction.FalseTarget)) return invalid("malformed branch");
                    instruction.Op = RirOp::Branch;
                } else if (first == "store") {
                    std::string slot, source, extra;
                    if (!(parts >> slot >> source) || (parts >> extra) ||
                        !ParseIndex(slot, "slot", instruction.Slot) ||
                        !ParseRegister(source, instruction.Left)) return invalid("malformed store");
                    instruction.Op = RirOp::StoreLocal;
                } else {
                    std::string destination, equals, operation, first_operand, second_operand, extra;
                    if ((first != "int" && first != "bool") || !(parts >> destination >> equals >> operation >> first_operand)
                        || equals != "=" || !ParseRegister(destination, instruction.Result))
                        return invalid("malformed instruction");
                    instruction.Type = first == "bool" ? RirValueType::Bool : RirValueType::Int;
                    if (operation == "load") {
                        instruction.Op = RirOp::LoadLocal;
                        if (!ParseIndex(first_operand, "slot", instruction.Slot))
                            return invalid("invalid load slot");
                    } else if (operation == "const") {
                        instruction.Op = RirOp::Const;
                        if (instruction.Type == RirValueType::Bool &&
                            (first_operand == "true" || first_operand == "false"))
                            instruction.Immediate = first_operand == "true" ? 1 : 0;
                        else if (!ParseI32(first_operand, instruction.Immediate))
                            return invalid("invalid constant");
                    } else if (operation == "call") {
                        instruction.Op = RirOp::Call;
                        std::string rest;
                        std::getline(parts, rest);
                        const std::string call = first_operand + rest;
                        const std::string_view call_text = Trim(call);
                        const std::size_t open = call_text.find('(');
                        if (!call_text.starts_with('@') || open == std::string_view::npos
                            || !call_text.ends_with(")") || open <= 1)
                            return invalid("malformed call");
                        instruction.Callee = std::string{ call_text.substr(1, open - 1) };
                        const std::string_view arguments = call_text.substr(open + 1,
                            call_text.size() - open - 2);
                        std::size_t begin = 0;
                        while (begin < arguments.size()) {
                            const std::size_t comma = arguments.find(',', begin);
                            std::size_t argument = 0;
                            if (!ParseRegister(Trim(arguments.substr(begin,
                                    comma == std::string_view::npos ? comma : comma - begin)), argument))
                                return invalid("invalid call argument");
                            instruction.Arguments.push_back(argument);
                            if (comma == std::string_view::npos) break;
                            begin = comma + 1;
                            if (begin == arguments.size()) return invalid("trailing call comma");
                        }
                    } else {
                        if (!ParseRegister(first_operand, instruction.Left)) return invalid("invalid first operand");
                        if (operation == "identity") instruction.Op = RirOp::Identity;
                        else if (operation == "negate") instruction.Op = RirOp::Negate;
                        else if (operation == "not") instruction.Op = RirOp::Not;
                        else {
                            if (!(parts >> second_operand) || !ParseRegister(second_operand, instruction.Right))
                                return invalid("invalid second operand");
                            if (operation == "add") instruction.Op = RirOp::Add;
                            else if (operation == "subtract") instruction.Op = RirOp::Subtract;
                            else if (operation == "multiply") instruction.Op = RirOp::Multiply;
                            else if (operation == "divide") instruction.Op = RirOp::Divide;
                            else if (operation == "less") instruction.Op = RirOp::Less;
                            else if (operation == "less_equals") instruction.Op = RirOp::LessEquals;
                            else if (operation == "greater") instruction.Op = RirOp::Greater;
                            else if (operation == "greater_equals") instruction.Op = RirOp::GreaterEquals;
                            else if (operation == "equals") instruction.Op = RirOp::Equals;
                            else if (operation == "not_equals") instruction.Op = RirOp::NotEquals;
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
