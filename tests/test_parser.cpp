#include <gtest/gtest.h>
#include "parser/parser.h"

// Valid GET request — all fields parsed correctly
TEST(ParserTest, ValidGetRequest) {
    std::string line = R"(127.0.0.1 - frank [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.1" 200 2326 "http://referer.com/" "Mozilla/5.0 ...")";
    auto result = parse_log_line(line);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->ip,         "127.0.0.1");
    EXPECT_EQ(result->time,       "10/Oct/2000:13:55:36 -0700");
    EXPECT_EQ(result->method,     "GET");
    EXPECT_EQ(result->url,        "/apache_pb.gif");
    EXPECT_EQ(result->protocol,   "HTTP/1.1");
    EXPECT_EQ(result->status,     200);
    EXPECT_EQ(result->bytes,      2326u);
    EXPECT_EQ(result->referer,    "http://referer.com/");
    EXPECT_EQ(result->user_agent, "Mozilla/5.0 ...");
}

// bytes field is "-" — must be converted to 0
TEST(ParserTest, BytesDash) {
    std::string line = R"(10.0.0.1 - - [01/Jan/2024:00:00:00 +0000] "HEAD /ping HTTP/1.0" 204 - "-" "-")";
    auto result = parse_log_line(line);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->bytes, 0u);
    EXPECT_EQ(result->status, 204);
}

// Empty string — must return nullopt
TEST(ParserTest, EmptyLine) {
    EXPECT_FALSE(parse_log_line("").has_value());
}

// Truncated line (no closing quote for user_agent) — must return nullopt
TEST(ParserTest, TruncatedLine) {
    std::string line = R"(127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.1" 200 2326 "http://referer.com/)";
    EXPECT_FALSE(parse_log_line(line).has_value());
}

// POST request with 404 status
TEST(ParserTest, PostRequest404) {
    std::string line = R"(192.168.1.5 - - [14/Mar/2026:10:00:00 +0300] "POST /api/data HTTP/1.1" 404 512 "-" "curl/8.0")";
    auto result = parse_log_line(line);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->method,  "POST");
    EXPECT_EQ(result->url,     "/api/data");
    EXPECT_EQ(result->status,  404);
    EXPECT_EQ(result->bytes,   512u);
    EXPECT_EQ(result->referer, "-");
}

// Status field is not a 3-digit number — must return nullopt
TEST(ParserTest, InvalidStatus) {
    std::string line = R"(127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] "GET / HTTP/1.1" 20x 100 "-" "-")";
    EXPECT_FALSE(parse_log_line(line).has_value());
}

// bytes field is non-numeric and not "-" — must return nullopt
TEST(ParserTest, InvalidBytes) {
    std::string line = R"(127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] "GET / HTTP/1.1" 200 abc "-" "-")";
    EXPECT_FALSE(parse_log_line(line).has_value());
}

// URL with query string — must parse correctly
TEST(ParserTest, UrlWithQueryString) {
    std::string line = R"(127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] "GET /search?q=test&p=1 HTTP/1.1" 200 512 "-" "-")";
    auto result = parse_log_line(line);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->url, "/search?q=test&p=1");
}