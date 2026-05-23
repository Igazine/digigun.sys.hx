#include "process_native.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>

#ifdef __linux__
#include <sys/prctl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <dirent.h>
#include <ctype.h>
#endif

#ifdef __APPLE__
#include <sys/event.h>
#include <thread>
#include <unistd.h>
#include <sys/resource.h>
#include <dirent.h>
#include <ctype.h>
#include <libproc.h>
#include <sys/proc_info.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <mach/mach_init.h>
#elif defined(__linux__)
#include <sched.h>
#endif

#ifdef _WIN32
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "advapi32.lib")

static DWORD WINAPI win32_parent_death_thread(LPVOID lpParam) {
    HANDLE hParent = (HANDLE)lpParam;
    WaitForSingleObject(hParent, INFINITE);
    CloseHandle(hParent);
    exit(0);
    return 0;
}
#endif

extern "C" {

int process_is_root() {
#ifdef _WIN32
    bool fIsElevated = false;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD dwSize;
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
            fIsElevated = elevation.TokenIsElevated != 0;
        }
    }
    if (hToken) CloseHandle(hToken);
    return fIsElevated ? 1 : 0;
#else
    return geteuid() == 0 ? 1 : 0;
#endif
}

int process_get_file_limit() {
#ifdef _WIN32
    return _getmaxstdio();
#else
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) return (int)rl.rlim_cur;
    return -1;
#endif
}

int process_set_file_limit(int limit) {
#ifdef _WIN32
    return _setmaxstdio(limit);
#else
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        rl.rlim_cur = (rlim_t)limit;
        if (rl.rlim_cur > rl.rlim_max) rl.rlim_max = rl.rlim_cur;
        if (setrlimit(RLIMIT_NOFILE, &rl) == 0) return (int)rl.rlim_cur;
    }
    return -1;
#endif
}

NativeProcessInfo* process_list_all() {
#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return NULL;
    PROCESSENTRY32 pe32;
    memset(&pe32, 0, sizeof(PROCESSENTRY32));
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hSnapshot, &pe32)) { CloseHandle(hSnapshot); return NULL; }
    NativeProcessInfo* head = NULL; NativeProcessInfo* tail = NULL;
    do {
        NativeProcessInfo* ni = (NativeProcessInfo*)malloc(sizeof(NativeProcessInfo));
        if (!ni) break;
        memset(ni, 0, sizeof(NativeProcessInfo));
        ni->pid = (int)pe32.th32ProcessID;
        ni->ppid = (int)pe32.th32ParentProcessID;
        strncpy(ni->name, pe32.szExeFile, 255); ni->name[255] = '\0';
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            PROCESS_MEMORY_COUNTERS_EX pmc;
            if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                ni->memory_rss = (double)pmc.WorkingSetSize;
                ni->memory_virtual = (double)pmc.PrivateUsage;
            }
            CloseHandle(hProcess);
        }
        ni->next = NULL; if (!head) head = ni; else tail->next = ni; tail = ni;
    } while (Process32Next(hSnapshot, &pe32));
    CloseHandle(hSnapshot); return head;
#elif defined(__APPLE__)
    NativeProcessInfo* head = NULL; NativeProcessInfo* tail = NULL;
    int buffer_size = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
    if (buffer_size <= 0) return NULL;
    pid_t* pids = (pid_t*)malloc(buffer_size);
    int actual_size = proc_listpids(PROC_ALL_PIDS, 0, pids, buffer_size);
    int actual_count = actual_size / sizeof(pid_t);
    for (int i = 0; i < actual_count; i++) {
        if (pids[i] <= 0) continue;
        NativeProcessInfo* ni = (NativeProcessInfo*)malloc(sizeof(NativeProcessInfo));
        if (!ni) break;
        memset(ni, 0, sizeof(NativeProcessInfo));
        ni->pid = pids[i];
        struct proc_bsdshortinfo bsd_info;
        if (proc_pidinfo(pids[i], PROC_PIDT_SHORTBSDINFO, 0, &bsd_info, sizeof(bsd_info)) > 0) {
            ni->ppid = bsd_info.pbsi_ppid;
        }
        if (proc_name(pids[i], ni->name, 255) <= 0) strncpy(ni->name, "Unknown", 255);
        struct proc_taskinfo task_info;
        if (proc_pidinfo(pids[i], PROC_PIDTASKINFO, 0, &task_info, sizeof(task_info)) > 0) {
            ni->memory_rss = (double)task_info.pti_resident_size;
            ni->memory_virtual = (double)task_info.pti_virtual_size;
        }
        ni->next = NULL; if (!head) head = ni; else tail->next = ni; tail = ni;
    }
    free(pids);
    return head;
