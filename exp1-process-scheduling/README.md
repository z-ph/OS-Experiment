# 实验一：基于 Kylin OS 的进程调度与优先级实验

## 1、实验目的

- 理解 Linux CFS 调度器在多进程竞争 CPU 时的基本调度现象。
- 掌握 taskset 绑定 CPU 核心、renice 调整进程优先级的方法。

## 2、实验内容及要求

编写一个持续占用 CPU 的单进程测试程序，分别启动两个 nice-exp 进程，观察初始 CPU 占用情况；再将两个进程绑定到同一 CPU 核心，并通过 renice 调整其中一个进程的 nice 值，观察 CFS 调度下 CPU 时间分配变化。

## 3、实验设计方案及原理

CFS 调度器会根据进程权重分配虚拟运行时间。nice 值越小，权重越大，同一 CPU 核心上竞争时获得的 CPU 时间比例越高。taskset 用于将进程绑定到指定 CPU 核心，renice 用于改变进程 nice 值。

**本实验采用单进程（单线程）程序，确保进程 PID 与线程 TID 一致，使 renice 修改的 nice 值直接作用于占用 CPU 的执行实体。**

> 说明：在 Linux NPTL 线程模型下，nice 是线程（task）属性。若使用多线程程序（主线程 sleep + 工作线程 while(1)），`renice PID` 只会修改主线程的 nice 值，而实际占用 CPU 的是工作线程（TID），导致优先级调整不生效。因此本实验采用单线程设计。

## 4、程序设计与流程图

程序 `main()` 首先打印进程号，然后进入 `while(1)` 空循环，持续产生 CPU 负载。

```
开始 → 打印 pid → while(1) 死循环 → (无结束)
```

## 5、程序与外部调度命令关系

nice-exp 程序由单个进程（单线程）进入 `while(1)` 死循环产生高 CPU 负载；进程 PID 与线程 TID 一致，renice 修改 PID 的 nice 值直接作用于实际占用 CPU 的执行实体。外部通过 taskset、renice、ps/top 等系统命令观察和调整进程调度效果。

## 6、源程序与关键说明

### nice-exp.c（单线程版）

```c
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(void)
{
    printf("pid:%d\n", getpid());
    while (1)
        ;
    return 0;
}
```

### 编译

```bash
mkdir -p ~/labs/cfs_experiment
cd ~/labs/cfs_experiment
gcc nice-exp.c -o nice-exp
ls -l nice-exp
```

## 7、运行过程与结果

实验运行过程分为启动准备、调整前验证、优先级调整和结果观察四个阶段。

### 阶段一：启动两个 nice-exp 测试进程，并将它们绑定到同一 CPU 核心

```bash
cd ~/labs/cfs_experiment
pkill nice-exp 2>/dev/null
taskset -c 0 ./nice-exp >/dev/null &
PID_A=$!
taskset -c 0 ./nice-exp >/dev/null &
PID_B=$!
echo "PID_A=$PID_A  PID_B=$PID_B"
```

### 阶段二：确认两个测试进程处于同一 CPU 核心，并记录调整前的 nice 值和 CPU 占用

```bash
taskset -cp $PID_A
taskset -cp $PID_B
ps -o pid,ni,psr,pcpu,comm -p $PID_A,$PID_B
```

**图 1-2**：截图显示两个 nice-exp 进程均绑定到 CPU 0，NI 均为 0，CPU 占用接近，符合调整前同优先级竞争状态。

### 阶段三：提高 PID_A 的调度优先级，并记录调整后的 nice 值和 CPU 占用变化

```bash
sudo renice -n -5 -p $PID_A
sleep 8
ps -o pid,ni,psr,pcpu,comm -p $PID_A,$PID_B
top -p $PID_A,$PID_B
```

**图 1-3**：截图显示两个进程的 NI 分别为 -5 和 0，说明 renice 已生效。

### 实验结束处理

```bash
pkill nice-exp 2>/dev/null
```

## 8、结果分析与实验小结

实验表明，两个单进程 nice-exp 程序绑定到同一 CPU 核心后会竞争同一处理器时间。由于程序采用单进程（单线程）设计，进程 PID 与线程 TID 一致，renice 修改 PID 的 nice 值直接作用于实际占用 CPU 的执行实体。

调整前两个进程 nice 值相同，CPU 占比接近；对其中一个进程执行 `sudo renice -n -5` 后，该进程 nice 值变小、权重增大，在 CFS 调度下获得的 CPU 时间比例显著高于 nice 值为 0 的进程，从而验证了 Linux CFS 调度器根据 nice 值权重分配运行时间的机制。

---

## 附录：多线程版本的注意事项

若使用多线程版本（主线程 `pthread_join` + 工作线程 `while(1)`），`renice PID` 只会修改主线程的 nice 值，实际占用 CPU 的工作线程 TID 不会受影响。解决方案：

1. **方案 A（推荐）**：使用单线程程序（本实验采用）。
2. **方案 B**：多线程程序中，通过 `syscall(SYS_gettid)` 获取工作线程 TID，然后 `renice -n -5 -p <TID>` 直接调整工作线程优先级。
