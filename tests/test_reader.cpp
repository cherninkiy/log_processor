#include <gtest/gtest.h>
#include <fstream>
#include "reader/reader.h"

TEST(ReaderTest, ReadExistingFile) {
    // Создаем временный файл с несколькими строками
    std::string test_filename = "test_reader.log";
    {
        std::ofstream out(test_filename);
        out << "line1\nline2\nline3\n";
    }

    LogReader reader(test_filename);
    auto lines = reader.readAllLines();
    EXPECT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "line1");
    EXPECT_EQ(lines[1], "line2");
    EXPECT_EQ(lines[2], "line3");

    std::remove(test_filename.c_str());
}

TEST(ReaderTest, NonexistentFile) {
    LogReader reader("nonexistent.log");
    auto lines = reader.readAllLines();
    EXPECT_TRUE(lines.empty());
}