#else
    NativeProcessInfo* head = NULL; NativeProcessInfo* tail = NULL;
    DIR* dir = opendir("/proc"); if (!dir) return NULL;
    struct dirent* entry; long page_size = sysconf(_SC_PAGESIZE);
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR || !isdigit(entry->d_name[0])) continue;
        int pid = atoi(entry->d_name);
        NativeProcessInfo* ni = (NativeProcessInfo*)malloc(sizeof(NativeProcessInfo));
        if (!ni) break;
        memset(ni, 0, sizeof(NativeProcessInfo));
        ni->pid = pid;
        char path[512]; char line[512];
        snprintf(path, 512, "/proc/%d/stat", pid);
        FILE* fstat = fopen(path, "r");
        if (fstat) {
            if (fgets(line, sizeof(line), fstat)) {
                int dummy_pid; char dummy_comm[256]; char dummy_state; int ppid;
                if (sscanf(line, "%d %s %c %d", &dummy_pid, dummy_comm, &dummy_state, &ppid) >= 4) {
                    ni->ppid = ppid;
                }
            }
            fclose(fstat);
        }
        snprintf(path, 512, "/proc/%d/comm", pid);
        FILE* f = fopen(path, "r");
        if (f) {
            if (fgets(ni->name, 255, f)) {
                size_t len = strlen(ni->name); if (len > 0 && ni->name[len-1] == '\n') ni->name[len-1] = '\0';
            }
            fclose(f);
        }
        snprintf(path, 512, "/proc/%d/statm", pid);
        f = fopen(path, "r");
        if (f) {
            if (fgets(line, sizeof(line), f)) {
                long vmsize, rss;
                if (sscanf(line, "%ld %ld", &vmsize, &rss) == 2) {
                    ni->memory_rss = (double)rss * page_size; ni->memory_virtual = (double)vmsize * page_size;
                }
            }
            fclose(f);
        }
        ni->next = NULL; if (!head) head = ni; else tail->next = ni; tail = ni;
    }
    closedir(dir);
    return head;
#endif
}

void process_free_list(NativeProcessInfo* list) {
    while (list) { NativeProcessInfo* next = list->next; free(list); list = next; }
}

int process_get_id() {
#ifdef _WIN32
    return (int)GetCurrentProcessId();
#else
    return (int)getpid();
#endif
}

int process_fork() {
#ifdef _WIN32
    // Windows fork emulation: re-launch the current executable
    char szPath[MAX_PATH];
    if (GetModuleFileNameA(NULL, szPath, MAX_PATH) == 0) return -1;

    // Use a special flag to identify as child
    std::string cmdLine = std::string(szPath) + " --digigun-child-fork";
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    if (CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        int childPid = (int)pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return childPid; // Parent returns child PID
    }
    return -1;
#else
    return (int)fork();
#endif
}

void process_exit_with_parent() {
#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    DWORD ppid = 0;
    DWORD currentPid = GetCurrentProcessId();
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == currentPid) {
                ppid = pe32.th32ParentProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);

    if (ppid > 0) {
        HANDLE hParent = OpenProcess(SYNCHRONIZE, FALSE, ppid);
        if (hParent) {
            CreateThread(NULL, 0, win32_parent_death_thread, hParent, 0, NULL);
        }
    }
