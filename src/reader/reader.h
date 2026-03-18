#pragma once

#include <string>
#include <string_view>
#include <vector>

class LogReader {
public:
    explicit LogReader(const std::string& filename);

    // Классический API: читает файл построчно, каждая строка — std::string (копия).
    std::vector<std::string> readAllLines();

    // Chunked API: читает весь файл одним блоком без построчного копирования.
    // readRawBuffer() возвращает сырые байты; время жизни буфера должно
    // превышать время жизни string_view-ов, возвращённых getLineViews().
    std::vector<char> readRawBuffer();

    // Разбивает буфер на строки без аллокации std::string.
    // Возвращает вектор string_view, указывающих прямо в buf.
    // Совместим с parse_log_line(string_view).
    static std::vector<std::string_view> getLineViews(const std::vector<char>& buf);

private:
    std::string filename_;
};