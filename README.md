# xv6 Operating System Enhancements (Project 1)

This repository contains substantial modifications and enhancements to the **xv6 Operating System** kernel. The project focuses on improving the kernel's console interactivity, implementing advanced shell features, and adding custom user-level programs.

Developed as part of the **Operating Systems Lab **.

## 👥 Contributors

* **Sadra Madayeni** 
* **Amir Hossein Alikhani**
* **Hasti Abolhassani**

---

## 🚀 Key Features Implemented

We modified the xv6 kernel to support modern console features, text manipulation, and shell automation.

### 1. Advanced Console Interaction
We overhauled `console.c` and `kbd.c` to support rich text editing capabilities directly in the kernel console:

* **Cursor Navigation:**
    *  **Left/Right Arrow Keys:** Move the cursor character-by-character within the command line[cite: 24, 47].
    * **Word Navigation:**
        *  `Ctrl + D`: Move cursor to the **beginning of the next word**[cite: 94].
        *  `Ctrl + A`: Move cursor to the **beginning of the previous/current word**[cite: 116].

* **Undo Mechanism (`Ctrl + Z`):**
    * Implemented a history buffer struct (`edit_op`) to track insertion and deletion operations.
    *  Supports multi-level undo functionality, reverting text changes and restoring cursor positions dynamically[cite: 167, 183].

* **Clipboard & Text Selection:**
    * **Selection (`Ctrl + S`):** Toggles selection mode. Users can highlight a substring of the command line.  Highlighted text is rendered with a distinct background color (white highlight)[cite: 230, 253].
    *  **Copy (`Ctrl + C`):** Copies the selected text into a kernel-level clipboard buffer[cite: 258].
    *  **Paste (`Ctrl + V`):** Inserts the clipboard content at the current cursor position[cite: 274].
    * **Delete Selection:** Pressing `Backspace` or replacing text while selected is fully supported.

### 2. Shell Enhancements
* **Tab Autocompletion:**
    * Modified `sh.c` to support dynamic command completion.
    * Pressing `TAB` scans available commands and programs. If a unique match is found, it auto-completes the command.  If multiple matches exist, it cycles through valid options[cite: 371, 435].

### 3. User-Level Programs
* **`find_sum` Utility:**
    * A custom C program added to `UPROGS` in the Makefile.
    * **Usage:** Accepts a string containing numbers and characters.  It extracts all integers, calculates their sum, and writes the result to a file named `result.txt`[cite: 300, 318].
    * *Example:* Input `op1e2rati34ng5` $\rightarrow$ Output `42` (1+2+34+5).

### 4. Boot Customization
*  Modified `init.c` to display the project contributors' names upon system boot, verifying successful kernel compilation and entry into user space[cite: 6, 7].

---

## 🛠️ Technical Implementation Details

### File Modifications
* **`console.c` & `kbd.c`:** The core logic for cursor movement, reference counting for the undo buffer, and the state machine for text selection (highlighting) was implemented here.  We handled specific scan codes (e.g., `0xE4` for Left, `0xE5` for Right) to map physical keys to logical cursor actions[cite: 24].
* **`sh.c`:** Enhanced to handle the `TAB` key interrupt and interface with the file system to find executable matches for autocompletion.
* **`init.c`:** Altered the initialization process to print the group greeting.
* **`find_sum.c`:** Created a new source file utilizing file descriptors (`open`, `write`) and string parsing.

### Challenges Solved
* **Visual Rendering:** One of the main challenges was updating the CRT controller (`CRTPORT`) correctly to move the blinking cursor without corrupting the video memory buffer, especially during "Undo" operations where multiple characters shift simultaneously.
*  **Buffer Management:** Implementing Copy/Paste required a dedicated static buffer (`clipboard_buf`) and careful boundary checks to prevent buffer overflows during paste operations[cite: 210].

---

