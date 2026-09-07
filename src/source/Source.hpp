#pragma once

#include <cassert>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Rivet {
    struct SourceSpan {
        std::size_t Offset{ 0 };
        std::size_t Length{ 0 };

        [[nodiscard]] constexpr std::size_t End() const noexcept { return Offset + Length; }
        [[nodiscard]] constexpr bool Empty() const noexcept { return Length == 0; }
    };

    struct SourcePosition {
        // Positions are zero-based internally. Diagnostics rendering can +1.
        std::size_t Line{ 0 };
        std::size_t Column{ 0 };
    };

    struct SourceLoadError {
        std::filesystem::path Path;
        std::string Message;
    };

    class SourceFile final {
    public:
        SourceFile(std::filesystem::path path, std::string text);    

        [[nodiscard]] static std::expected<SourceFile, SourceLoadError> Load(const std::filesystem::path& path);

        [[nodiscard]] const std::filesystem::path& Path() const noexcept;

        [[nodiscard]] std::string_view Text() const noexcept;

        [[nodiscard]] std::string_view Text(SourceSpan span) const noexcept;

        [[nodiscard]] std::string_view LineText(std::size_t line) const noexcept;

        [[nodiscard]] SourceSpan LineSpan(std::size_t line) const noexcept;

        [[nodiscard]] SourcePosition PositionAt(std::size_t offset) const noexcept;

        [[nodiscard]] std::size_t Size() const noexcept;

        [[nodiscard]] std::size_t LineCount() const noexcept;

        [[nodiscard]] bool Contains(SourceSpan span) const noexcept;

        [[nodiscard]] SourceSpan EofSpan() const noexcept;

    private:
        void BuildLineTable();

        std::filesystem::path m_Path;
        std::string m_Text;

        // Each entry contains the byte offset at which a line begins.
        // The first entry is always zero, including for an empty file.
        std::vector<std::size_t> m_LineStarts;
    };
}