#include <gtest/gtest.h>
#include "parser/parser.h"

TEST(ParserTest, ValidLine) {
    std::string line = R"(127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.1" 200 2326 "http://referer.com/" "Mozilla/5.0 ...")";
    auto result = parse_log_line(line);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->ip, "127.0.0.1");
    EXPECT_EQ(result->time, "10/Oct/2000:13:55:36 -0700");
    EXPECT_EQ(result->method, "GET");
    EXPECT_EQ(result->url, "/apache_pb.gif");
    EXPECT_EQ(result->protocol, "HTTP/1.1");
    EXPECT_EQ(result->status, 200);
    EXPECT_EQ(result->bytes, 2326);
    EXPECT_EQ(result->referer, "http://referer.com/");
    EXPECT_EQ(result->user_agent, "Mozilla/5.0 ...");
}

TEST(ParserTest, MissingField) {
    std::string line = R"(127.0.0.1 - - [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.1" 200 2326 "http://referer.com/)"; // нет user-agent
    auto result = parse_log_line(line);
    EXPECT_FALSE(result.has_value());
}