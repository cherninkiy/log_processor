#!/usr/bin/env python3
"""
generate_report.py - генерация отчёта по результатам бенчмарков из папки results/

Использование:
    python scripts/generate_report.py [--results-dir RESULTS_DIR] [--output OUTPUT] [--verbose]

Пример:
    python scripts/generate_report.py --results-dir results --output results/performance_report.md --verbose
"""

import argparse
import json
import re
from datetime import datetime
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Optional

# ------------------------------------------------------------------------------
# Вспомогательные функции (определены до основного кода)
# ------------------------------------------------------------------------------
def parse_timestamp_from_filename(filename: str) -> Optional[str]:
    """Извлекает временную метку из имени файла вида YYYY-MM-DD_HH-MM-SS_*.json"""
    match = re.search(r'(\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2})', filename)
    return match.group(1) if match else None

def format_time(value: float, unit: str) -> str:
    """Форматирует время в человекочитаемый вид с учётом единицы измерения"""
    if unit == "ns":
        if value < 1000:
            return f"{value:.2f} ns"
        elif value < 1e6:
            return f"{value/1000:.2f} µs"
        else:
            return f"{value/1e6:.2f} ms"
    elif unit == "us":
        return f"{value:.2f} µs"
    elif unit == "ms":
        return f"{value:.2f} ms"
    elif unit == "s":
        return f"{value:.2f} s"
    else:
        return f"{value:.2f} {unit}"

def load_info_file(info_path: Path) -> Dict[str, str]:
    """Загружает информацию из info.txt в словарь с обработкой секций."""
    info = {}
    if not info_path.exists():
        return info

    with open(info_path, 'r', encoding='utf-8-sig') as f:
        lines = [line.rstrip('\n') for line in f]

    # Регулярные выражения для ключевых полей
    patterns = {
        'Branch': r'^Branch:\s*(.*)$',
        'Timestamp': r'^Timestamp:\s*(.*)$',
        'Test file': r'^Test file:\s*(.*)$',
    }

    # Сначала ищем простые поля ключ: значение
    for line in lines:
        for key, pattern in patterns.items():
            match = re.match(pattern, line)
            if match:
                info[key] = match.group(1).strip()
                break

    # Теперь ищем секции
    section_map = {
        '--- System ---': 'System',
        '--- Compiler ---': 'Compiler',
        '--- Git commit ---': 'Git commit'
    }

    current_section = None
    for i, line in enumerate(lines):
        line_stripped = line.strip()
        if line_stripped in section_map:
            current_section = section_map[line_stripped]
            continue
        if current_section and line_stripped and not line_stripped.startswith('---'):
            if current_section not in info:
                info[current_section] = line_stripped
                # Для системной информации берём только первую строку
                current_section = None

    # Если Compiler не найден в секциях, попробуем альтернативный вариант
    if 'Compiler' not in info:
        for line in lines:
            if 'compiler' in line.lower() and 'version' in line.lower():
                info['Compiler'] = line.strip()
                break

    # Определяем платформу по наличию "Windows" в System
    system_str = info.get('System', '')
    if 'Windows' in system_str:
        info['Platform'] = 'Windows'
    elif 'Linux' in system_str:
        info['Platform'] = 'Linux'
    else:
        info['Platform'] = 'Unknown'

    return info

