import json, glob

for path in sorted(glob.glob('results/**/*benchmark.json', recursive=True)):
    try:
        with open(path, encoding='utf-8') as f:
            data = json.load(f)
    except Exception as e:
        print(f'SKIP {path}: {e}')
        continue
    print(f'=== {path} ===')
    for b in data['benchmarks']:
        name = b['name']
        rt = b['real_time']
        ct = b['cpu_time']
        unit = b['time_unit']
        iters = b['iterations']
        print(f'  {name}: real={rt:.3f} {unit}, cpu={ct:.3f} {unit}, iters={iters}')
    print()
