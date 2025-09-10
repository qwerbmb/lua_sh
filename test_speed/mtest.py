# -*- coding: utf-8 -*-
import os
import time
import subprocess
import signal

# ====== 参数区 ======
LUA_INTERPRETER = "./lua_sh"  # lua解释器路径
LUA_CODE_FILES = [
    "shm_copy.lua",
    "shm_read.lua",
    # "shm_exist.lua",
    "tb_read.lua"
]  # lua代码文件列表
NUM_PROCESSES = 5                # 每个代码文件启动的进程数
SAMPLE_INTERVAL = 1              # 采样间隔（秒）
TOTAL_RUN_TIME = 10              # 总运行时间（秒）

def get_rss_kb(pid):
    """返回进程的RSS（KB）"""
    try:
        with open("/proc/%d/statm" % pid, "r") as f:
            parts = f.readline().split()
            rss_pages = int(parts[1])
            page_size = os.sysconf('SC_PAGE_SIZE') // 1024  # KB
            return rss_pages * page_size
    except:
        return -1

def get_pss_kb(pid):
    """返回进程的PSS（KB）"""
    pss = 0
    try:
        with open("/proc/%d/smaps" % pid, "r") as f:
            for line in f:
                if line.startswith("Pss:"):
                    pss += int(line.split()[1])
        return pss
    except:
        return -1

def start_lua_process(lua_interpreter, lua_file):
    """启动一个lua进程，返回subprocess.Popen对象"""
    return subprocess.Popen([lua_interpreter, lua_file])

def kill_process(proc):
    """杀死进程"""
    try:
        os.kill(proc.pid, signal.SIGKILL)
    except:
        pass

def monitor_processes(procs, sample_interval, total_time):
    """监控进程内存占用"""
    start_time = time.time()
    # 输出一次表头
    print("Time(s)\tAVG_RSS(KB)\tTOTAL_RSS(KB)\tAVG_PSS(KB)\tTOTAL_PSS(KB)")
    while True:
        now = time.time()
        elapsed = now - start_time
        if elapsed > total_time:
            break
        rss_list = []
        pss_list = []
        for i, proc in enumerate(procs):
            pid = proc.pid
            rss = get_rss_kb(pid)
            pss = get_pss_kb(pid)
            rss_list.append(rss)
            pss_list.append(pss)
        # 计算平均和总和
        valid_rss = [x for x in rss_list if x >= 0]
        valid_pss = [x for x in pss_list if x >= 0]
        if valid_rss:
            avg_rss = sum(valid_rss) / float(len(valid_rss))
            total_rss = sum(valid_rss)
        else:
            avg_rss = total_rss = -1
        if valid_pss:
            avg_pss = sum(valid_pss) / float(len(valid_pss))
            total_pss = sum(valid_pss)
        else:
            avg_pss = total_pss = -1
        # 只输出数据
        print("%.1f\t%.1f\t%d\t%.1f\t%d" % (
            elapsed, avg_rss, total_rss, avg_pss, total_pss
        ))
        time.sleep(sample_interval)

def main():
    for lua_file in LUA_CODE_FILES:
        print("=== Testing %s ===" % lua_file)
        procs = []
        for _ in range(NUM_PROCESSES):
            proc = start_lua_process(LUA_INTERPRETER, lua_file)
            procs.append(proc)
        monitor_processes(procs, SAMPLE_INTERVAL, TOTAL_RUN_TIME)
        # 结束所有进程
        for proc in procs:
            kill_process(proc)
        print("=== Finished %s ===\n" % lua_file)

if __name__ == "__main__":
    main()