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

// ---------------------------------------------------------------------------
// Тесты chunked API: readRawBuffer() + getLineViews()
// ---------------------------------------------------------------------------

TEST(ReaderTest, ReadRawBufferMatchesFileSize) {
    std::string fname = "test_raw.log";
    const std::string content = "alpha\nbeta\ngamma\n";
    {
        std::ofstream out(fname, std::ios::binary);
        out << content;
    }

    LogReader reader(fname);
    auto buf = reader.readRawBuffer();
    EXPECT_EQ(buf.size(), content.size());

    std::remove(fname.c_str());
}

TEST(ReaderTest, ReadRawBufferNonexistent) {
    LogReader reader("no_such_file.log");
    auto buf = reader.readRawBuffer();
    EXPECT_TRUE(buf.empty());
}

TEST(ReaderTest, GetLineViewsUnixNewlines) {
    const std::string s = "line1\nline2\nline3\n";
    std::vector<char> buf(s.begin(), s.end());
    auto views = LogReader::getLineViews(buf);
    ASSERT_EQ(views.size(), 3u);
    EXPECT_EQ(views[0], "line1");
    EXPECT_EQ(views[1], "line2");
    EXPECT_EQ(views[2], "line3");
}

TEST(ReaderTest, GetLineViewsCRLF) {
    // Windows-style CRLF — '\r' должен быть срезан
    const std::string s = "line1\r\nline2\r\nline3\r\n";
    std::vector<char> buf(s.begin(), s.end());
    auto views = LogReader::getLineViews(buf);
    ASSERT_EQ(views.size(), 3u);
    EXPECT_EQ(views[0], "line1");
    EXPECT_EQ(views[1], "line2");
    EXPECT_EQ(views[2], "line3");
}

TEST(ReaderTest, GetLineViewsNoTrailingNewline) {
    // Файл без финального '\n' — последняя строка всё равно должна попасть в вектор
    const std::string s = "aaa\nbbb\nccc";
    std::vector<char> buf(s.begin(), s.end());
    auto views = LogReader::getLineViews(buf);
    ASSERT_EQ(views.size(), 3u);
    EXPECT_EQ(views[2], "ccc");
}

TEST(ReaderTest, GetLineViewsEmpty) {
    std::vector<char> buf;
    auto views = LogReader::getLineViews(buf);
    EXPECT_TRUE(views.empty());
}

TEST(ReaderTest, ChunkedMatchesAllLines) {
    // readRawBuffer + getLineViews должны давать те же строки, что readAllLines
    std::string fname = "test_chunked.log";
    {
        std::ofstream out(fname);
        out << "hello world\nfoo bar\nbaz\n";
    }

    LogReader reader(fname);
    auto classic = reader.readAllLines();

    auto buf   = reader.readRawBuffer();
    auto views = LogReader::getLineViews(buf);

    ASSERT_EQ(classic.size(), views.size());
    for (size_t i = 0; i < classic.size(); ++i) {
        EXPECT_EQ(classic[i], views[i]) << "Mismatch at line " << i;
    }

    std::remove(fname.c_str());
}