import json

path = 'results/main/2026-03-15_01-22-06_benchmark.json'
with open(path) as f:
    d = json.load(f)
for b in d['benchmarks']:
    name = b['name']
    rt = b['real_time']
    ct = b['cpu_time']
    unit = b['time_unit']
    iters = b['iterations']
    print(f'{name}: real={rt:.3f} {unit}, cpu={ct:.3f} {unit}, iters={iters}')
