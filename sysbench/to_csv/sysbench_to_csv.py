import re
import sys
import csv

def parse_sysbench_fileio(content):
    data = {
        'reads_per_s': '',
        'writes_per_s': '',
        'fsyncs_per_s': '',
        'read_MBps': '',
        'written_MBps': '',
        'latency_min_ms': '',
        'latency_avg_ms': '',
        'latency_max_ms': '',
        'latency_95pct_ms': '',
        'total_time_s': '',
        'total_events': ''
    }

    # File operations
    m = re.search(r'reads/s:\s*([\d\.]+)', content)
    if m: data['reads_per_s'] = m.group(1)

    m = re.search(r'writes/s:\s*([\d\.]+)', content)
    if m: data['writes_per_s'] = m.group(1)

    m = re.search(r'fsyncs/s:\s*([\d\.]+)', content)
    if m: data['fsyncs_per_s'] = m.group(1)

    # Throughput
    m = re.search(r'read, MiB/s:\s*([\d\.]+)', content)
    if m: data['read_MBps'] = m.group(1)

    m = re.search(r'written, MiB/s:\s*([\d\.]+)', content)
    if m: data['written_MBps'] = m.group(1)

    # Latency
    m = re.search(r'Latency \(ms\):\s*min:\s*([\d\.]+)', content)
    if m: data['latency_min_ms'] = m.group(1)

    m = re.search(r'avg:\s*([\d\.]+)', content)
    if m: data['latency_avg_ms'] = m.group(1)

    m = re.search(r'max:\s*([\d\.]+)', content)
    if m: data['latency_max_ms'] = m.group(1)

    m = re.search(r'95th percentile:\s*([\d\.]+)', content)
    if m: data['latency_95pct_ms'] = m.group(1)

    # General stats
    m = re.search(r'total time:\s*([\d\.]+)s', content)
    if m: data['total_time_s'] = m.group(1)

    m = re.search(r'total number of events:\s*([\d\.]+)', content)
    if m: data['total_events'] = m.group(1)

    return data

def main():
    content = sys.stdin.read()
    data = parse_sysbench_fileio(content)

    header = [
        'reads_per_s', 'writes_per_s', 'fsyncs_per_s',
        'read_MBps', 'written_MBps',
        'latency_min_ms', 'latency_avg_ms', 'latency_max_ms', 'latency_95pct_ms',
        'total_time_s', 'total_events'
    ]

    writer = csv.DictWriter(sys.stdout, fieldnames=header)
    # writer.writeheader()
    writer.writerow(data)


if __name__ == "__main__":
    main()
