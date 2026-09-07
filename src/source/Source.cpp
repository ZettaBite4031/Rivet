#include "source/Source.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>

namespace Rivet {
    SourceFile::SourceFile(std::filesystem::path path, std::string text)
        : m_Path(std::move(path)), m_Text(std::move(text)) { BuildLineTable(); }

    std::expected<SourceFile, SourceLoadError> SourceFile::Load(const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open()) 
            return std::unexpected(SourceLoadError{ .Path = path, .Message = "could not open source file" });

        std::string text{ std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{} };
        if (stream.bad()) 
            return std::unexpected(SourceLoadError{ .Path = path, .Message = "failed while reading source file" });

        return SourceFile(path, std::move(text));
    }

    const std::filesystem::path& SourceFile::Path() const noexcept { return m_Path; }
    std::string_view SourceFile::Text() const noexcept { return m_Text; }

    std::string_view SourceFile::Text(SourceSpan span) const noexcept {
        assert(Contains(span));
        return std::string_view{ m_Text }.substr(span.Offset, span.Length);
    }

    std::string_view SourceFile::LineText(std::size_t line) const noexcept { return Text(LineSpan(line)); }

    SourceSpan SourceFile::LineSpan(std::size_t line) const noexcept { 
        assert(line < m_LineStarts.size());
        
        const std::size_t beginning = m_LineStarts[line];

        std::size_t ending = line + 1 < m_LineStarts.size() ? m_LineStarts[line + 1] : m_Text.size();

        // Exclude the line ending from the returned text. This handles both
        // LF and CRLF while retaining byte-accurate offsets elsewhere.
        if (ending > beginning && m_Text[ending - 1] == '\n') ending--;
        if (ending > beginning && m_Text[ending - 1] == '\r') ending--;

        return SourceSpan{ .Offset = beginning, .Length = ending - beginning };
    }

    SourcePosition SourceFile::PositionAt(std::size_t offset) const noexcept {
        assert(offset <= m_Text.size());

        // Find the first line beginning greater than the requested offset.
        const auto next_line = std::upper_bound(m_LineStarts.begin(), m_LineStarts.end(), offset);
        const auto current_line = std::prev(next_line);
        const std::size_t line = static_cast<std::size_t>(std::distance(m_LineStarts.begin(), current_line));
        return SourcePosition{ .Line = line, .Column = offset - *current_line };
    }

    std::size_t SourceFile::Size() const noexcept { return m_Text.size(); }
    std::size_t SourceFile::LineCount() const noexcept { return m_LineStarts.size(); }

    bool SourceFile::Contains(SourceSpan span) const noexcept {
        // Written this way instead of checking span.End() so a malicious or
        // buggy span cannot wrap around size_t during addition.
        return span.Offset <= m_Text.size() && span.Length <= m_Text.size() - span.Offset;
    }

    SourceSpan SourceFile::EofSpan() const noexcept {
        return SourceSpan{ .Offset = m_Text.size(), .Length = 0 };
    }

    void SourceFile::BuildLineTable() {
        m_LineStarts.clear();
        m_LineStarts.push_back(0);

        for (std::size_t offset = 0; offset < m_Text.size(); offset++)
            if (m_Text[offset] == '\n') m_LineStarts.push_back(offset + 1);
    }
}