## 💻 How to Run

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/Sadra-Madayeni/Operating-System-LAB-projects
    cd codes
    ```

2.  **Build and Run in QEMU:**
    ```bash
    make qemu
    ```

3.  **Test Features:**
    * **Boot:** Observe names printed on startup.
    * **Console:** Type text, use Arrow keys, try `Ctrl+A/D` to jump words. Select text with `Ctrl+S`, Copy with `Ctrl+C`, and Paste with `Ctrl+V`.
    * **User Program:**
        ```bash
        find_sum string123with456numbers
        cat result.txt
        ```

---


# xv6 System Calls & Kernel Hacking (Project 2)

This repository contains the implementation of custom system calls and kernel enhancements for the **xv6 Operating System**. This project explores kernel-space file management, process scheduling algorithms, and low-level register manipulation.
 

## 🚀 Key Features Implemented

We expanded the xv6 kernel capabilities by adding 5 distinct system calls and modifying the core scheduler.

### 1. Register-Based Argument Passing
**Syscall:** `simple_arithmetic_syscall`
* **Goal:** Bypass the standard stack-based argument passing convention (`argint`) and read arguments directly from CPU registers.
* **Mechanism:**
    * The user program uses **Inline Assembly** to move arguments into `EBX` and `ECX` registers before triggering interrupt `0x40` (64).
    * The kernel reads directly from the process's **Trap Frame** (`tf->ebx`, `tf->ecx`).
* **Function:** Calculates $(a+b) \times (a-b)$.

### 2. Kernel-Level File Duplication
**Syscall:** `make_duplicate(char *src_file)`
* **Goal:** Create a copy of a file entirely within kernel space without transferring data buffers to user space.
* **Implementation:**
    * Opens the source file using `namei` and locks the inode (`ilock`).
    * Creates a destination file with a `.copy` suffix.
    * Reads raw disk blocks using `bread` and writes to the new inode using `writei`.
    * Uses kernel memory allocation (`kalloc`) for temporary buffering.

### 3. Process Genealogy (Family Tree)
**Syscall:** `show_process_family(int pid)`
* **Goal:** Inspect the process table to reveal relationships between active processes.
* **Features:**
    * Iterates through the kernel `ptable`.
    * Identifies and prints the **Parent** (PID), **Children** (PIDs), and **Siblings** (processes sharing the same parent) of the target PID.

### 4. In-Kernel GREP
**Syscall:** `grep_syscall(char *keyword, char *filename, char *buffer, int size)`
* **Goal:** Search for a specific string inside a file *without* reading the file into user memory first.
* **Implementation:**
    * Reads the file block-by-block inside the kernel.
    * Implements a custom string search algorithm.
    * If the line containing the keyword is found, it is copied to the user's buffer using `copyout`.

### 5. Priority Scheduling
**Syscall:** `set_priority_syscall(int pid, int priority)`
* **Goal:** Replace the default Round-Robin scheduler with a **Priority-Based Scheduler**.
* **Modifications:**
    * Added `int priority` to `struct proc`.
    * Modified `allocproc` to set a default priority (1).
    * **Scheduler Logic:** The scheduler now scans the entire process table to find the `RUNNABLE` process with the **highest priority** (lowest numerical value) and switches context to it.

---

## 🛠️ Technical Details

### Modified Files
* `kernel/syscall.h` & `kernel/syscall.c`: Registered new system call numbers and handlers.
* `kernel/sysproc.c`: Implemented the logic for process-related syscalls.
* `kernel/proc.c`: Modified the `scheduler()` loop for priority scheduling and `allocproc`.
* `kernel/sysfile.c`: Implemented file-heavy syscalls (`grep`, `duplicate`).
* `kernel/fs.c`: Exposed `bmap` and file system helpers for kernel-level I/O.

### User Programs (Testing)
We created specific user-level programs to test each feature:
* `regcalltest`: Uses inline assembly to test register argument passing.
* `makeduptest`: Duplicates a file and verifies the `.copy` existence.
* `familytest`: Forks children to create a process tree and queries relations.
* `grep_test`: Searches for keywords in a file created via `cat`.
* `priority_test`: Spawns child processes with different priorities to demonstrate execution order.

---

## 💻 How to Run

1.  **Build and Run QEMU:**
    ```bash
    make qemu
    ```

2.  **Test Register Syscall:**
    ```bash
    $ regcalltest
    # Output: Calc: (5+3)*(5-3)=16
    ```

3.  **Test Process Family:**
    ```bash
    $ familytest 2
    # Displays Parent, Children, and Siblings of PID 2.
    ```

4.  **Test Priority Scheduler:**
    ```bash
    $ priority_test
    # You should see High Priority children finishing before Low Priority ones.
    ```

5.  **Test In-Kernel Grep:**
    ```bash
    $ cat testfile.txt
    hello world
    this is a test
    $ grep_test test testfile.txt
    # Output: Found line: 'this is a test'
    ```

---


# xv6 Heterogeneous Scheduling (Project 3)

This project contains advanced modifications to the **xv6 Operating System** scheduler. The project transforms xv6 from a simple Round-Robin system into a **Heterogeneous Multiprocessing (HMP)** system that distinguishes between Performance cores (P-cores) and Efficiency cores (E-cores), applying different scheduling policies to each.


## 🧠 Core Features

We implemented a heterogeneity-aware scheduler that optimizes for both performance and energy efficiency, mimicking modern hybrid CPU architectures (like Intel Alder Lake or ARM big.LITTLE).

### 1. Hybrid CPU Architecture
* **Core Identification:** The kernel now detects the `CPUID`.
    * **E-Cores (Efficiency):** Even-numbered CPUs (0, 2, ...). Optimized for background or standard tasks.
    * **P-Cores (Performance):** Odd-numbered CPUs (1, 3, ...). Optimized for critical, high-performance tasks.
* **Per-Core Queues:** Replaced the single global run queue with unique run queues for each core to reduce lock contention and improve scalability.

### 2. Dual-Algorithm Scheduling
Each core type utilizes a distinct scheduling algorithm tailored to its purpose:

* **E-Cores: Round Robin (RR)**
    * Standard time-slicing approach.
    * **Custom Quantum:** Modified the timer interrupt handler to enforce a **30ms time slice** (approx. 3 ticks) instead of the default 10ms, reducing context switch overhead for efficiency.

* **P-Cores: Modified FCFS (First-Come-First-Served)**
    * Non-preemptive scheduling for sustained performance.
    * **Selection Logic:** Selects processes based on **Creation Time** (not queue arrival time). The oldest process in the system gets priority on P-cores to minimize turnaround time.

### 3. Load Balancing (Process Pushing)
To prevent core starvation or overloading, we implemented an active load balancer:
* **Mechanism:** Periodic check (every 50ms) initiated by E-cores.
* **Threshold:** If an E-core's queue is significantly longer than the lightest P-core (Threshold $\ge$ 3 processes), it "pushes" a waiting process to that P-core.
* **Constraint:** `init` and `sh` processes are pinned to E-cores to ensure system stability.

### 4. Performance Evaluation System
Added system calls to measure scheduler throughput:
* `sys_start_measuring()`: Resets kernel-level tick counters.
* `sys_stop_measuring()`: Calculates and prints the system throughput (Processes completed / Time).
* `sys_print_info()`: Dumps the current state, CPU affinity, and scheduling policy of a specific process.

---

## 🛠️ Technical Implementation

### Key File Modifications
* **`kernel/proc.c` & `proc.h`:**
    * Extended `struct cpu` to track core type (E/P).
    * Implemented per-core process lists.
    * Rewrote `scheduler()` to branch logic based on `cpuid`.
    * Added `creation_time` to `struct proc` for the P-core FCFS logic.
* **`kernel/trap.c`:** Modified the timer interrupt handler to manage the custom 30ms quantum for E-cores.
* **`kernel/sysproc.c`:** Implemented the measurement system calls.

### Load Balancing Logic
The load balancer runs within the scheduler loop. It strictly enforces a "push" model (E $\to$ P) to preserve the energy-saving characteristics of E-cores while utilizing P-cores when the system is under heavy load.

---

## 📊 Evaluation & Testing

### Throughput Analysis
We developed user-level programs to stress-test the scheduler:
1.  **Workload Generation:** Spawns multiple CPU-intensive and I/O-intensive processes.
2.  **Measurement:**
    * **Scenario A (Single Core):** Baseline throughput.
    * **Scenario B (Multi-Core Heterogeneous):** Demonstrates improved throughput via P-cores and Load Balancing.

### How to Run

1.  **Build with Multiple CPUS:**
    The heterogeneity logic requires at least 2 CPUs to activate both P and E cores.
    ```bash
    make qemu CPUS=4
    ```

2.  **Run Test Program:**
    ```bash
    $ schedtest
    # Output will show PIDs executing on specific Core IDs (e.g., PID 5 on CPU 1 [P-Core])
    ```

3.  **Check Throughput:**
    ```bash
    $ throughput_test
    # Output: System Throughput: X processes/sec
    ```

---


# xv6 Synchronization & Concurrency (Project 4)

This repository contains advanced modifications to the **xv6 Operating System** kernel, focusing on concurrency control, deadlock prevention, and synchronization primitives. The project addresses limitations in standard spinlocks/sleeplocks and introduces priority-aware scheduling for locks.


## 🔐 Key Features Implemented

We enhanced the xv6 kernel with four major synchronization mechanisms:

### 1. Safer Sleeplocks (Ownership Tracking)
* **Problem:** Standard xv6 sleeplocks do not track which process owns the lock, allowing any process to accidentally release a lock held by another.
* **Solution:** Modified `struct sleeplock` to store the `pid` of the owner.
* **Mechanism:** `releasesleep()` now panics if the releasing process is not the owner, preventing race conditions caused by buggy code.

### 2. Reader-Writer Locks (Bonus)
* **Problem:** Exclusive locks reduce performance for read-heavy data structures (e.g., file lists).
* **Solution:** Implemented `struct rwlock` allowing **Multiple Readers** OR **Single Exclusive Writer**.
* **Logic:** Readers increment a counter (`read_count`). Writers wait until the counter is zero and the lock is free.

### 3. Per-CPU Lock Profiling
* **Problem:** Identifying "hot" locks (bottlenecks) in a multicore system is difficult without metrics.
* **Solution:** Implemented a profiling system using **Per-CPU data structures** to prevent cache thrashing while collecting statistics.
* **Metrics:**
    * `acq_count`: How many times the lock was acquired.
    * `total_spins`: How many loop iterations the CPU wasted waiting for the lock.
* **Syscall:** `sys_getlockstat` allows user-space programs to retrieve these metrics for analysis.

### 4. Priority Locks (Plock)
* **Problem:** Standard spinlocks are "luck-based" (FIFO/Random), potentially leading to **Starvation** or **Priority Inversion** where high-priority processes wait for low-priority ones.
* **Solution:** Implemented `struct plock` with a **Priority Queue**.
* **Mechanism:**
    * Processes waiting for a lock are added to a linked list sorted (or scanned) by priority.
    * **Lock Handoff:** Upon release, the lock is *not* simply opened. Instead, ownership is directly transferred to the highest-priority waiting process, and only that process is woken up.

---

 

 # xv6 Memory Management & Page Replacement (Project 5)

This repository contains the final project for the Operating Systems Lab, focusing on **Virtual Memory Management**. We implemented a custom software-managed paging system within the xv6 kernel to simulate restricted physical memory and evaluate various **Page Replacement Algorithms**.


## 🧠 Core Objectives

The goal was to simulate a scenario where physical memory is scarce, forcing the Operating System to swap pages in and out. We implemented:

1.  **Software-Managed Page Table:** A centralized kernel data structure to track virtual pages, mapping them to a limited set of physical frames (simulated capacity: 4-10 pages).
2.  **Page Replacement Algorithms:** Logic to decide which page to evict when a page fault occurs and memory is full.
3.  **Performance Analytics:** Real-time tracking of **Hit Counts** and **Hit Ratios** to compare algorithm efficiency.

---

## ⚙️ Implemented Algorithms

We implemented four distinct replacement policies to handle page faults:

### 1. FIFO (First-In, First-Out)
* **Logic:** The oldest page in the memory (the one brought in first) is the first to be evicted.
* **Implementation:** Maintained a simple queue index to track the insertion order.

### 2. LRU (Least Recently Used) * **Logic:** Evicts the page that has not been accessed for the longest time.
* **Implementation:** Every time a page is accessed (read/write), its "timestamp" or position in the list is updated. The page at the bottom of the stack is evicted.

### 3. LFU (Least Frequently Used)
* **Logic:** Evicts the page with the lowest access count.
* **Implementation:** Added a `frequency_counter` to the page structure. This counter increments on every hit.

### 4. Clock Algorithm (Second-Chance)
* **Logic:** A modification of FIFO that gives pages a "second chance" if they have been referenced recently.
* **Implementation:** Uses a circular buffer and a reference bit (`R`). The pointer moves like a clock hand; if `R=1`, it resets `R=0` and moves on. If `R=0`, that page is evicted.

---

 
