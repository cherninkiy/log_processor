#pragma once

#include <string>
#include <vector>

class LogReader {
public:
    explicit LogReader(const std::string& filename);
    std::vector<std::string> readAllLines(); // читает все строки в вектор

private:
    std::string filename_;
};