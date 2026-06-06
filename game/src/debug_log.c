#include "debug_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <windows.h>

#define TIMESTAMP_LEN 64

static FILE* s_logFile = NULL;
static int s_initialized = 0;
static char s_baseDir[MAX_PATH];

unsigned int g_debugCategories = DEBUG_ALL;

void DebugLog_SetFilter(unsigned int mask) {
    g_debugCategories = mask;
}

static void GetTimestamp(char* buf, int len) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    if (t) {
        strftime(buf, len, "%Y-%m-%d_%H-%M-%S", t);
    } else {
        snprintf(buf, len, "unknown");
    }
}

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* info) {
    char crashDir[MAX_PATH];
    snprintf(crashDir, MAX_PATH, "%s\\crash", s_baseDir);
    CreateDirectoryA(crashDir, NULL);

    char crashPath[MAX_PATH];
    char ts[TIMESTAMP_LEN];
    GetTimestamp(ts, TIMESTAMP_LEN);
    snprintf(crashPath, MAX_PATH, "%s\\crash_%s.txt", crashDir, ts);

    FILE* f = fopen(crashPath, "w");
    if (f) {
        fprintf(f, "=== Heroes of Taboo Crash Report ===\n");
        fprintf(f, "Time: %s\n", ts);

        if (info && info->ExceptionRecord) {
            DWORD code    = info->ExceptionRecord->ExceptionCode;
            PVOID addr    = info->ExceptionRecord->ExceptionAddress;
            DWORD flags   = info->ExceptionRecord->ExceptionFlags;

            fprintf(f, "ExceptionCode: 0x%08lX\n", code);
            fprintf(f, "ExceptionAddr: 0x%p\n", addr);
            fprintf(f, "ExceptionFlags: %lu\n", flags);

            const char* desc = "Unknown";
            switch (code) {
                case EXCEPTION_ACCESS_VIOLATION:    desc = "Access Violation"; break;
                case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: desc = "Array Bounds Exceeded"; break;
                case EXCEPTION_BREAKPOINT:           desc = "Breakpoint"; break;
                case EXCEPTION_DATATYPE_MISALIGNMENT:desc = "Datatype Misalignment"; break;
                case EXCEPTION_FLT_DENORMAL_OPERAND: desc = "Float Denormal"; break;
                case EXCEPTION_FLT_DIVIDE_BY_ZERO:   desc = "Float Divide By Zero"; break;
                case EXCEPTION_FLT_INEXACT_RESULT:   desc = "Float Inexact"; break;
                case EXCEPTION_FLT_INVALID_OPERATION:desc = "Float Invalid"; break;
                case EXCEPTION_FLT_OVERFLOW:         desc = "Float Overflow"; break;
                case EXCEPTION_FLT_STACK_CHECK:      desc = "Float Stack Check"; break;
                case EXCEPTION_FLT_UNDERFLOW:        desc = "Float Underflow"; break;
                case EXCEPTION_ILLEGAL_INSTRUCTION:  desc = "Illegal Instruction"; break;
                case EXCEPTION_IN_PAGE_ERROR:        desc = "In-Page Error"; break;
                case EXCEPTION_INT_DIVIDE_BY_ZERO:   desc = "Int Divide By Zero"; break;
                case EXCEPTION_INT_OVERFLOW:         desc = "Int Overflow"; break;
                case EXCEPTION_INVALID_DISPOSITION:  desc = "Invalid Disposition"; break;
                case EXCEPTION_NONCONTINUABLE_EXCEPTION: desc = "Noncontinuable Exception"; break;
                case EXCEPTION_PRIV_INSTRUCTION:     desc = "Privileged Instruction"; break;
                case EXCEPTION_SINGLE_STEP:          desc = "Single Step"; break;
                case EXCEPTION_STACK_OVERFLOW:       desc = "Stack Overflow"; break;
            }
            fprintf(f, "ExceptionDesc: %s\n", desc);

            if (info->ContextRecord) {
#if defined(_M_X64) || defined(__x86_64__)
                fprintf(f, "RAX: 0x%016llX RBX: 0x%016llX RCX: 0x%016llX RDX: 0x%016llX\n",
                    info->ContextRecord->Rax, info->ContextRecord->Rbx,
                    info->ContextRecord->Rcx, info->ContextRecord->Rdx);
                fprintf(f, "RSI: 0x%016llX RDI: 0x%016llX RBP: 0x%016llX RSP: 0x%016llX\n",
                    info->ContextRecord->Rsi, info->ContextRecord->Rdi,
                    info->ContextRecord->Rbp, info->ContextRecord->Rsp);
                fprintf(f, "RIP: 0x%016llX\n", info->ContextRecord->Rip);
#else
                fprintf(f, "EAX: 0x%08lX EBX: 0x%08lX ECX: 0x%08lX EDX: 0x%08lX\n",
                    info->ContextRecord->Eax, info->ContextRecord->Ebx,
                    info->ContextRecord->Ecx, info->ContextRecord->Edx);
                fprintf(f, "ESI: 0x%08lX EDI: 0x%08lX EBP: 0x%08lX ESP: 0x%08lX\n",
                    info->ContextRecord->Esi, info->ContextRecord->Edi,
                    info->ContextRecord->Ebp, info->ContextRecord->Esp);
                fprintf(f, "EIP: 0x%08lX\n", info->ContextRecord->Eip);
#endif
            }
        } else {
            fprintf(f, "(no exception info available)\n");
        }

        fclose(f);
    }

    // Flush the debug log if open
    if (s_logFile) {
        fprintf(s_logFile, "\n=== CRASH DETECTED ===\n");
        fflush(s_logFile);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void DebugLog_InitCrashHandler(void) {
    SetUnhandledExceptionFilter(CrashHandler);
}

static void EnsureLogDirs(void) {
    if (s_initialized) return;
    s_initialized = 1;

    char exePath[MAX_PATH];
    DWORD plen = GetModuleFileNameA(NULL, exePath, MAX_PATH);
    if (plen == 0 || plen >= MAX_PATH) return;

    char* sep = strrchr(exePath, '\\');
    if (!sep) return;
    *(sep + 1) = '\0';
    snprintf(s_baseDir, MAX_PATH, "%slogs", exePath);

    char latestDir[MAX_PATH], oldDir[MAX_PATH];
    snprintf(latestDir, MAX_PATH, "%s\\latest", s_baseDir);
    snprintf(oldDir,    MAX_PATH, "%s\\old",    s_baseDir);
    CreateDirectoryA(s_baseDir, NULL);
    CreateDirectoryA(latestDir, NULL);
    CreateDirectoryA(oldDir,    NULL);

    char crashDir[MAX_PATH];
    snprintf(crashDir, MAX_PATH, "%s\\crash", s_baseDir);
    CreateDirectoryA(crashDir, NULL);

    DebugLog_InitCrashHandler();

    char oldPath[MAX_PATH], archivedPath[MAX_PATH];
    snprintf(oldPath, MAX_PATH, "%s\\debug.log", latestDir);
    FILE* oldLog = fopen(oldPath, "r");
    if (oldLog) {
        fclose(oldLog);
        char ts[TIMESTAMP_LEN];
        GetTimestamp(ts, TIMESTAMP_LEN);
        snprintf(archivedPath, MAX_PATH, "%s\\debug_%s.log", oldDir, ts);
        rename(oldPath, archivedPath);
    }

    char logPath[MAX_PATH];
    snprintf(logPath, MAX_PATH, "%s\\debug.log", latestDir);
    s_logFile = fopen(logPath, "w");
    if (s_logFile) {
        setbuf(s_logFile, NULL);
        char ts[TIMESTAMP_LEN];
        GetTimestamp(ts, TIMESTAMP_LEN);
        fprintf(s_logFile, "--- Heroes of Taboo Debug Log ---\n");
        fprintf(s_logFile, "--- Started: %s ---\n", ts);
        fprintf(s_logFile, "--- Categories: 0x%08X ---\n\n", g_debugCategories);
    }
}

void DebugLog_Write(unsigned int category, const char* fmt, ...) {
    if (!(g_debugCategories & category)) return;
    EnsureLogDirs();
    if (!s_logFile) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(s_logFile, fmt, args);
    fputc('\n', s_logFile);
    fflush(s_logFile);
    va_end(args);
}
