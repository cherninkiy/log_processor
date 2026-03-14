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
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}