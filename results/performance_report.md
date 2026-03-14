# Performance Report

Generated: 2026-03-13 10:56 UTC

## Branch: main

### 2026-03-13 10:55:46 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 17.78 µs | 17.58 µs |
| BM_Accumulate | 355.95 ns | 359.93 ns |
| BM_ProcessFile | 1399.23 ms | 1390.62 ms |
| BM_ReadFile | 44.39 ms | 44.92 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: debug
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 4067090f290715417ffe53fded832ea8525c4937

---

### 2026-03-13 10:29:34 (Linux)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 17.70 µs | 17.70 µs |
| BM_Accumulate | 398.78 ns | 398.77 ns |
| BM_ProcessFile | 1552.77 ms | 1383.04 ms |
| BM_ReadFile | 221.68 ms | 50.14 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: debug
- Compiler: g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
- Git commit: 4067090f290715417ffe53fded832ea8525c4937

---

### 2026-03-03 15:00:02 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 4.28 µs | 4.30 µs |
| BM_Accumulate | 198.51 ns | 198.80 ns |
| BM_ProcessFile | 381.07 ms | 382.81 ms |
| BM_ReadFile | 33.17 ms | 32.81 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 4067090f290715417ffe53fded832ea8525c4937

---

### 2026-03-03 14:59:37 (Linux)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 3.83 µs | 3.83 µs |
| BM_Accumulate | 176.90 ns | 176.90 ns |
| BM_ProcessFile | 524.14 ms | 336.14 ms |
| BM_ReadFile | 224.45 ms | 35.12 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
- Git commit: 4067090f290715417ffe53fded832ea8525c4937

---

### 2026-03-03 14:45:06 (Linux)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 3.85 µs | 3.85 µs |
| BM_Accumulate | 174.48 ns | 174.48 ns |
| BM_ProcessFile | 509.92 ms | 331.74 ms |
| BM_ReadFile | 215.64 ms | 33.14 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
- Git commit: 4067090f290715417ffe53fded832ea8525c4937

---

### 2026-03-03 14:44:51 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 4.48 µs | 4.52 µs |
| BM_Accumulate | 201.34 ns | 199.50 ns |
| BM_ProcessFile | 378.75 ms | 382.81 ms |
| BM_ReadFile | 34.55 ms | 34.23 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 4067090f290715417ffe53fded832ea8525c4937

---

### 2026-03-03 14:02:05 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_Placeholder | 2.34 ns | 2.30 ns |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: debug
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 4067090f290715417ffe53fded832ea8525c4937

---

### 2026-03-03 14:00:53 (Linux)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_Placeholder | 1.22 ns | 1.22 ns |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: debug
- Compiler: g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
- Git commit: 4067090f290715417ffe53fded832ea8525c4937

---

## Summary (latest runs)

| Branch | Platform | Date | Benchmark | real time |
|--------|----------|------|-----------|-----------|
| main | Windows | 2026-03-13 | BM_ParseLine | 17.78 µs |
| main | Windows | 2026-03-13 | BM_Accumulate | 355.95 ns |
| main | Windows | 2026-03-13 | BM_ProcessFile | 1399.23 ms |
| main | Windows | 2026-03-13 | BM_ReadFile | 44.39 ms |
