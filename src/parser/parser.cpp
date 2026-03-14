#include "parser.h"
#include <charconv>
#include <system_error>

namespace {

// Возвращает часть строки до символа-разделителя |delim|,
// продвигает |sv| за разделитель.
// Если разделитель не найден — возвращает string_view с data() == nullptr.
std::string_view consume_token(std::string_view& sv, char delim) {
    auto pos = sv.find(delim);
    if (pos == std::string_view::npos) return {};
    auto token = sv.substr(0, pos);
    sv.remove_prefix(pos + 1);
    return token;
}

// Возвращает содержимое между символами |open| и |close|,
// продвигает |sv| за закрывающий символ.
// Если открывающий или закрывающий символ отсутствует — возвращает nullopt.
// Known limitation: ищет первое вхождение |close|, поэтому экранированные
// кавычки внутри поля (редкость в реальных логах) приведут к неверному разбору.
std::optional<std::string_view> consume_between(std::string_view& sv,
                                                char open, char close) {
    if (sv.empty() || sv.front() != open) return std::nullopt;
    sv.remove_prefix(1);
    auto pos = sv.find(close);
    if (pos == std::string_view::npos) return std::nullopt;
    auto content = sv.substr(0, pos);
    sv.remove_prefix(pos + 1);
    return content;
}

} // namespace

// Разбирает одну строку Apache Combined Log Format.
// Использует string_view и std::from_chars — без аллокаций и без regex.
// При любом несоответствии формату возвращает std::nullopt.
std::optional<LogEntry> parse_log_line(std::string_view line) {
    // Защитное удаление \r\n: reader уже удаляет \r,
    // но при прямом вызове из тестов строка может содержать переносы
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        line.remove_suffix(1);
    if (line.empty()) return std::nullopt;

    LogEntry entry;

    // 1. IP-адрес клиента
    auto ip = consume_token(line, ' ');
    if (ip.empty()) return std::nullopt;
    entry.ip = std::string(ip);

    // 2. ident (RFC 1413) — игнорируем, обычно «-»
    if (consume_token(line, ' ').empty()) return std::nullopt;

    // 3. имя пользователя — игнорируем, обычно «-»
    if (consume_token(line, ' ').empty()) return std::nullopt;

    // 4. дата и время в квадратных скобках: [10/Oct/2000:13:55:36 -0700]
    auto time = consume_between(line, '[', ']');
    if (!time) return std::nullopt;
    entry.time = std::string(*time);
    if (line.empty() || line.front() != ' ') return std::nullopt;
    line.remove_prefix(1);

    // 5. строка запроса в кавычках: "METHOD URL PROTOCOL"
    auto request = consume_between(line, '"', '"');
    if (!request) return std::nullopt;
    if (line.empty() || line.front() != ' ') return std::nullopt;
    line.remove_prefix(1);

    auto method = consume_token(*request, ' ');
    if (method.empty()) return std::nullopt;
    entry.method = std::string(method);

    auto url = consume_token(*request, ' ');
    if (url.empty()) return std::nullopt;
    entry.url = std::string(url);

    entry.protocol = std::string(*request);  // остаток после второго пробела

    // 6. HTTP-статус — ровно три цифры
    {
        auto sv = consume_token(line, ' ');
        if (sv.empty()) return std::nullopt;
        int status = 0;
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), status);
        if (ec != std::errc{} || ptr != sv.data() + sv.size()) return std::nullopt;
        entry.status = status;
    }

    // 7. размер ответа в байтах; «-» означает 0 (нет тела ответа)
    {
        auto sv = consume_token(line, ' ');
        if (sv.empty()) return std::nullopt;
        if (sv == "-") {
            entry.bytes = 0;
        } else {
            uint64_t bytes = 0;
            auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), bytes);
            if (ec != std::errc{} || ptr != sv.data() + sv.size()) return std::nullopt;
            entry.bytes = bytes;
        }
    }

    // 8. referer в кавычках
    auto referer = consume_between(line, '"', '"');
    if (!referer) return std::nullopt;
    entry.referer = std::string(*referer);
    if (line.empty() || line.front() != ' ') return std::nullopt;
    line.remove_prefix(1);

    // 9. User-Agent в кавычках
    auto ua = consume_between(line, '"', '"');
    if (!ua) return std::nullopt;
    entry.user_agent = std::string(*ua);

    return entry;
}