#include "system_calls.h"


namespace rail{

    bool handle_syscalls(uint64_t *TRAPFRAME){

        uint64_t A0 = TRAPFRAME[regs::A0 -1];
        uint64_t A1 = TRAPFRAME[regs::A1 -1];
        uint64_t A2 = TRAPFRAME[regs::A2 -1];
        uint64_t A3 = TRAPFRAME[regs::A3 -1];
        uint64_t A4 = TRAPFRAME[regs::A4 -1];
        uint64_t A5 = TRAPFRAME[regs::A5 -1];
        uint64_t A6 = TRAPFRAME[regs::A6 -1];
        uint64_t A7 = TRAPFRAME[regs::A7 -1];

        uint64_t offset = CodeCache::dataOffset;
        
        bool ecalled = true;

        switch(A7){
            case SYSTEMCALL::READ:
                TRAPFRAME[regs::A0 -1] = read(A0, (void *)(A1 + offset), A2);
                fflush(NULL);
                #ifdef DEBUG
                    outfile << "READ SYSTEMCALL DETECTED" << std::endl;
                #endif
                break;
            case SYSTEMCALL::READV:
                TRAPFRAME[regs::A0 -1] = read(A0, (void *)(A1 + offset), A2);
                #ifdef DEBUG
                    outfile << "READV SYSTEMCALL DETECTED" << std::endl;
                #endif
                break;
            case SYSTEMCALL::WRITE:
                TRAPFRAME[regs::A0 -1] = write(A0, (void *)(A1 + offset), A2);
                #ifdef DEBUG
                    outfile << "WRITE SYSTEMCALL DETECTED" << std::endl;
                #endif
                break;
            case SYSTEMCALL::WRITEV:
                TRAPFRAME[regs::A0 -1] = write(A0, (void *)(A1 + offset), A2);
                #ifdef DEBUG
                    outfile << "WRITEV SYSTEMCALL DETECTED" << std::endl;
                #endif
                break;
            case SYSTEMCALL::EXIT:
                exit_binary(TRAPFRAME);
            case SYSTEMCALL::EXIT_GROUP:
                exit_binary(TRAPFRAME);
                break;
            default:
                ecalled = false;
                break;
        }

        return ecalled;
    }


