#pragma once

#include <optional>
#include <string_view>

#include "common/stats.h"

// Парсит строку лога, возвращает LogEntry или std::nullopt в случае ошибки
std::optional<LogEntry> parse_log_line(std::string_view line);