#elif defined(__linux__)
    prctl(PR_SET_PDEATHSIG, SIGKILL);
#elif defined(__APPLE__)
    int ppid = getppid();
    if (ppid > 1) {
        std::thread([ppid]() {
            int kq = kqueue();
            struct kevent ke;
            EV_SET(&ke, ppid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, NULL);
            kevent(kq, &ke, 1, NULL, 0, NULL);
            struct kevent event;
            kevent(kq, NULL, 0, &event, 1, NULL);
            exit(0);
        }).detach();
    }
#endif
}

int process_set_affinity(int mask) {
#ifdef _WIN32
    return SetProcessAffinityMask(GetCurrentProcess(), (DWORD_PTR)mask) ? 1 : 0;
#elif defined(__APPLE__)
    thread_affinity_policy_data_t policy = { mask };
    kern_return_t ret = thread_policy_set(
        mach_thread_self(),
        THREAD_AFFINITY_POLICY,
        (thread_policy_t)&policy,
        THREAD_AFFINITY_POLICY_COUNT
    );
    return (ret == KERN_SUCCESS) ? 1 : 0;
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < 32; i++) {
        if (mask & (1 << i)) CPU_SET(i, &cpuset);
    }
    return (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0) ? 1 : 0;
#endif
}

int process_get_affinity() {
#ifdef _WIN32
    DWORD_PTR procMask, sysMask;
    if (GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask)) {
        return (int)procMask;
    }
    return -1;
#elif defined(__APPLE__)
    return -1;
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (sched_getaffinity(0, sizeof(cpu_set_t), &cpuset) == 0) {
        int mask = 0;
        for (int i = 0; i < 32; i++) {
            if (CPU_ISSET(i, &cpuset)) mask |= (1 << i);
        }
        return mask;
    }
    return -1;
#endif
}

int process_set_priority(int priority_class) {
#ifdef _WIN32
    DWORD win_priority;
    switch (priority_class) {
        case 0: win_priority = IDLE_PRIORITY_CLASS; break;
        case 1: win_priority = BELOW_NORMAL_PRIORITY_CLASS; break;
        case 2: win_priority = NORMAL_PRIORITY_CLASS; break;
        case 3: win_priority = ABOVE_NORMAL_PRIORITY_CLASS; break;
        case 4: win_priority = HIGH_PRIORITY_CLASS; break;
        case 5: win_priority = REALTIME_PRIORITY_CLASS; break;
        default: win_priority = NORMAL_PRIORITY_CLASS;
    }
    return SetPriorityClass(GetCurrentProcess(), win_priority) ? 1 : 0;
#else
    int nice_val;
    switch (priority_class) {
        case 0: nice_val = 19; break; // Idle
        case 1: nice_val = 10; break; // Below Normal
        case 2: nice_val = 0;  break; // Normal
        case 3: nice_val = -5; break; // Above Normal
        case 4: nice_val = -15; break; // High
        case 5: nice_val = -20; break; // Realtime
        default: nice_val = 0;
    }
    return (setpriority(PRIO_PROCESS, 0, nice_val) == 0) ? 1 : 0;
#endif
}

int process_get_priority() {
    // Standard Haxe doesn't have process_get_priority yet, 
    // but we can implement it if needed later. 
    return 0; 
}

DIGIGUN_API const char* process_echo(const char* input) { return input; }

struct Point { int x; int y; };
struct ComplexData { int id; double value; const char* name; };

DIGIGUN_API void process_point(struct Point* p) {
    if (p) { p->x += 100; p->y += 200; }
}

DIGIGUN_API void process_complex(struct ComplexData* d) {
    if (d) { d->id *= 2; d->value += 1.5; }
}

} // extern "C"