    const std::unordered_map<SYSTEMCALL, std::string> SYSTEMCALL_NAMES = {
        {IO_SETUP, "IO_SETUP"},
        {IO_DESTROY, "IO_DESTROY"},
        {IO_SUBMIT, "IO_SUBMIT"},
        {IO_CANCEL, "IO_CANCEL"},
        {IO_GETEVENTS, "IO_GETEVENTS"},
        {SETXATTR, "SETXATTR"},
        {LSETXATTR, "LSETXATTR"},
        {FSETXATTR, "FSETXATTR"},
        {GETXATTR, "GETXATTR"},
        {LGETXATTR, "LGETXATTR"},
        {FGETXATTR, "FGETXATTR"},
        {LISTXATTR, "LISTXATTR"},
        {LLISTXATTR, "LLISTXATTR"},
        {FLISTXATTR, "FLISTXATTR"},
        {REMOVEXATTR, "REMOVEXATTR"},
        {LREMOVEXATTR, "LREMOVEXATTR"},
        {FREMOVEXATTR, "FREMOVEXATTR"},
        {GETCWD, "GETCWD"},
        {LOOKUP_DCOOKIE, "LOOKUP_DCOOKIE"},
        {EVENTFD2, "EVENTFD2"},
        {EPOLL_CREATE1, "EPOLL_CREATE1"},
        {EPOLL_CTL, "EPOLL_CTL"},
        {EPOLL_PWAIT, "EPOLL_PWAIT"},
        {DUP, "DUP"},
        {DUP3, "DUP3"},
        {FCNTL64, "FCNTL64"},
        {INOTIFY_INIT1, "INOTIFY_INIT1"},
        {INOTIFY_ADD_WATCH, "INOTIFY_ADD_WATCH"},
        {INOTIFY_RM_WATCH, "INOTIFY_RM_WATCH"},
        {IOCTL, "IOCTL"},
        {IOPRIO_SET, "IOPRIO_SET"},
        {IOPRIO_GET, "IOPRIO_GET"},
        {FLOCK, "FLOCK"},
        {MKNODAT, "MKNODAT"},
        {MKDIRAT, "MKDIRAT"},
        {UNLINKAT, "UNLINKAT"},
        {SYMLINKAT, "SYMLINKAT"},
        {LINKAT, "LINKAT"},
        {RENAMEAT, "RENAMEAT"},
        {UMOUNT, "UMOUNT"},
        {MOUNT, "MOUNT"},
        {PIVOT_ROOT, "PIVOT_ROOT"},
        {NI_SYSCALL, "NI_SYSCALL"},
        {STATFS64, "STATFS64"},
        {FSTATFS64, "FSTATFS64"},
        {TRUNCATE64, "TRUNCATE64"},
        {FTRUNCATE64, "FTRUNCATE64"},
        {FALLOCATE, "FALLOCATE"},
        {FACCESSAT, "FACCESSAT"},
        {CHDIR, "CHDIR"},
        {FCHDIR, "FCHDIR"},
        {CHROOT, "CHROOT"},
        {FCHMOD, "FCHMOD"},
        {FCHMODAT, "FCHMODAT"},
        {FCHOWNAT, "FCHOWNAT"},
        {FCHOWN, "FCHOWN"},
        {OPENAT, "OPENAT"},
        {CLOSE, "CLOSE"},
        {VHANGUP, "VHANGUP"},
        {PIPE2, "PIPE2"},
        {QUOTACTL, "QUOTACTL"},
        {GETDENTS64, "GETDENTS64"},
        {LSEEK, "LSEEK"},
        {READ, "READ"},
        {WRITE, "WRITE"},
        {READV, "READV"},
        {WRITEV, "WRITEV"},
        {PREAD64, "PREAD64"},
        {PWRITE64, "PWRITE64"},
        {PREADV, "PREADV"},
        {PWRITEV, "PWRITEV"},
        {SENDFILE64, "SENDFILE64"},
        {PSELECT6_TIME32, "PSELECT6_TIME32"},
        {PPOLL_TIME32, "PPOLL_TIME32"},
        {SIGNALFD4, "SIGNALFD4"},
        {VMSPLICE, "VMSPLICE"},
        {SPLICE, "SPLICE"},
        {TEE, "TEE"},
        {READLINKAT, "READLINKAT"},
        {NEWFSTATAT, "NEWFSTATAT"},
        {NEWFSTAT, "NEWFSTAT"},
        {SYNC, "SYNC"},
        {FSYNC, "FSYNC"},
        {FDATASYNC, "FDATASYNC"},
        {SYNC_FILE_RANGE, "SYNC_FILE_RANGE"},
        {TIMERFD_CREATE, "TIMERFD_CREATE"},
        {TIMERFD_SETTIME, "TIMERFD_SETTIME"},
        {TIMERFD_GETTIME, "TIMERFD_GETTIME"},
        {UTIMENSAT, "UTIMENSAT"},
        {ACCT, "ACCT"},
        {CAPGET, "CAPGET"},
        {CAPSET, "CAPSET"},
        {PERSONALITY, "PERSONALITY"},
        {EXIT, "EXIT"},
        {EXIT_GROUP, "EXIT_GROUP"},
        {WAITID, "WAITID"},
        {SET_TID_ADDRESS, "SET_TID_ADDRESS"},
        {UNSHARE, "UNSHARE"},
        {FUTEX, "FUTEX"},
        {SET_ROBUST_LIST, "SET_ROBUST_LIST"},
        {GET_ROBUST_LIST, "GET_ROBUST_LIST"},
        {NANOSLEEP, "NANOSLEEP"},
        {GETITIMER, "GETITIMER"},
        {SETITIMER, "SETITIMER"},
        {KEXEC_LOAD, "KEXEC_LOAD"},
        {INIT_MODULE, "INIT_MODULE"},
        {DELETE_MODULE, "DELETE_MODULE"},
        {TIMER_CREATE, "TIMER_CREATE"},
        {TIMER_GETTIME, "TIMER_GETTIME"},
        {TIMER_GETOVERRUN, "TIMER_GETOVERRUN"},
        {TIMER_SETTIME, "TIMER_SETTIME"},
        {TIMER_DELETE, "TIMER_DELETE"},
        {CLOCK_SETTIME, "CLOCK_SETTIME"},
        {CLOCK_GETTIME, "CLOCK_GETTIME"},
        {CLOCK_GETRES, "CLOCK_GETRES"},
        {CLOCK_NANOSLEEP, "CLOCK_NANOSLEEP"},
        {SYSLOG, "SYSLOG"},
        {PTRACE, "PTRACE"},
        {SCHED_SETPARAM, "SCHED_SETPARAM"},
        {SCHED_SETSCHEDULER, "SCHED_SETSCHEDULER"},
        {SCHED_GETSCHEDULER, "SCHED_GETSCHEDULER"},
        {SCHED_GETPARAM, "SCHED_GETPARAM"},
        {SCHED_SETAFFINITY, "SCHED_SETAFFINITY"},
        {SCHED_GETAFFINITY, "SCHED_GETAFFINITY"},
        {SCHED_YIELD, "SCHED_YIELD"},
        {SCHED_GET_PRIORITY_MAX, "SCHED_GET_PRIORITY_MAX"},
        {SCHED_GET_PRIORITY_MIN, "SCHED_GET_PRIORITY_MIN"},
        {SCHED_RR_GET_INTERVAL, "SCHED_RR_GET_INTERVAL"},
        {RESTART_SYSCALL, "RESTART_SYSCALL"},
        {KILL, "KILL"},
        {TKILL, "TKILL"},
        {TGKILL, "TGKILL"},
        {SIGALTSTACK, "SIGALTSTACK"},
        {RT_SIGSUSPEND, "RT_SIGSUSPEND"},
        {RT_SIGACTION, "RT_SIGACTION"},
        {RT_SIGPROCMASK, "RT_SIGPROCMASK"},
        {RT_SIGPENDING, "RT_SIGPENDING"},
        {RT_SIGTIMEDWAIT_TIME32, "RT_SIGTIMEDWAIT_TIME32"},
        {RT_SIGQUEUEINFO, "RT_SIGQUEUEINFO"},
        {SETPRIORITY, "SETPRIORITY"},
        {GETPRIORITY, "GETPRIORITY"},
        {REBOOT, "REBOOT"},
        {SETREGID, "SETREGID"},
        {SETGID, "SETGID"},
        {SETREUID, "SETREUID"},
        {SETUID, "SETUID"},
        {SETRESUID, "SETRESUID"},
        {GETRESUID, "GETRESUID"},
        {SETRESGID, "SETRESGID"},
        {GETRESGID, "GETRESGID"},
        {SETFSUID, "SETFSUID"},
        {SETFSGID, "SETFSGID"},
        {TIMES, "TIMES"},
        {SETPGID, "SETPGID"},
        {GETPGID, "GETPGID"},
        {GETSID, "GETSID"},
        {SETSID, "SETSID"},
        {GETGROUPS, "GETGROUPS"},
        {SETGROUPS, "SETGROUPS"},
        {NEWUNAME, "NEWUNAME"},
        {SETHOSTNAME, "SETHOSTNAME"},
        {SETDOMAINNAME, "SETDOMAINNAME"},
        {GETRLIMIT, "GETRLIMIT"},
        {SETRLIMIT, "SETRLIMIT"},
        {GETRUSAGE, "GETRUSAGE"},
        {UMASK, "UMASK"},
        {PRCTL, "PRCTL"},
        {GETCPU, "GETCPU"},
        {GETTIMEOFDAY, "GETTIMEOFDAY"},
        {SETTIMEOFDAY, "SETTIMEOFDAY"},
        {ADJTIMEX, "ADJTIMEX"},
        {GETPID, "GETPID"},
        {GETPPID, "GETPPID"},
        {GETUID, "GETUID"},
        {GETEUID, "GETEUID"},
        {GETGID, "GETGID"},
        {GETEGID, "GETEGID"},
        {GETTID, "GETTID"},
        {SYSINFO, "SYSINFO"},
        {MQ_OPEN, "MQ_OPEN"},
        {MQ_UNLINK, "MQ_UNLINK"},
        {MQ_TIMEDSEND, "MQ_TIMEDSEND"},
        {MQ_TIMEDRECEIVE, "MQ_TIMEDRECEIVE"},
        {MQ_NOTIFY, "MQ_NOTIFY"},
        {MQ_GETSETATTR, "MQ_GETSETATTR"},
        {MSGGET, "MSGGET"},
        {MSGCTL, "MSGCTL"},
        {MSGRCV, "MSGRCV"},
        {MSGSND, "MSGSND"},
        {SEMGET, "SEMGET"},
        {SEMCTL, "SEMCTL"},
        {SEMTIMEDOP, "SEMTIMEDOP"},
        {SEMOP, "SEMOP"},
        {SHMGET, "SHMGET"},
        {SHMCTL, "SHMCTL"},
        {SHMAT, "SHMAT"},
        {SHMDT, "SHMDT"},
        {SOCKET, "SOCKET"},
        {SOCKETPAIR, "SOCKETPAIR"},
        {BIND, "BIND"},
        {LISTEN, "LISTEN"},
        {ACCEPT, "ACCEPT"},
        {CONNECT, "CONNECT"},
        {GETSOCKNAME, "GETSOCKNAME"},
        {GETPEERNAME, "GETPEERNAME"},
        {SENDTO, "SENDTO"},
        {RECVFROM, "RECVFROM"},
        {SETSOCKOPT, "SETSOCKOPT"},
        {GETSOCKOPT, "GETSOCKOPT"},
        {SHUTDOWN, "SHUTDOWN"},
        {SENDMSG, "SENDMSG"},
        {RECVMSG, "RECVMSG"},
        {READAHEAD, "READAHEAD"},
        {BRK, "BRK"},
        {MUNMAP, "MUNMAP"},
        {MREMAP, "MREMAP"},
        {ADD_KEY, "ADD_KEY"},
        {REQUEST_KEY, "REQUEST_KEY"},
        {KEYCTL, "KEYCTL"},
        {CLONE, "CLONE"},
        {EXECVE, "EXECVE"},
        {MMAP, "MMAP"},
        {FADVISE64_64, "FADVISE64_64"},
        {SWAPON, "SWAPON"},
        {SWAPOFF, "SWAPOFF"},
        {MPROTECT, "MPROTECT"},
        {MSYNC, "MSYNC"},
        {MLOCK, "MLOCK"},
        {MUNLOCK, "MUNLOCK"},
        {MLOCKALL, "MLOCKALL"},
        {MUNLOCKALL, "MUNLOCKALL"},
        {MINCORE, "MINCORE"},
        {MADVISE, "MADVISE"},
        {REMAP_FILE_PAGES, "REMAP_FILE_PAGES"},
        {MBIND, "MBIND"},
        {GET_MEMPOLICY, "GET_MEMPOLICY"},
        {SET_MEMPOLICY, "SET_MEMPOLICY"},
        {MIGRATE_PAGES, "MIGRATE_PAGES"},
        {MOVE_PAGES, "MOVE_PAGES"},
        {RT_TGSIGQUEUEINFO, "RT_TGSIGQUEUEINFO"},
        {PERF_EVENT_OPEN, "PERF_EVENT_OPEN"},
        {ACCEPT4, "ACCEPT4"},
        {RECVMMSG_TIME32, "RECVMMSG_TIME32"},
        {WAIT4, "WAIT4"},
        {PRLIMIT64, "PRLIMIT64"},
        {FANOTIFY_INIT, "FANOTIFY_INIT"},
        {FANOTIFY_MARK, "FANOTIFY_MARK"},
        {NAME_TO_HANDLE_AT, "NAME_TO_HANDLE_AT"},
        {OPEN_BY_HANDLE_AT, "OPEN_BY_HANDLE_AT"},
        {CLOCK_ADJTIME, "CLOCK_ADJTIME"},
        {SYNCFS, "SYNCFS"},
        {SETNS, "SETNS"},
        {SENDMMSG, "SENDMMSG"},
        {PROCESS_VM_READV, "PROCESS_VM_READV"},
        {KCMP, "KCMP"},
        {FINIT_MODULE, "FINIT_MODULE"},
        {SCHED_SETATTR, "SCHED_SETATTR"},
        {SCHED_GETATTR, "SCHED_GETATTR"},
        {RENAMEAT2, "RENAMEAT2"},
        {SECCOMP, "SECCOMP"},
        {GETRANDOM, "GETRANDOM"},
        {MEMFD_CREATE, "MEMFD_CREATE"},
        {BPF, "BPF"},
        {EXECVEAT, "EXECVEAT"},
        {USERFAULTFD, "USERFAULTFD"},
        {MEMBARRIER, "MEMBARRIER"},
        {MLOCK2, "MLOCK2"},
        {COPY_FILE_RANGE, "COPY_FILE_RANGE"},
        {PREADV2, "PREADV2"},
        {PWRITEV2, "PWRITEV2"},
        {PKEY_MPROTECT, "PKEY_MPROTECT"},
        {PKEY_ALLOC, "PKEY_ALLOC"},
        {PKEY_FREE, "PKEY_FREE"},
        {STATX, "STATX"},
        {IO_PGETEVENTS, "IO_PGETEVENTS"},
        {RSEQ, "RSEQ"},
        {KEXEC_FILE_LOAD, "KEXEC_FILE_LOAD"},
        {PIDFD_SEND_SIGNAL, "PIDFD_SEND_SIGNAL"},
        {IO_URING_SETUP, "IO_URING_SETUP"},
        {IO_URING_ENTER, "IO_URING_ENTER"},
        {IO_URING_REGISTER, "IO_URING_REGISTER"},
        {OPEN_TREE, "OPEN_TREE"},
        {MOVE_MOUNT, "MOVE_MOUNT"},
        {FSOPEN, "FSOPEN"},
        {FSCONFIG, "FSCONFIG"},
        {FSMOUNT, "FSMOUNT"},
        {FSPICK, "FSPICK"},
        {PIDFD_OPEN, "PIDFD_OPEN"},
        {CLONE3, "CLONE3"},
        {CLOSE_RANGE, "CLOSE_RANGE"},
        {OPENAT2, "OPENAT2"},
        {PIDFD_GETFD, "PIDFD_GETFD"},
        {FACCESSAT2, "FACCESSAT2"},
        {PROCESS_MADVISE, "PROCESS_MADVISE"}
    };

    const std::string& getSystemCallName(SYSTEMCALL syscall) {
        static const std::string unknown_syscall = "UNKNOWN_SYSCALL";
        auto it = SYSTEMCALL_NAMES.find(syscall);
        if (it != SYSTEMCALL_NAMES.end()) {
            return it->second;
        }
        return unknown_syscall;
    }

}