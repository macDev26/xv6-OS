# xv6 OS Extensions & Lab Projects

A comprehensive repository containing various enhancements and modifications to the MIT xv6 Operating System (both x86 and RISC-V versions) as part of an academic OS course (OS 344 / CS 344).

---

## 📁 Repository Structure

The project is organized into the following major modules and lab directories:

| Component / Directory | Platform | Description | Key Source Files |
| :--- | :--- | :--- | :--- |
| **[system_calls](file:///c:/Tech/Projects/xv6-OS/system_calls)** | x86 | Custom system calls and Shortest Job First (SJF) scheduling. | [`proc.c`](file:///c:/Tech/Projects/xv6-OS/system_calls/proc.c), [`syscall.c`](file:///c:/Tech/Projects/xv6-OS/system_calls/syscall.c), [`test_scheduler.c`](file:///c:/Tech/Projects/xv6-OS/system_calls/test_scheduler.c) |
| **[lazy_memory_allocation](file:///c:/Tech/Projects/xv6-OS/lazy_memory_allocation)** | x86 | Heap allocation optimization via page fault handled demand paging. | [`trap.c`](file:///c:/Tech/Projects/xv6-OS/lazy_memory_allocation/trap.c), [`sysproc.c`](file:///c:/Tech/Projects/xv6-OS/lazy_memory_allocation/sysproc.c) |
| **[kernel_threads](file:///c:/Tech/Projects/xv6-OS/kernel_threads)** | x86 | Lightweight kernel-level threads with user-space mutexes. | [`proc.c`](file:///c:/Tech/Projects/xv6-OS/kernel_threads/proc.c), [`ulib.c`](file:///c:/Tech/Projects/xv6-OS/kernel_threads/ulib.c), [`thread.c`](file:///c:/Tech/Projects/xv6-OS/kernel_threads/thread.c) |
| **[Labs](file:///c:/Tech/Projects/xv6-OS/Labs)** | Mixed | Lab reports, assignments, and advanced feature changes (IPC, VM swap, File Systems). | `Lab01` through `Lab05` |

---

## 🛠️ Modules & Features Detailed

### 1. System Calls & SJF Scheduler
*Location: [`system_calls/`](file:///c:/Tech/Projects/xv6-OS/system_calls)*

Extends xv6 with telemetry/utility system calls and implements a non-preemptive **Shortest Job First (SJF)** scheduler.

- **System Calls Added:**
  - `getNumProc`: Returns the total number of processes in the system.
  - `getMaxPID`: Returns the maximum PID among active processes.
  - `getProcInfo(pid, &info)`: Populates a [`processInfo`](file:///c:/Tech/Projects/xv6-OS/system_calls/processInfo.h) structure containing parent PID, memory size, and context switch counts.
  - `set_burst_time(time)` & `get_burst_time()`: Set/get expected execution time for SJF scheduling.
  - `getCurrentInfo(&info)` & `getCurrentPID()`: Telemetry for the active process.
- **SJF Scheduling:**
  - Standard round-robin scheduling is replaced with a Shortest Job First algorithm.
  - It maintains a priority queue (`pqueue`) of processes ordered by their estimated burst time.
  - Min-heap operations (`fix`, `extractMin`, `changeKey`) are implemented directly in [`proc.c`](file:///c:/Tech/Projects/xv6-OS/system_calls/proc.c).
- **Verification Tests:**
  - [`test_scheduler.c`](file:///c:/Tech/Projects/xv6-OS/system_calls/test_scheduler.c) forks CPU-bound and I/O-bound child processes with randomized burst times to verify proper scheduling order and track context switch counts.

---

### 2. Lazy Memory Allocation
*Location: [`lazy_memory_allocation/`](file:///c:/Tech/Projects/xv6-OS/lazy_memory_allocation)*

Optimizes the memory footprint of processes by delaying physical page allocation until a page is actually accessed.

- **Lazy Allocation Mechanism:**
  - The `sbrk()` system call in [`sysproc.c`](file:///c:/Tech/Projects/xv6-OS/lazy_memory_allocation/sysproc.c) is modified to only increment the process size (`sz`) without mapping or allocating any physical memory pages.
- **Page Fault Trap Handler:**
  - When the process accesses the unallocated heap range, a Page Fault exception (`T_PGFLT`) is generated.
  - In [`trap.c`](file:///c:/Tech/Projects/xv6-OS/lazy_memory_allocation/trap.c), the handler catches `T_PGFLT`, reads the faulting address using `rcr2()`, allocates a physical frame via `kalloc()`, zeroes it, and maps it using `mappages()`.
  - If allocation or mapping fails, the process is terminated gracefully.

---

### 3. Kernel Threads & User-space Mutexes
*Location: [`kernel_threads/`](file:///c:/Tech/Projects/xv6-OS/kernel_threads)*

Introduces lightweight execution threads sharing the same page table/address space as their parent process.

- **System Calls for Threading:**
  - `clone(fcn, arg, stack)`: Spawns a thread executing `fcn(arg)` using a caller-allocated user stack.
  - `join(&stackPtr)`: Suspends the caller until a child thread terminates, returning its stack pointer.
  - `texit()`: Thread-specific exit call.
- **User Threading Library:**
  - [`ulib.c`](file:///c:/Tech/Projects/xv6-OS/kernel_threads/ulib.c) provides wrappers: `thread_create()`, `thread_join()`, `thread_exit()`.
  - Implements user-level synchronization primitives:
    - **Spinlock**: Uses x86 atomic exchange `xchg()` to spin until the lock flag is acquired.
    - **Mutex**: A sleep-based mutex that puts the thread to sleep for 1 tick if the lock is held, avoiding CPU-spinning wastage.
- **Verification Tests:**
  - [`thread.c`](file:///c:/Tech/Projects/xv6-OS/kernel_threads/thread.c) demonstrates two threads updating a shared balance structure concurrently using `mutex_acquire` and `mutex_release` to prevent race conditions.

---

### 4. Course Lab Assignments
*Location: [`Labs/`](file:///c:/Tech/Projects/xv6-OS/Labs)*

This directory groups PDF instructions, academic lab reports, and advanced RISC-V/x86 code updates.

* **[Lab 01](file:///c:/Tech/Projects/xv6-OS/Labs/Lab01) & [Lab 02](file:///c:/Tech/Projects/xv6-OS/Labs/Lab02):** Introductory assignments on xv6 structure, shell operations, system call additions, and scheduling telemetry.
* **[Lab 03](file:///c:/Tech/Projects/xv6-OS/Labs/Lab03) - Shared Memory & Mailboxes (RISC-V):**
  - [`shm.c`](file:///c:/Tech/Projects/xv6-OS/Labs/Lab03/Lab%203%20Code%20Files/shm.c): Implements shared memory segments identified by integer keys, mapping them into the process's page table. Tracks process references with `refcnt` and frees the physical page on the last detach.
  - [`mbox.c`](file:///c:/Tech/Projects/xv6-OS/Labs/Lab03/Lab%203%20Code%20Files/mbox.c): Implements bounded-buffer IPC mailboxes. Uses kernel sleep and wakeup queues (`not_full`, `not_empty`) to block producers when full and consumers when empty.
* **[Lab 04](file:///c:/Tech/Projects/xv6-OS/Labs/Lab04) - Virtual Memory Swapping & Page Replacement (RISC-V):**
  - [`mru.c`](file:///c:/Tech/Projects/xv6-OS/Labs/Lab04/Lab%204%20Code%20changes/kernel/mru.c): Implements demand-paging virtual memory using a Most Recently Used (MRU) page eviction replacement policy.
  - Enforces a process-level limit of `MAX_PAGES_PER_PROC = 8` physical pages. When exceeded, the MRU page is swapped out to a simulated swap array (`swap_storage`) and marked in the page table with the custom `PTE_S` bit.
  - Accessing a swapped page triggers `handle_page_fault`, which allocates a new physical frame, copies the swapped data back in, and restores the PTE.
* **[Lab 05](file:///c:/Tech/Projects/xv6-OS/Labs/Lab05) - Doubly-Indirect Files & Symbolic Links (RISC-V):**
  - **Task 1 (Large Files):** Modifies [`fs.h`](file:///c:/Tech/Projects/xv6-OS/Labs/Lab05/Lab%205%20Code%20changes/Task%201/fs.h) and [`fs.c`](file:///c:/Tech/Projects/xv6-OS/Labs/Lab05/Lab%205%20Code%20changes/Task%201/fs.c) to support doubly-indirect blocks. Expands the maximum file size dynamically (adds 11 direct, 1 single-indirect, and 1 doubly-indirect block).
  - **Task 2 (Symbolic Links):** Adds a `symlink(target, path)` system call in [`sysfile.c`](file:///c:/Tech/Projects/xv6-OS/Labs/Lab05/Lab%205%20Code%20changes/Task%202/sysfile.c). Updates file lookup functions `namei` and `nameiparent` to resolve symbolic links recursively (up to a depth limit to prevent cycle deadlocks).

---

## 🚀 Running and Compiling

Instructions to compile and run the standard x86 configurations:

1. Install QEMU and the appropriate x86 ELF cross-compilers (e.g., `gcc-multilib` on Linux).
2. Inside the desired workspace directory (`system_calls`, `lazy_memory_allocation`, or `kernel_threads`), compile using:
   ```bash
   make
   ```
3. Boot xv6 in the QEMU simulator:
   ```bash
   make qemu
   ```

For the RISC-V labs under `Labs/Lab05/xv6-riscv`, follow the standard RISC-V cross-compilation pipeline:
```bash
make TOOLPREFIX=riscv64-unknown-elf-
make qemu
```