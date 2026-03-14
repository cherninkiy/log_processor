#include "parser.h"
#include <regex>
#include <string>
#include <cstdint>

namespace {

std::string_view trimCRLF(std::string_view str) {
    // Удаляем \r и \n с конца строки
    while (!str.empty() && (str.back() == '\r' || str.back() == '\n')) {
        str.remove_suffix(1);
    }
    return str;
}

} // namespace

std::optional<LogEntry> parse_log_line(std::string_view line) {
    // Регулярное выражение для Apache Combined Log
    static const std::regex pattern(R"(^(\S+) \S+ \S+ \[([^\]]+)\] \"(\S+) (\S+) ([^\"]+)\" (\d{3}) (\S+) \"([^\"]*)\" \"([^\"]*)\"$)");
    std::match_results<std::string_view::const_iterator> matches;

    line = trimCRLF(line);  // очищаем от CR/LF
    
    if (std::regex_match(line.begin(), line.end(), matches, pattern)) {
        LogEntry entry;
        entry.ip = matches[1];
        entry.time = matches[2];
        entry.method = matches[3];
        entry.url = matches[4];
        entry.protocol = matches[5];
        entry.status = std::stoi(matches[6]);
        
        std::string bytes_str = matches[7];
        entry.bytes = (bytes_str == "-") ? 0 : std::stoull(bytes_str);
        
        entry.referer = matches[8];
        entry.user_agent = matches[9];
        return entry;
    }
    return std::nullopt;
}