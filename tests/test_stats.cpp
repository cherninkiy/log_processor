#include <gtest/gtest.h>
#include "common/stats.h"

// Вспомогательная функция для создания stats с заданными параметрами
LogStats make_stats(uint64_t total_lines, uint64_t parsed_ok, uint64_t parse_errors, uint64_t total_bytes,
                    std::initializer_list<std::pair<int, uint64_t>> status = {},
                    std::initializer_list<std::pair<std::string, uint64_t>> ips = {},
                    std::initializer_list<std::pair<std::string, uint64_t>> urls = {}) {
    LogStats s;
    s.total_lines = total_lines;
    s.parsed_ok = parsed_ok;
    s.parse_errors = parse_errors;
    s.total_bytes = total_bytes;
    for (auto& p : status) s.status_codes[p.first] = p.second;
    for (auto& p : ips) s.ip_counts[p.first] = p.second;
    for (auto& p : urls) s.url_counts[p.first] = p.second;
    return s;
}

TEST(StatsTest, MergeEmpty) {
    LogStats a;
    LogStats b;
    a.merge(b);
    EXPECT_EQ(a.total_lines, 0);
    EXPECT_EQ(a.parsed_ok, 0);
    EXPECT_EQ(a.parse_errors, 0);
    EXPECT_EQ(a.total_bytes, 0);
    EXPECT_TRUE(a.status_codes.empty());
    EXPECT_TRUE(a.ip_counts.empty());
    EXPECT_TRUE(a.url_counts.empty());
}

TEST(StatsTest, MergeNonEmptyIntoEmpty) {
    LogStats a;
    LogStats b = make_stats(10, 8, 2, 12345,
                            {{200, 5}, {404, 3}},
                            {{"1.1.1.1", 2}, {"2.2.2.2", 1}},
                            {{"/", 4}, {"/api", 2}});
    a.merge(b);
    EXPECT_EQ(a.total_lines, 10);
    EXPECT_EQ(a.parsed_ok, 8);
    EXPECT_EQ(a.parse_errors, 2);
    EXPECT_EQ(a.total_bytes, 12345);
    EXPECT_EQ(a.status_codes.size(), 2);
    EXPECT_EQ(a.status_codes[200], 5);
    EXPECT_EQ(a.status_codes[404], 3);
    EXPECT_EQ(a.ip_counts.size(), 2);
    EXPECT_EQ(a.ip_counts["1.1.1.1"], 2);
    EXPECT_EQ(a.ip_counts["2.2.2.2"], 1);
    EXPECT_EQ(a.url_counts.size(), 2);
    EXPECT_EQ(a.url_counts["/"], 4);
    EXPECT_EQ(a.url_counts["/api"], 2);
}

TEST(StatsTest, MergeNonEmptyIntoNonEmpty) {
    LogStats a = make_stats(5, 4, 1, 500,
                            {{200, 3}, {500, 1}},
                            {{"1.1.1.1", 2}, {"3.3.3.3", 1}},
                            {{"/", 2}});
    LogStats b = make_stats(7, 6, 1, 700,
                            {{200, 4}, {404, 2}},
                            {{"1.1.1.1", 1}, {"2.2.2.2", 2}},
                            {{"/", 3}, {"/about", 1}, {"/api", 1}});
    a.merge(b);

    EXPECT_EQ(a.total_lines, 12);
    EXPECT_EQ(a.parsed_ok, 10);
    EXPECT_EQ(a.parse_errors, 2);
    EXPECT_EQ(a.total_bytes, 1200);

    EXPECT_EQ(a.status_codes.size(), 3);
    EXPECT_EQ(a.status_codes[200], 7);   // 3+4
    EXPECT_EQ(a.status_codes[500], 1);
    EXPECT_EQ(a.status_codes[404], 2);

    EXPECT_EQ(a.ip_counts.size(), 3);
    EXPECT_EQ(a.ip_counts["1.1.1.1"], 3); // 2+1
    EXPECT_EQ(a.ip_counts["3.3.3.3"], 1);
    EXPECT_EQ(a.ip_counts["2.2.2.2"], 2);

    EXPECT_EQ(a.url_counts.size(), 3);
    EXPECT_EQ(a.url_counts["/"], 5);       // 2+3
    EXPECT_EQ(a.url_counts["/about"], 1);
    EXPECT_EQ(a.url_counts["/api"], 1);    // only in b
    // "/api" не было в a, но была в b? Нет, в b только "/" и "/about". Ок.
}

TEST(StatsTest, PrintDoesNotCrash) {
    LogStats stats = make_stats(1000, 950, 50, 12345678,
                                {{200, 800}, {404, 100}, {500, 50}},
                                {{"1.1.1.1", 200}, {"2.2.2.2", 150}},
                                {{"/", 500}, {"/index.html", 200}, {"/api", 100}});
    // Просто проверяем, что print не вызывает исключений и не падает
    EXPECT_NO_THROW(stats.print("test.log", 1.234));
}