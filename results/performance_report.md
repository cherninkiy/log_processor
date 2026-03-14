# Performance Report

Generated: 2026-03-15 01:31 UTC

## Branch: main

### 2026-03-15 01:22:06 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 4.26 µs | 4.20 µs |
| BM_Accumulate | 567.74 ns | 565.01 ns |
| BM_ProcessFile/iterations:1/real_time | 40905.48 ms | 40906.25 ms |
| BM_ReadFile/iterations:1/real_time | 3401.58 ms | 3390.62 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 10e49d4bda3dbe469136689da54b3229707c17ca

---

### 2026-03-15 00:48:17 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 4.30 µs | 4.30 µs |
| BM_Accumulate | 475.40 ns | 475.73 ns |
| BM_ProcessFile | 41785.45 ms | 41781.25 ms |
| BM_ReadFile | 3400.81 ms | 3375.00 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: b9f23efa72522cf80134a6c22bed4dbd7ec4f43e

---

## Branch: step1-sequential

### 2026-03-15 01:28:06 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 413.73 ns | 414.34 ns |
| BM_Accumulate | 497.10 ns | 500.00 ns |
| BM_ProcessFile/iterations:1/real_time | 13515.94 ms | 13437.50 ms |
| BM_ReadFile/iterations:1/real_time | 3333.12 ms | 3343.75 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 4c3daeb1484cac37f2a87055ba34601042af1aa2

---

### 2026-03-15 00:52:36 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 416.35 ns | 414.34 ns |
| BM_Accumulate | 471.76 ns | 464.91 ns |
| BM_ProcessFile | 16427.12 ms | 16234.38 ms |
| BM_ReadFile | 3407.63 ms | 3390.62 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: aa3ca9f79e88c9e6860fffc8f9cc54776bfcca1b

---

## Summary (latest runs)

| Branch | Platform | Date | Benchmark | real time |
|--------|----------|------|-----------|-----------|
| main | Windows | 2026-03-15 | BM_ParseLine | 4.26 µs |
| main | Windows | 2026-03-15 | BM_Accumulate | 567.74 ns |
| main | Windows | 2026-03-15 | BM_ProcessFile/iterations:1/real_time | 40905.48 ms |
| main | Windows | 2026-03-15 | BM_ReadFile/iterations:1/real_time | 3401.58 ms |
| step1-sequential | Windows | 2026-03-15 | BM_ParseLine | 413.73 ns |
| step1-sequential | Windows | 2026-03-15 | BM_Accumulate | 497.10 ns |
| step1-sequential | Windows | 2026-03-15 | BM_ProcessFile/iterations:1/real_time | 13515.94 ms |
| step1-sequential | Windows | 2026-03-15 | BM_ReadFile/iterations:1/real_time | 3333.12 ms |
