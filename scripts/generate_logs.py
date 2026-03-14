#!/usr/bin/env python3
"""
Генератор тестовых логов в формате Apache Combined Log.
Использование:
    python scripts/generate_logs.py --size-gb 1 --output data/access.log
"""

import argparse
import random
import sys
from datetime import datetime, timedelta, timezone

# ---------------------------------------------------------------------------
# Пулы данных для генерации
# ---------------------------------------------------------------------------
URLS = [
    "/", "/index.html", "/about", "/contact",
    "/api/v1/status", "/api/v1/users", "/api/v1/products",
    "/static/main.css", "/static/app.js", "/favicon.ico",
    "/images/logo.png", "/docs/getting-started", "/blog/",
    "/login", "/logout", "/dashboard", "/profile",
    "/search?q=test", "/api/v2/data", "/health",
]

STATUS_CODES = [200] * 70 + [301] * 5 + [302] * 3 + [304] * 5 + \
               [400] * 2 + [401] * 2 + [403] * 2 + [404] * 8 + \
               [500] * 2 + [502] * 1

METHODS = ["GET"] * 80 + ["POST"] * 15 + ["PUT"] * 3 + ["DELETE"] * 2

USER_AGENTS = [
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15",
    "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/115.0",
    "curl/8.5.0",
    "python-requests/2.31.0",
    "Googlebot/2.1 (+http://www.google.com/bot.html)",
]


def random_ip() -> str:
    return ".".join(str(random.randint(1, 254)) for _ in range(4))


def random_date(base: datetime) -> str:
    delta = timedelta(seconds=random.randint(0, 86400 * 30))
    d = base + delta
    tz = "-0700"
    return d.strftime(f"%d/%b/%Y:%H:%M:%S {tz}")


def generate_line(base_date: datetime) -> str:
    ip      = random_ip()
    date    = random_date(base_date)
    method  = random.choice(METHODS)
    url     = random.choice(URLS)
    status  = random.choice(STATUS_CODES)
    size    = random.randint(100, 50_000) if status not in (301, 302, 304) else 0
    referer = random.choice(["-", "http://www.example.com/", "https://google.com/"])
    ua      = random.choice(USER_AGENTS)
    return (
        f'{ip} - - [{date}] '
        f'"{method} {url} HTTP/1.1" '
        f'{status} {size} '
        f'"{referer}" "{ua}"'
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Apache log generator")
    parser.add_argument("--size-gb", type=float, default=0.01,
                        help="Target file size in GB (default: 0.01 = ~10 MB)")
    parser.add_argument("--seed", type=int,
                        help="Random seed for reproducibility")
    parser.add_argument("--output", default="data/access.log",
                        help="Output file path")
    args = parser.parse_args()

    target_bytes = int(args.size_gb * 1024 ** 3)
    base_date    = datetime(2024, 1, 1, tzinfo=timezone.utc)

    if args.seed:
        random.seed(args.seed)

    written = 0
    lines   = 0
    print(f"Generating ~{args.size_gb:.2f} GB → {args.output} ...", flush=True)

    with open(args.output, "w", encoding="utf-8", buffering=1 << 16) as f:
        while written < target_bytes:
            line = generate_line(base_date)
            f.write(line + "\n")
            written += len(line) + 1
            lines   += 1
            if lines % 500_000 == 0:
                pct = 100.0 * written / target_bytes
                print(f"  {lines:,} lines  |  {written / 1024**2:.0f} MB  ({pct:.1f}%)",
                      flush=True)

    print(f"Done. {lines:,} lines, {written / 1024**2:.1f} MB written.")


if __name__ == "__main__":
    main()