# -*- coding: utf-8 -*-
import os
import subprocess

output_file = "deps.txt"

# 获取当前目录下所有 .c 文件
c_files = [f for f in os.listdir('.') if f.endswith('.c')]

deps = []

for cfile in c_files:
    try:
        # Python2.7用check_output
        result = subprocess.check_output(
            ['gcc', '-MM', cfile],
            stderr=subprocess.STDOUT,
            universal_newlines=True
        )
        deps.append(result.strip())
    except subprocess.CalledProcessError as e:
        print("Error processing %s: %s" % (cfile, e.output))

with open(output_file, 'w') as f:
    for line in deps:
        f.write(line + '\n')

print("依赖已写入 %s" % output_file)