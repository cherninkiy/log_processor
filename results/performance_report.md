# Performance Report

Generated: 2026-03-19 00:52 UTC

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

## Branch: step2-thread-manual

### 2026-03-15 02:04:20 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 410.17 ns | 408.06 ns |
| BM_Accumulate | 504.16 ns | 515.62 ns |
| BM_ProcessFile/iterations:1/real_time | 13278.92 ms | 13281.25 ms |
| BM_ReadFile/iterations:1/real_time | 3386.88 ms | 3390.62 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 10530.68 ms | 5015.62 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 44e11d260621c2d1b22764b0586d65d5e8781b16

---

### 2026-03-15 01:52:00 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 418.72 ns | 414.34 ns |
| BM_Accumulate | 534.24 ns | 531.25 ns |
| BM_ProcessFile/iterations:1/real_time | 12533.10 ms | 12500.00 ms |
| BM_ReadFile/iterations:1/real_time | 3405.76 ms | 3390.62 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 9949.52 ms | 4671.88 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 44e11d260621c2d1b22764b0586d65d5e8781b16

---

## Branch: step3-atomic-fixes

### 2026-03-19 00:51:11 (Linux)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 202.86 ns | 216.88 ns |
| BM_Accumulate | 493.85 ns | 528.00 ns |
| BM_ProcessFile/iterations:1/real_time | 9496.78 ms | 10354.74 ms |
| BM_ReadFile/iterations:1/real_time | 2652.75 ms | 2701.81 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 7620.03 ms | 2209.95 ms |
| BM_ProcessFileAtomicLocal/iterations:1/real_time | 10561.11 ms | 10452.48 ms |
| BM_ProcessFileConcurrentTBB/iterations:1/real_time | 12013.90 ms | 11410.67 ms |
| BM_ReadFileRaw/iterations:1/real_time | 501.82 ms | 494.05 ms |
| BM_ProcessFileChunked/iterations:1/real_time | 8740.49 ms | 8227.09 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
- Git commit: 5bea1a1d412c8c356ede4946c0ac4303159c9154

---

### 2026-03-19 00:35:07 (Linux)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 196.47 ns | 214.33 ns |
| BM_Accumulate | 474.83 ns | 517.99 ns |
| BM_ProcessFile/iterations:1/real_time | 33805.78 ms | 13418.11 ms |
| BM_ReadFile/iterations:1/real_time | 26524.15 ms | 5083.53 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 31399.38 ms | 4675.82 ms |
| BM_ProcessFileAtomicLocal/iterations:1/real_time | 34554.18 ms | 13729.48 ms |
| BM_ProcessFileConcurrentTBB/iterations:1/real_time | 36148.29 ms | 13407.27 ms |
| BM_ReadFileRaw/iterations:1/real_time | 5308.99 ms | 479.92 ms |
| BM_ProcessFileChunked/iterations:1/real_time | 12840.07 ms | 8400.36 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
- Git commit: 5bea1a1d412c8c356ede4946c0ac4303159c9154

---

### 2026-03-19 00:03:22 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 411.42 ns | 417.13 ns |
| BM_Accumulate | 493.23 ns | 500.00 ns |
| BM_ProcessFile/iterations:1/real_time | 12827.40 ms | 12828.12 ms |
| BM_ReadFile/iterations:1/real_time | 3326.80 ms | 3328.12 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 9942.99 ms | 4687.50 ms |
| BM_ProcessFileAtomicLocal/iterations:1/real_time | 11902.21 ms | 10859.38 ms |
| BM_ProcessFileConcurrentTBB/iterations:1/real_time | 13946.84 ms | 12812.50 ms |
| BM_ReadFileRaw/iterations:1/real_time | 484.06 ms | 468.75 ms |
| BM_ProcessFileChunked/iterations:1/real_time | 8956.51 ms | 8187.50 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: a0090d4358471f3646f14f6f2b6bcc617f25e05f

---

### 2026-03-18 23:44:08 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 418.86 ns | 414.34 ns |
| BM_Accumulate | 488.98 ns | 486.54 ns |
| BM_ProcessFile/iterations:1/real_time | 14250.52 ms | 14234.38 ms |
| BM_ReadFile/iterations:1/real_time | 3409.69 ms | 3390.62 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 10538.66 ms | 5125.00 ms |
| BM_ProcessFileAtomicLocal/iterations:1/real_time | 12579.36 ms | 11734.38 ms |
| BM_ProcessFileConcurrentTBB/iterations:1/real_time | 14806.64 ms | 13343.75 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 62196eac5ff382ec926f386c0d6a27b2f4d6188d

---

