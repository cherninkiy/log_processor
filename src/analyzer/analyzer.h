#pragma once

#include "common/stats.h"

// Добавляет данные из распарсенной записи в общую статистику
void accumulate(LogStats& stats, const LogEntry& entry);