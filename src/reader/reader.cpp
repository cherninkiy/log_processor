#include "reader.h"
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