### 2026-03-18 23:29:02 (Linux)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 187.78 ns | 202.84 ns |
| BM_Accumulate | 505.41 ns | 545.91 ns |
| BM_ProcessFile/iterations:1/real_time | 36215.87 ms | 14334.06 ms |
| BM_ReadFile/iterations:1/real_time | 27291.39 ms | 5258.09 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 29572.82 ms | 4485.14 ms |
| BM_ProcessFileAtomicLocal/iterations:1/real_time | 30201.98 ms | 12703.33 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
- Git commit: 62196eac5ff382ec926f386c0d6a27b2f4d6188d

---

### 2026-03-18 23:20:48 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 409.27 ns | 408.06 ns |
| BM_Accumulate | 499.22 ns | 500.00 ns |
| BM_ProcessFile/iterations:1/real_time | 13696.86 ms | 13687.50 ms |
| BM_ReadFile/iterations:1/real_time | 3488.90 ms | 3500.00 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 11255.70 ms | 5250.00 ms |
| BM_ProcessFileAtomicLocal/iterations:1/real_time | 12622.70 ms | 11765.62 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 62196eac5ff382ec926f386c0d6a27b2f4d6188d

---

### 2026-03-18 23:10:05 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 406.62 ns | 408.06 ns |
| BM_Accumulate | 477.25 ns | 475.73 ns |
| BM_ProcessFile/iterations:1/real_time | 13324.06 ms | 13328.12 ms |
| BM_ReadFile/iterations:1/real_time | 4180.36 ms | 4109.38 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 10603.13 ms | 4984.38 ms |
| BM_ProcessFileAtomicLocal/iterations:1/real_time | 12386.55 ms | 11578.12 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 62196eac5ff382ec926f386c0d6a27b2f4d6188d

---

### 2026-03-18 23:01:08 (Linux)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 2.10 µs | 2.32 µs |
| BM_Accumulate | 930.24 ns | 1.03 µs |
| BM_ProcessFile/iterations:1/real_time | 57934.99 ms | 40423.07 ms |
| BM_ReadFile/iterations:1/real_time | 28014.28 ms | 7243.03 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 38004.95 ms | 6601.27 ms |
| BM_ProcessFileAtomicLocal/iterations:1/real_time | 41205.80 ms | 17940.93 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: debug
- Compiler: g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
- Git commit: 62196eac5ff382ec926f386c0d6a27b2f4d6188d

---

### 2026-03-18 22:38:37 (Windows)

| Benchmark | real time | cpu time |
|-----------|-----------|----------|
| BM_ParseLine | 416.02 ns | 417.13 ns |
| BM_Accumulate | 503.05 ns | 500.00 ns |
| BM_ProcessFile/iterations:1/real_time | 12644.74 ms | 12609.38 ms |
| BM_ReadFile/iterations:1/real_time | 3394.63 ms | 3390.62 ms |
| BM_ProcessFileThreaded/iterations:1/real_time | 11364.61 ms | 5531.25 ms |
| BM_ProcessFileAtomicLocal/iterations:1/real_time | 14644.16 ms | 13484.38 ms |

**Context**  
- CPUs: 16 @ 2895 MHz
- Build type: release
- Compiler: g++.exe (Rev8, Built by MSYS2 project) 15.2.0
- Git commit: 62196eac5ff382ec926f386c0d6a27b2f4d6188d

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
| step2-thread-manual | Windows | 2026-03-15 | BM_ParseLine | 410.17 ns |
| step2-thread-manual | Windows | 2026-03-15 | BM_Accumulate | 504.16 ns |
| step2-thread-manual | Windows | 2026-03-15 | BM_ProcessFile/iterations:1/real_time | 13278.92 ms |
| step2-thread-manual | Windows | 2026-03-15 | BM_ReadFile/iterations:1/real_time | 3386.88 ms |
| step2-thread-manual | Windows | 2026-03-15 | BM_ProcessFileThreaded/iterations:1/real_time | 10530.68 ms |
| step3-atomic-fixes | Linux | 2026-03-19 | BM_ParseLine | 202.86 ns |
| step3-atomic-fixes | Linux | 2026-03-19 | BM_Accumulate | 493.85 ns |
| step3-atomic-fixes | Linux | 2026-03-19 | BM_ProcessFile/iterations:1/real_time | 9496.78 ms |
| step3-atomic-fixes | Linux | 2026-03-19 | BM_ReadFile/iterations:1/real_time | 2652.75 ms |
| step3-atomic-fixes | Linux | 2026-03-19 | BM_ProcessFileThreaded/iterations:1/real_time | 7620.03 ms |
| step3-atomic-fixes | Linux | 2026-03-19 | BM_ProcessFileAtomicLocal/iterations:1/real_time | 10561.11 ms |
| step3-atomic-fixes | Linux | 2026-03-19 | BM_ProcessFileConcurrentTBB/iterations:1/real_time | 12013.90 ms |
| step3-atomic-fixes | Linux | 2026-03-19 | BM_ReadFileRaw/iterations:1/real_time | 501.82 ms |
| step3-atomic-fixes | Linux | 2026-03-19 | BM_ProcessFileChunked/iterations:1/real_time | 8740.49 ms |
