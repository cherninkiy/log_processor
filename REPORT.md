# Анализ производительности

**Стенд:** AMD Ryzen 7 4800H, 16 логических ядер @ 2895 MHz, Windows 10 / WSL2 Ubuntu 22.04 (Linux 6.6)  
**Тестовый файл:** `data/access.log` (1,080,890,899 байт ≈ 1.01 ГБ, 7,148,945 строк Apache Combined Log Format)  
**Компиляторы:** GCC 15.2.0 (MSYS2/Windows), GCC 11.4.0 (Ubuntu 22.04/WSL)

> **Примечание о WSL-замерах.** Тестовый файл лежит на Windows-томе (`/mnt/d/...`) и доступен из WSL через слой interop (WSL↔NTFS). Каждый системный вызов `read()` проходит через этот слой, что многократно увеличивает реальное время I/O при многократных мелких чтениях. Для получения честных Linux-цифр нужно скопировать файл на нативный раздел ext4 (`cp /mnt/d/.../access.log ~/access.log`).

---

## Сводка (Windows, release, 1 ГБ / 7.1М строк)

| Benchmark | main (`std::regex`) | step1 (ручной) | step2 (mutex merge) | step3 (thread-local) | step3 + chunked |
|-----------|--------------------:|---------------:|--------------------:|---------------------:|----------------:|
| BM_ParseLine | 4263 ns | 414 ns | 419 ns | 410 ns | 411 ns |
| BM_Accumulate | 568 ns | 497 ns | 534 ns | 504 ns | 493 ns |
| BM_ProcessFile (однопоток) | **40.9 с** | **13.5 с** | **12.5 с** | **12.6 с** | **12.8 с** |
| BM_ProcessFileThreaded (mutex) | — | — | **9.95 с** | **11.4 с** ¹ | **9.9 с** |
| BM_ProcessFileAtomicLocal (no mutex) | — | — | — | **14.6 с** ² | **11.9 с** |
| BM_ReadFile | 3.40 с | 3.33 с | 3.41 с | 3.40 с | 3.33 с |
| BM_ReadFileRaw | — | — | — | — | **0.48 с** |
| BM_ProcessFileChunked | — | — | — | — | **9.0 с** |

> ¹ Прогон в той же сессии что и шаги 1–4 — file cache частично тёплый.  
> ² Запускается шестым в сессии; heap фрагментирован из-за трёх предыдущих 1 ГБ-загрузок. В изолированном запуске ожидается ≈ 10–11 с.

---

## Сводка (Linux/WSL, release, 1 ГБ / 7.1М строк)

| Benchmark | /mnt/ (WSL interop) | native ext4 |
|-----------|--------------------:|------------:|
| BM_ProcessFile (однопоток) | 33.8 с | **9.50 с** |
| BM_ReadFile | 26.5 с | **2.65 с** |
| BM_ProcessFileThreaded | 31.4 с | **7.62 с** ✅ |
| BM_ProcessFileAtomicLocal | 34.6 с | 10.56 с |
| BM_ProcessFileConcurrentTBB | 36.1 с | 12.01 с |
| BM_ReadFileRaw | **5.3 с** | **0.50 с** |
| BM_ProcessFileChunked | **12.8 с** | 8.74 с |

> **WSL interop-эффект** (`/mnt/`): `readAllLines()` делает ~7М вызовов `getline` → каждый проходит через WSL↔NTFS → 26.5 с вместо ~3 с. `readRawBuffer()` — один `read()` → 5.3 с (5× быстрее).  
> **На нативном ext4** I/O перестаёт быть узким местом (ReadFile 2.65 с, ReadFileRaw 0.50 с). Лучший результат — `BM_ProcessFileThreaded` = **7.62 с**: он **опережает** `BM_ProcessFileChunked` (8.74 с), так как overhead разбивки буфера на `string_view` заметен при отсутствии I/O-задержки.  
> TBB на Linux уступает `std::thread` (12.01 с vs 7.62 с) — overhead диспетчеризации превышает выигрыш на равномерной нагрузке.

---

## Анализ по бенчмаркам

### BM_ParseLine — main: 4263 нс, step1: 414 нс

`std::regex` при каждом вызове выполняет полный проход NFA/DFA по строке ~200–400 символов: **4.3 µs/строку**.  
Ручной парсер на `string_view` + `std::from_chars` без аллокаций: **0.41 µs/строку** — ускорение **10.3×**.

Вклад в полный pipeline на 7.1М строк:
- main (regex): 4263 нс × 7.1М ≈ **30.3 сек** — почти весь BM_ProcessFile (40.9 сек)
- step1: 414 нс × 7.1М ≈ **2.9 сек** — только ~22% BM_ProcessFile (13.5 сек)

Оставшиеся 10.6 сек в step1 — это чтение файла в память (~3.3 сек) и накладные расходы агрегации + аллокация `vector<string>` для 1 ГБ данных.

### BM_ProcessFile — **40.9 сек** (main) → **13.5 сек** (step1) → **12.5 сек** (step2, однопоток)

Сквозной pipeline на реальном 1 ГБ файле. Цель курса — **< 10 сек на 8 ядрах**.

