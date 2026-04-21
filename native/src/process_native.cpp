#include "process_native.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>

#ifdef _WIN32
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "advapi32.lib")

extern "C" {

int process_is_root() {
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
}

int process_get_file_limit() {
    return _getmaxstdio();
}

int process_set_file_limit(int limit) {
    return _setmaxstdio(limit);
}

NativeProcessInfo* process_list_all() {
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
}

void process_free_list(NativeProcessInfo* list) {
    while (list) { NativeProcessInfo* next = list->next; free(list); list = next; }
}

int process_get_id() {
    return (int)GetCurrentProcessId();
}

int process_fork() {
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
}

} // extern "C"

#else
// POSIX Implementation
#include <unistd.h>
#include <sys/resource.h>
#include <dirent.h>
#include <ctype.h>

#ifdef __APPLE__
#include <libproc.h>
#include <sys/proc_info.h>
#endif

extern "C" {

int process_is_root() { return geteuid() == 0 ? 1 : 0; }

int process_get_file_limit() {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) return (int)rl.rlim_cur;
    return -1;
}

int process_set_file_limit(int limit) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        rl.rlim_cur = (rlim_t)limit;
        if (rl.rlim_cur > rl.rlim_max) rl.rlim_max = rl.rlim_cur;
        if (setrlimit(RLIMIT_NOFILE, &rl) == 0) return (int)rl.rlim_cur;
    }
    return -1;
}

NativeProcessInfo* process_list_all() {
    NativeProcessInfo* head = NULL; NativeProcessInfo* tail = NULL;
#ifdef __APPLE__
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
#else
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
                if (sscanf(line, "%d %s %c %d", &dummy_pid, dummy_comm, &dummy_state, &ppid) == 4) {
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
#endif
    return head;
}

void process_free_list(NativeProcessInfo* list) {
    while (list) { NativeProcessInfo* next = list->next; free(list); list = next; }
}

int process_get_id() { return (int)getpid(); }

int process_fork() { return (int)fork(); }

} // extern "C"
#endif