# ------------------------------------------------------------------------------
# Основная функция
# ------------------------------------------------------------------------------
def generate_report(results_dir: str, output_file: str, verbose: bool = False):
    results_path = Path(results_dir)
    if not results_path.is_dir():
        print(f"Ошибка: директория {results_dir} не найдена")
        return

    if verbose:
        print(f"Scanning results directory: {results_dir}")

    # Структура: branches[branch][timestamp] = данные
    branches: Dict[str, Dict[str, Dict]] = defaultdict(dict)

    # Обход подпапок (веток)
    for branch_dir in sorted(results_path.iterdir()):
        if not branch_dir.is_dir():
            continue
        branch = branch_dir.name

        if verbose:
            print(f"\nBranch: {branch}")

        # Ищем все JSON-файлы с бенчмарками
        for json_file in branch_dir.glob("*_benchmark.json"):
            timestamp = parse_timestamp_from_filename(json_file.name)
            if not timestamp:
                if verbose:
                    print(f"  Skipping {json_file.name} (no timestamp)")
                continue

            if verbose:
                print(f"  Found benchmark: {json_file.name} (timestamp: {timestamp})")

            # Загружаем JSON
            try:
                with open(json_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)
            except Exception as e:
                print(f"  Предупреждение: не удалось прочитать {json_file}: {e}")
                continue

            # Извлекаем информацию о бенчмарках
            benchmarks = {}
            for b in data.get("benchmarks", []):
                name = b.get("name")
                if name:
                    benchmarks[name] = {
                        "real": b.get("real_time", 0),
                        "cpu": b.get("cpu_time", 0),
                        "unit": b.get("time_unit", "ns")
                    }

            # Контекст
            context = data.get("context", {})

            # Ищем соответствующий info.txt
            info_file = branch_dir / json_file.name.replace("_benchmark.json", "_info.txt")
            info = load_info_file(info_file)

            if verbose and info:
                print(f"    Info: {info.get('Compiler', '?')}, {info.get('Git commit', '?')}")

            # Сохраняем
            branches[branch][timestamp] = {
                "benchmarks": benchmarks,
                "context": context,
                "info": info,
                "platform": info.get('Platform', 'Unknown')
            }

    if not branches:
        print("Нет данных для отчёта.")
        return

    # Генерация Markdown
    with open(output_file, 'w', encoding='utf-8') as out:
        out.write("# Performance Report\n\n")
        out.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M UTC')}\n\n")

        for branch, runs in sorted(branches.items()):
            out.write(f"## Branch: {branch}\n\n")
            # Сортируем запуски по времени (новые сверху)
            for timestamp in sorted(runs.keys(), reverse=True):
                run = runs[timestamp]
                dt = datetime.strptime(timestamp, "%Y-%m-%d_%H-%M-%S").strftime("%Y-%m-%d %H:%M:%S")
                out.write(f"### {dt} ({run.get('platform', 'Unknown')})\n\n")

                # Таблица с бенчмарками
                if run["benchmarks"]:
                    out.write("| Benchmark | real time | cpu time |\n")
                    out.write("|-----------|-----------|----------|\n")
                    for name, vals in run["benchmarks"].items():
                        real = format_time(vals["real"], vals["unit"])
                        cpu = format_time(vals["cpu"], vals["unit"])
                        out.write(f"| {name} | {real} | {cpu} |\n")
                else:
                    out.write("_Нет данных о бенчмарках._\n")

                out.write("\n**Context**  \n")
                ctx = run["context"]
                out.write(f"- CPUs: {ctx.get('num_cpus', '?')} @ {ctx.get('mhz_per_cpu', '?')} MHz\n")
                out.write(f"- Build type: {ctx.get('library_build_type', '?')}\n")
                if run["info"]:
                    compiler = run["info"].get('Compiler', '?')
                    git_commit = run["info"].get('Git commit', '?')
                    out.write(f"- Compiler: {compiler}\n")
                    out.write(f"- Git commit: {git_commit}\n")
                out.write("\n---\n\n")

        # Сводная таблица по последним запускам каждой ветки
        out.write("## Summary (latest runs)\n\n")
        out.write("| Branch | Platform | Date | Benchmark | real time |\n")
        out.write("|--------|----------|------|-----------|-----------|\n")
        for branch, runs in sorted(branches.items()):
            if not runs:
                continue
            latest_ts = max(runs.keys())
            latest = runs[latest_ts]
            dt = datetime.strptime(latest_ts, "%Y-%m-%d_%H-%M-%S").strftime("%Y-%m-%d")
            platform = latest.get('platform', 'Unknown')
            for name, vals in latest["benchmarks"].items():
                real = format_time(vals["real"], vals["unit"])
                out.write(f"| {branch} | {platform} | {dt} | {name} | {real} |\n")

    print(f"Отчёт сохранён в {output_file}")

# ------------------------------------------------------------------------------
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Генерация отчёта по результатам бенчмарков")
    parser.add_argument("--results-dir", default="results", help="Директория с результатами (по умолчанию results)")
    parser.add_argument("--output", default="results/performance_report.md", help="Выходной файл отчёта")
    parser.add_argument("--verbose", action="store_true", help="Подробный вывод списка обрабатываемых файлов")
    args = parser.parse_args()
    generate_report(args.results_dir, args.output, args.verbose)