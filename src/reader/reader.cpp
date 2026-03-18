#include "reader.h"
#include <cstring>
#include <fstream>
#include <iostream>

LogReader::LogReader(const std::string& filename) : filename_(filename) {}

std::vector<std::string> LogReader::readAllLines() {
    std::vector<std::string> lines;
    std::ifstream file(filename_);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename_ << std::endl;
        return lines;
    }
    std::string line;
    line.reserve(512);  // типичная длина строки Apache CLF — избегаем лишних перевыделений
    while (std::getline(file, line)) {
        // std::getline убирает '\n', но на Windows файлы содержат "\r\n" —
        // удаляем '\r' вручную, чтобы парсер не видел мусорный символ
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

std::vector<char> LogReader::readRawBuffer() {
    // Открываем в бинарном режиме и сразу ищем конец файла, чтобы
    // выделить буфер точного размера и прочитать всё за один вызов read().
    std::ifstream file(filename_, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename_ << std::endl;
        return {};
    }
    const auto size = static_cast<std::streamsize>(file.tellg());
    if (size <= 0) return {};
    file.seekg(0);
    std::vector<char> buf(static_cast<size_t>(size));
    file.read(buf.data(), size);
    return buf;
}

std::vector<std::string_view> LogReader::getLineViews(const std::vector<char>& buf) {
    std::vector<std::string_view> views;
    if (buf.empty()) return views;
    // Типичная длина строки Apache CLF ~170 байт; резервируем с запасом.
    views.reserve(buf.size() / 128);

    const char* data  = buf.data();
    const char* end   = data + buf.size();
    const char* start = data;

    while (start < end) {
        // memchr значительно быстрее посимвольного поиска (SIMD в libc)
        const char* nl = static_cast<const char*>(std::memchr(start, '\n', static_cast<size_t>(end - start)));
        const char* line_end = nl ? nl : end;
        // Убираем '\r' перед '\n' (Windows CRLF в бинарном режиме)
        if (line_end > start && *(line_end - 1) == '\r') --line_end;
        views.emplace_back(start, static_cast<size_t>(line_end - start));
        start = nl ? nl + 1 : end;
    }
    return views;
}