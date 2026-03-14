#include <gtest/gtest.h>
#include "analyzer/analyzer.h"

TEST(AnalyzerTest, AccumulateUpdatesStats) {
    LogStats stats;
    LogEntry entry;
    entry.ip = "127.0.0.1";
    entry.time = "now";
    entry.method = "GET";
    entry.url = "/index";
    entry.protocol = "HTTP/1.1";
    entry.status = 200;
    entry.bytes = 100;
    entry.referer = "-";
    entry.user_agent = "agent";

    accumulate(stats, entry);
    EXPECT_EQ(stats.total_lines, 0);
    EXPECT_EQ(stats.total_bytes, 100);
    EXPECT_EQ(stats.status_codes[200], 1);
    EXPECT_EQ(stats.ip_counts["127.0.0.1"], 1);
    EXPECT_EQ(stats.url_counts["/index"], 1);
}