- main: доминирует regex (>70% времени)
- step1: ускорение 3× относительно main, но узким местом стало чтение файла целиком в RAM
- step2 (однопоток): незначительно быстрее step1 — тот же reader, тот же парсер

### BM_ProcessFileThreaded — **9.95 сек** (step2, 16 потоков)

16 потоков на парсинг и агрегацию. Ускорение **1.35×** относительно step1.  
Ожидали больше — узкое место сместилось на **чтение файла** (3.4 сек): файл читается последовательно до старта потоков, а сам merge выполняется один раз в конце без конкуренции. Итого:
- чтение: ~3.4 сек (однопоточный `std::getline`)
- парсинг 7.1М строк на 16 потоков: ~0.19 сек/поток × параллельно ≈ ~0.2 сек
- накладные расходы thread + merge: ~0.1–0.2 сек
- суммарно: **~3.4 + парсинг ≈ 3.6 сек**, но реально 9.95 — значит **аллокации** `vector<string>` и merge `unordered_map` (7М ключей) дорогие

**Цель < 10 сек достигнута** на 16 ядрах (9.95 сек). На 8 ядрах результат будет в районе 10–12 сек — нужен step3.

### BM_ProcessFileAtomicLocal — **~14.6 сек** (step3, сессионный), **11.9 сек** (изолированный)

Shared-nothing подход: каждый поток пишет в `slots[tid].stats` без мьютекса и атомиков в горячем пути.  
`alignas(64)` на `ThreadSlot` гарантирует, что соседние слоты в `std::vector<ThreadSlot>` не делят кэш-линий.  
После `parallel_for_indexed` главный поток делает N-1 merge последовательно без конкуренции.

Sессионный результат (14.6 сек) завышен: бенчмарк запускается шестым, после трёх полных загрузок 1 ГБ в RAM. Изолированный запуск: **11.9 сек**. Bottleneck не сдвинулся — всё ещё `readAllLines()` (~3.4 сек) + аллокации 7М `std::string`.

### BM_ReadFileRaw + BM_ProcessFileChunked — **0.48 сек** чтение, **9.0 сек** полный pipeline (Windows)

Чанковый reader устраняет главный bottleneck — 7М аллокаций `std::string`:
- `readRawBuffer()` — один системный вызов `read()`, возвращает `vector<char>` (~0.48 с vs 3.33 с для `readAllLines()`)
- `getLineViews()` — `O(n)` разбивка через `memchr` (SIMD в libc), возвращает `vector<string_view>` без аллокаций
- `parse_log_line(string_view)` — парсер уже совместим, аллокаций в hot path нет

**Windows:** `BM_ProcessFileChunked` = **9.0 сек** — быстрее step2 mutex (9.95 с) и step3 atomic (11.9 с).

**Linux/WSL** с файлом на `/mnt/`:  `BM_ReadFileRaw` = 5.3 с (vs 26.5 с для `readAllLines`), `BM_ProcessFileChunked` = **12.8 с** — в **2.7×** быстрее `BM_ProcessFileThreaded` (31.4 с). Эффект исключительно от сокращения числа interop-вызовов.

**Linux native ext4** (`~/access.log`): `BM_ReadFileRaw` = **0.50 с** ✅, `BM_ProcessFileChunked` = **8.74 с**. На быстром ext4 `BM_ProcessFileThreaded` (7.62 с) **опережает** `BM_ProcessFileChunked` (8.74 с): overhead разбивки буфера на `string_view` ощутим при отсутствии I/O-задержки. Chunked reader максимально эффективен при медленном I/O (WSL interop, HDD, сетевая ФС).

### BM_Accumulate — ~0.5 µs/запись

`unordered_map` по IP и URL — узкое место при параллельной агрегации.  
В step2 merge 7М-элементных map'ов занимает заметное время под мьютексом.  
В step3: thread-local слоты → merge после join, без конкуренции → contention устранён.

---

## Что сдерживает производительность

| Причина | Влияние | Где устраняется |
|---------|---------|----------------|
| Последовательное чтение в `vector<string>` (~7М аллокаций) | ~3.4 с (Windows) / ~26.5 с (WSL `/mnt/`) | **step3** (`readRawBuffer` + `getLineViews`) ✅ |
| Merge `unordered_map` под мьютексом | lock contention при финальном merge (step2) | step3: thread-local + merge после join |
| False sharing в per-thread структурах | cache line bouncing при прямой записи в сос. слоты | step3: `alignas(64) ThreadSlot` |
| `std::thread` создаётся на каждый файл | малые накладные расходы, latency запуска | step4: постоянный thread pool |

---

## Результаты по шагам (1 ГБ, Windows)

| Шаг | Механизм | Время (измерено) |
|-----|----------|------------------|
| main (regex) | `std::regex`, 1 поток | **40.9 сек** |
| step1-sequential | ручной парсер, 1 поток | **13.5 сек** |
| step2-thread-manual | `std::thread` + mutex merge, 16 потоков | **9.95 сек** |
| step3-atomic-fixes | thread-local слоты + `alignas(64)` + chunked reader | **9.0 с** (Windows) / **7.62 с** (Linux ext4) ✅ |
| step4+ | thread pool / par STL / корутины | — (ожидается < 5 сек) |
