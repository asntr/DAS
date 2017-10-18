#!/usr/bin/python
import json
import sys

def get_benchmarks(filename):
    return json.load(open(filename))['benchmarks']

def get_score(results, baseline):
    score = 0
    for result, reference in zip(results, baseline):
        score += float(reference['real_time']) / result['real_time']
        if result['real_time'] > 10000:
            sys.exit(1)
    score /= len(baseline)
    return score

if __name__ == '__main__':
    print(get_score(get_benchmarks(sys.argv[1]), get_benchmarks(sys.argv[2])))
