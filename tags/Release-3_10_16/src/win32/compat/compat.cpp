//                              -*- Mode: C++ -*- 
// compat.cpp -- compatibilty layer to make bacula-fd run
//               natively under windows
//
// Copyright (C)2004 Raider Solutions, Inc
// 
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//   General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free
//   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//   MA 02111-1307, USA.
//
// Author          : Christopher S. Hull
// Created On      : Sat Jan 31 15:55:00 2004
// Last Modified By: Christopher S. Hull
// Last Modified On: Tue Feb 24 11:27:33 2004
// Update Count    : 665
// $Id: compat.cpp,v 1.1 2004-04-16 13:18:45 kerns Exp $

#include <stdio.h>

#include "compat.h"
#include "pthread.h"

#define USE_WIN32_COMPAT_IO 1

extern void d_msg(const char *file, int line, int level, const char *fmt,...);
extern DWORD   g_platform_id;

// from CYGWIN (should be diff between Jan 1 1601 and Jan 1 1970
#define WIN32_FILETIME_ADJUST 0x19DB1DED53E8000I64

#define WIN32_FILETIME_SCALE  10000000             // 100ns/second

extern "C" void
cygwin_conv_to_win32_path(const char *name, char *win32_name)
{
    const char *fname = name;
    while (*name) {
        /* Check for Unix separator and convert to Win32 */
        if (*name == '/') {
            *win32_name++ = '\\';     /* convert char */
        /* If Win32 separated that is "quoted", remove quote */
        } else if (*name == '\\' && name[1] == '\\') {
            *win32_name++ = '\\';
            name++;                   /* skip first \ */
        } else {
            *win32_name++ = *name;    /* copy character */
        }
        name++;
    }
    /* Strip any trailing slash, if we stored something */
    if (*fname != 0 && win32_name[-1] == '\\') {
        win32_name[-1] = 0;
    } else {
        *win32_name = 0;
    }
}


void
wchar_win32_path(const char *name, WCHAR *win32_name)
{
    const char *fname = name;
    while (*name) {
        /* Check for Unix separator and convert to Win32 */
        if (*name == '/') {
            *win32_name++ = '\\';     /* convert char */
        /* If Win32 separated that is "quoted", remove quote */
        } else if (*name == '\\' && name[1] == '\\') {
            *win32_name++ = '\\';
            name++;                   /* skip first \ */
        } else {
            *win32_name++ = *name;    /* copy character */
        }
        name++;
    }
    /* Strip any trailing slash, if we stored something */
    if (*fname != 0 && win32_name[-1] == '\\') {
        win32_name[-1] = 0;
    } else {
        *win32_name = 0;
    }
}

int
umask(int)
{
    return 0;
}

int
chmod(const char *, mode_t)
{
    return 0;
}

int
chown(const char *k, uid_t, gid_t)
{
    return 0;
}

int
lchown(const char *k, uid_t, gid_t)
{
    return 0;
}



long int
random(void)
{
    return rand();
}

void
srandom(unsigned int seed)
{
    srand(seed);
}
// /////////////////////////////////////////////////////////////////
// convert from Windows concept of time to Unix concept of time
// /////////////////////////////////////////////////////////////////
void
cvt_utime_to_ftime(const time_t  &time, FILETIME &wintime)
{
    uint64_t mstime = time;
    mstime *= WIN32_FILETIME_SCALE;
    mstime += WIN32_FILETIME_ADJUST;

    wintime.dwLowDateTime = (DWORD)(mstime & 0xffffffffI64);
    wintime.dwHighDateTime = (DWORD) ((mstime>>32)& 0xffffffffUL);
}

time_t
cvt_ftime_to_utime(const FILETIME &time)
{
    uint64_t mstime = time.dwHighDateTime;
    mstime <<= 32;
    mstime |= time.dwLowDateTime;

    mstime -= WIN32_FILETIME_ADJUST;
    mstime /= WIN32_FILETIME_SCALE; // convert to seconds.

    return (time_t) (mstime & 0xffffffff);
}

static const char *
errorString(void)
{
    LPVOID lpMsgBuf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default lang
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL);

    return (const char *) lpMsgBuf;
}

static int
statDir(const char *file, struct stat *sb)
{
    WIN32_FIND_DATA info;       // window's file info

    if (file[1] == ':' && file[2] == 0) {
        d_msg(__FILE__, __LINE__, 99, "faking ROOT attrs(%s).\n", file);
        sb->st_mode = S_IFDIR;
        sb->st_mode |= S_IREAD|S_IEXEC|S_IWRITE;
        time(&sb->st_ctime);
        time(&sb->st_mtime);
        time(&sb->st_atime);
        return 0;
    }
    HANDLE h = FindFirstFile(file, &info);

    if (h == INVALID_HANDLE_VALUE) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99, "FindFirstFile(%s):%s\n", file, err);
        LocalFree((void *)err);
        errno = GetLastError();
        return -1;
    }

    sb->st_mode = 0777;               /* start with everything */
    if (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        sb->st_mode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
    if (info.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        sb->st_mode &= ~S_IRWXO; /* remove everything for other */
    if (info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        sb->st_mode |= S_ISVTX; /* use sticky bit -> hidden */
    sb->st_mode |= S_IFDIR;

    sb->st_size = info.nFileSizeHigh;
    sb->st_size <<= 32;
    sb->st_size |= info.nFileSizeLow;
    sb->st_blksize = 4096;
    sb->st_blocks = (uint32_t)(sb->st_size + 4095)/4096;

    sb->st_atime = cvt_ftime_to_utime(info.ftLastAccessTime);
    sb->st_mtime = cvt_ftime_to_utime(info.ftLastWriteTime);
    sb->st_ctime = cvt_ftime_to_utime(info.ftLastWriteTime);
    FindClose(h);

    return 0;
}

static int
stat2(const char *file, struct stat *sb)
{
    BY_HANDLE_FILE_INFORMATION info;
    HANDLE h;
    int rval = 0;
    char tmpbuf[1024];
    cygwin_conv_to_win32_path(file, tmpbuf);

    DWORD attr = GetFileAttributes(tmpbuf);

    if (attr == -1) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "GetFileAttrubtes(%s): %s\n", tmpbuf, err);
        LocalFree((void *)err);
        errno = GetLastError();
        return -1;
    }
    
    if (attr & FILE_ATTRIBUTE_DIRECTORY) 
        return statDir(tmpbuf, sb);

    h = CreateFile(tmpbuf, GENERIC_READ,
                   FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "Cannot open file for stat (%s):%s\n", tmpbuf, err);
        LocalFree((void *)err);
        rval = -1;
        errno = GetLastError();
        goto error;
    }
    
    if (!GetFileInformationByHandle(h, &info)) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "GetfileInformationByHandle(%s): %s\n", tmpbuf, err);
        LocalFree((void *)err);
        rval = -1;
        errno = GetLastError();
        goto error;
    }
    
    sb->st_dev = info.dwVolumeSerialNumber;
    sb->st_ino = info.nFileIndexHigh;
    sb->st_ino <<= 32;
    sb->st_ino |= info.nFileIndexLow;
    sb->st_nlink = (short)info.nNumberOfLinks;

    sb->st_mode = 0777;               /* start with everything */
    if (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        sb->st_mode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
    if (info.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        sb->st_mode &= ~S_IRWXO; /* remove everything for other */
    if (info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        sb->st_mode |= S_ISVTX; /* use sticky bit -> hidden */
    sb->st_mode |= S_IFREG;

    sb->st_size = info.nFileSizeHigh;
    sb->st_size <<= 32;
    sb->st_size |= info.nFileSizeLow;
    sb->st_blksize = 4096;
    sb->st_blocks = (uint32_t)(sb->st_size + 4095)/4096;
    sb->st_atime = cvt_ftime_to_utime(info.ftLastAccessTime);
    sb->st_mtime = cvt_ftime_to_utime(info.ftLastWriteTime);
    sb->st_ctime = cvt_ftime_to_utime(info.ftLastWriteTime);

error:
    CloseHandle(h);
    return rval;
}

int
stat(const char *file, struct stat *sb)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    errno = 0;


    memset(sb, 0, sizeof(*sb));

    if (g_platform_id == VER_PLATFORM_WIN32_WINDOWS)
        return stat2(file, sb);

    // otherwise we're on NT
#if 0
    WCHAR buf[32767];
    buf[0] = '\\';
    buf[1] = '\\';
    buf[2] = '?';
    buf[3] = '\\';

    wchar_win32_path(file, buf+4);

    if (!GetFileAttributesExW((WCHAR *)buf, GetFileExInfoStandard, &data))
        return stat2(file, sb);
#else
    if (!GetFileAttributesEx(file, GetFileExInfoStandard, &data))
        return stat2(file, sb);
#endif
    
    sb->st_mode = 0777;               /* start with everything */
    if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        sb->st_mode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
    if (data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        sb->st_mode &= ~S_IRWXO; /* remove everything for other */
    if (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        sb->st_mode |= S_ISVTX; /* use sticky bit -> hidden */

    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        sb->st_mode |= S_IFDIR;
    else
        sb->st_mode |= S_IFREG;

    sb->st_nlink = 1;
    sb->st_size = data.nFileSizeHigh;
    sb->st_size <<= 32;
    sb->st_size |= data.nFileSizeLow;
    sb->st_blksize = 4096;
    sb->st_blocks = (uint32_t)(sb->st_size + 4095)/4096;
    sb->st_atime = cvt_ftime_to_utime(data.ftLastAccessTime);
    sb->st_mtime = cvt_ftime_to_utime(data.ftLastWriteTime);
    sb->st_ctime = cvt_ftime_to_utime(data.ftLastWriteTime);
    return 0;
}

int
lstat(const char *file, struct stat *sb)
{
    return stat(file, sb);
}

void
sleep(int sec)
{
    Sleep(sec * 1000);
}    

int
geteuid(void)
{
    return 0;
}

int
execvp(const char *, char *[]) {
    return -1;
}


int
fork(void)
{
    return -1;
}

int
pipe(int[])
{
    return -1;
}

int
waitpid(int, int*, int)
{
    return -1;
}

int
readlink(const char *, char *, int)
{
    return -1;
}


int
strcasecmp(const char *s1, const char *s2)
{
    register int ch1, ch2;
    
    if (s1==s2)
        return 0;       /* strings are equal if same object. */
    else if (!s1)
        return -1;
    else if (!s2)
        return 1;
    do
    {
        ch1 = *s1;
        ch2 = *s2;
        s1++;
        s2++;
    } while (ch1 != 0 && tolower(ch1) == tolower(ch2));
    
  return(ch1 - ch2);
}

int
strncasecmp(const char *s1, const char *s2, int len)
{
    register int ch1, ch2;
    
    if (s1==s2)
        return 0;       /* strings are equal if same object. */
    else if (!s1)
        return -1;
    else if (!s2)
        return 1;
    do
    {
        ch1 = *s1;
        ch2 = *s2;
        s1++;
        s2++;
    } while (len-- && ch1 != 0 && tolower(ch1) == tolower(ch2));
    
    return(ch1 - ch2);
}

int
gettimeofday(struct timeval *tv, struct timezone *)
{
    SYSTEMTIME now;
    FILETIME tmp;
    GetSystemTime(&now);

    if (tv == NULL) return -1;
    if (!SystemTimeToFileTime(&now, &tmp)) return -1;

    int64_t _100nsec = tmp.dwHighDateTime;
    _100nsec <<= 32;
    _100nsec |= tmp.dwLowDateTime;
    _100nsec -= WIN32_FILETIME_ADJUST;
    
    tv->tv_sec =(long) (_100nsec / 10000000);
    tv->tv_usec = (long) ((_100nsec % 10000000)/10);
    return 0;
    
}

int
syslog(int, const char *, const char *)
{
    return 0;
}

struct passwd *
getpwuid(uid_t)
{
    return NULL;
}

struct group *
getgrgid(uid_t)
{
    return NULL;
}


// implement opendir/readdir/closedir on top of window's API
typedef struct _dir
{
    WIN32_FIND_DATA data;       // window's file info
    const char *spec;           // the directory we're traversing
    HANDLE      dirh;           // the search handle
    BOOL        valid;          // the info in data field is valid
    UINT32      offset;         // pseudo offset for d_off
} _dir;

DIR *
opendir(const char *path)
{
    int max_len = strlen(path) + 16;
    _dir *rval = NULL;
    if (path == NULL) return NULL;
    
    rval = (_dir *)malloc(sizeof(_dir));
    if (rval == NULL) return NULL;
    char *tspec = (char *)malloc(max_len);
    if (tspec == NULL) goto err1;
    
    if (g_platform_id != VER_PLATFORM_WIN32_WINDOWS) {
        // allow path to be 32767 bytes
        tspec[0] = '\\';
        tspec[1] = '\\';
        tspec[2] = '?';
        tspec[3] = '\\';
        tspec[4] = 0;
        cygwin_conv_to_win32_path(path, tspec+4); 
    } else {
        cygwin_conv_to_win32_path(path, tspec);
    }
    strncat(tspec, "\\*", max_len);
    rval->spec = tspec;

    rval->dirh = FindFirstFile(rval->spec, &rval->data);
    d_msg(__FILE__, __LINE__,
          99, "opendir(%s)\n\tspec=%s,\n\tFindFirstFile returns %d\n",
          path, rval->spec, rval->dirh);

    rval->offset = 0;
    if (rval->dirh == INVALID_HANDLE_VALUE)
        goto err;
    rval->valid = 1;
    d_msg(__FILE__, __LINE__,
          99, "\tFirstFile=%s\n", rval->data.cFileName);
    return (DIR *)rval;
err:
    free((void *)rval->spec);
err1:
    free(rval);
    return NULL;
}
    
int
closedir(DIR *dirp)
{
    _dir *dp = (_dir *)dirp;
    FindClose(dp->dirh);
    free((void *)dp->spec);
    free((void *)dp);
    return 0;
}

/*
  typedef struct _WIN32_FIND_DATA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    TCHAR cFileName[MAX_PATH];
    TCHAR cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA;
*/

static int
copyin(struct dirent &dp, const char *fname)
{
    dp.d_ino = 0;
    dp.d_reclen = 0;
    char *cp = dp.d_name;
    while (*fname) {
        *cp++ = *fname++;
        dp.d_reclen++;
    }
        *cp = 0;
    return dp.d_reclen;
}
int
readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{

    _dir *dp = (_dir *)dirp;
    if (dp->valid) {
        entry->d_off = dp->offset;
        dp->offset += copyin(*entry, dp->data.cFileName);
        *result = entry;              /* return entry address */
        d_msg(__FILE__, __LINE__,
              99, "readdir_r(%p, { d_name=\"%s\", d_reclen=%d, d_off=%d\n",
              dirp, entry->d_name, entry->d_reclen, entry->d_off);
    } else {
//      d_msg(__FILE__, __LINE__, 99, "readdir_r !valid\n");
        return -1;
    }
    dp->valid = FindNextFile(dp->dirh, &dp->data);
    return 0;
}


int
inet_aton(const char *a, struct in_addr *inp)
{

    const char *cp = a;
    uint32_t acc = 0, tmp = 0;
    int dotc = 0;
    if (!isdigit(*a)) return 0;
    while (*cp) {
        if (isdigit(*cp))
            tmp = (tmp * 10) + (*cp -'0');
        else if (*cp == '.') {
            if (tmp > 255) return 0;
            acc = (acc << 8) + tmp;
            dotc++;
        }
        else return 0;
    }

    if (dotc != 3) return 0;
    inp->s_addr = acc;
    return 1;
}

int
nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (rem)
        rem->tv_sec = rem->tv_nsec = 0;
    Sleep((req->tv_sec * 1000) + (req->tv_nsec/100000));
    return 0;
}

void
init_signals(void terminate(int sig))
{

}

void
init_stack_dump(void)
{

}


long
pathconf(const char *path, int name)
{
    switch(name) {
    case _PC_PATH_MAX :
        if (strncmp(path, "\\\\?\\", 4) == 0)
            return 32767;
    case _PC_NAME_MAX :
        return 255;
    }

    return -1;
}

int
WSA_Init(void)
{
    WORD wVersionRequested = MAKEWORD( 1, 1);
    WSADATA wsaData;
    
    int err = WSAStartup(wVersionRequested, &wsaData);
    
    
    if (err != 0)
    {
        printf("Can not start Windows Sockets\n");
        return -1;
    }

    return 0;
}


int
win32_chdir(const char *dir)
{
    if (0 == SetCurrentDirectory(dir)) return -1;
    return 0;
}

char *
win32_getcwd(char *buf, int maxlen)
{
   int n =  GetCurrentDirectory(maxlen, buf);

   if (n == 0 || n > maxlen) return NULL;

   if (n+1 > maxlen) return NULL;
   if (n != 3) {
       buf[n] = '\\';
       buf[n+1] = 0;
   }
   
   return buf;
}

#include "mswinver.h"

char WIN_VERSION_LONG[64];
char WIN_VERSION[32];

class winver {
public:
    winver(void);
};

static winver INIT;                     // cause constructor to be called before main()

#include "bacula.h"
#include "jcr.h"

winver::winver(void)
{
    const char *version = "";
    const char *platform = "";
    OSVERSIONINFO osvinfo;
    osvinfo.dwOSVersionInfoSize = sizeof(osvinfo);

    // Get the current OS version
    if (!GetVersionEx(&osvinfo)) {
        version = "Unknown";
        platform = "Unknown";
    }
    else
        switch (_mkversion(osvinfo.dwPlatformId, osvinfo.dwMajorVersion, osvinfo.dwMinorVersion))
        {
        case MS_WINDOWS_95: (version =  "Windows 95"); break;
        case MS_WINDOWS_98: (version =  "Windows 98"); break;
        case MS_WINDOWS_ME: (version =  "Windows ME"); break;
        case MS_WINDOWS_NT4:(version =  "Windows NT 4.0"); platform = "NT"; break;
        case MS_WINDOWS_2K: (version =  "Windows 2000");platform = "NT"; break;
        case MS_WINDOWS_XP: (version =  "Windows XP");platform = "NT"; break;
        case MS_WINDOWS_S2003: (version =  "Windows Server 2003");platform = "NT"; break;
        default: version = "Windows ??"; break;
        }

    bstrncpy(WIN_VERSION_LONG, version, sizeof(WIN_VERSION_LONG));
    snprintf(WIN_VERSION, sizeof(WIN_VERSION), "%s %d.%d.%d",
             platform, osvinfo.dwMajorVersion, osvinfo.dwMinorVersion, osvinfo.dwBuildNumber);

#if 1
    HANDLE h = CreateFile("G:\\foobar", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    CloseHandle(h);
#endif
#if 0
    BPIPE *b = open_bpipe("ls -l", 10, "r");
    char buf[1024];
    while (!feof(b->rfd)) {
        fgets(buf, sizeof(buf), b->rfd);
    }
    close_bpipe(b);
#endif
}
 
BOOL CreateChildProcess(VOID); 
VOID WriteToPipe(VOID); 
VOID ReadFromPipe(VOID); 
VOID ErrorExit(LPTSTR); 
VOID ErrMsg(LPTSTR, BOOL); 
 

const char *
getArgv0(const char *cmdline)
{
    const char *cp = cmdline;

    while (*cp && !isspace(*cp)) cp++;

    int len = cp - cmdline;
    char *rval = (char *)malloc(len+1);

    cp = cmdline;
    char *rp = rval;
    while (len--)
        *rp++ = *cp++;

    *rp = 0;
    return rval;
}

HANDLE
CreateChildProcess(const char *cmdline, HANDLE in, HANDLE out, HANDLE err) 
{ 
    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFO siStartInfo;
    BOOL bFuncRetn = FALSE; 
    
    // Set up members of the PROCESS_INFORMATION structure. 
    
    ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
    
    // Set up members of the STARTUPINFO structure. 
    
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO);
    // setup new process to use supplied handles for stdin,stdout,stderr
    // if supplied handles are not used the send a copy of our STD_HANDLE
    // as appropriate
    siStartInfo.dwFlags = STARTF_USESTDHANDLES;

    if (in != INVALID_HANDLE_VALUE)
        siStartInfo.hStdInput = in;
    else
        siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    
    if (out != INVALID_HANDLE_VALUE)
        siStartInfo.hStdOutput = out;
    else
        siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (err != INVALID_HANDLE_VALUE)
        siStartInfo.hStdError = err;
    else
        siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    // Create the child process. 
    
    char cmdLine[1024];
    char exeFile[1024];
    // retrive the first compont of the command line which should be the
    // executable 
    const char *exeName = getArgv0(cmdline);
    // check to see if absolute path was passed to us already?
    if (exeName[1] != ':'
        || (strchr(cmdline, '/') == NULL
            && strchr(cmdline, '\\') == NULL))
    {
        // only command name so perform search of PATH to find 
        char *file;
        DWORD rval = SearchPath(NULL,
                                exeName,
                                ".exe",
                                sizeof(exeFile),
                                exeFile,
                                &file);
        if (rval == 0)
            return INVALID_HANDLE_VALUE;
        if (rval > sizeof(exeFile))
            return INVALID_HANDLE_VALUE;

    }
    else 
        strcpy(exeFile, exeName);

    // exeFile now has absolute path to program to execute.
    free((void *)exeName);
    // copy original command line to pass to create process
    strcpy(cmdLine, cmdline);
    // try to execute program
    bFuncRetn = CreateProcess(exeFile, 
                              cmdLine, // command line 
                              NULL, // process security attributes 
                              NULL, // primary thread security attributes 
                              TRUE, // handles are inherited 
                              0, // creation flags 
                              NULL, // use parent's environment 
                              NULL, // use parent's current directory 
                              &siStartInfo, // STARTUPINFO pointer 
                              &piProcInfo); // receives PROCESS_INFORMATION
    
    if (bFuncRetn == 0) {
        ErrorExit("CreateProcess failed\n");
        return INVALID_HANDLE_VALUE;
    }
    // we don't need a handle on the process primary thread so we close
    // this now.
    CloseHandle(piProcInfo.hThread);
    
    return piProcInfo.hProcess;
}

 
void
ErrorExit (LPTSTR lpszMessage) 
{ 
    d_msg(__FILE__, __LINE__, 0, "%s", lpszMessage);
} 
 

/*
typedef struct s_bpipe {
   pid_t worker_pid;
   time_t worker_stime;
   int wait;
   btimer_t *timer_id;
   FILE *rfd;
   FILE *wfd;
} BPIPE;
*/

static void
CloseIfValid(HANDLE handle)
{
    if (handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);
}

BPIPE *
open_bpipe(char *prog, int wait, const char *mode)
{
    HANDLE hChildStdinRd, hChildStdinWr, hChildStdinWrDup, 
        hChildStdoutRd, hChildStdoutWr, hChildStdoutRdDup, 
        hInputFile;
    
    SECURITY_ATTRIBUTES saAttr; 

    BOOL fSuccess; 

    hChildStdinRd = hChildStdinWr = hChildStdinWrDup = 
        hChildStdoutRd = hChildStdoutWr = hChildStdoutRdDup = 
        hInputFile = INVALID_HANDLE_VALUE;
    
    BPIPE *bpipe = (BPIPE *)malloc(sizeof(BPIPE));
    memset((void *)bpipe, 0, sizeof(BPIPE));

    int mode_read = (mode[0] == 'r');
    int mode_write = (mode[0] == 'w' || mode[1] == 'w');
    
    
    // Set the bInheritHandle flag so pipe handles are inherited. 
    
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 
    
    if (mode_read) {
        
        // Create a pipe for the child process's STDOUT. 
        if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
            ErrorExit("Stdout pipe creation failed\n");
            goto cleanup;
        }
        // Create noninheritable read handle and close the inheritable read 
        // handle. 
        
        fSuccess = DuplicateHandle(GetCurrentProcess(), hChildStdoutRd,
                                   GetCurrentProcess(), &hChildStdoutRdDup , 0,
                                   FALSE,
                                   DUPLICATE_SAME_ACCESS);
        if ( !fSuccess ) {
            ErrorExit("DuplicateHandle failed");
            goto cleanup;
        }
        
        CloseHandle(hChildStdoutRd);
    }
    
    if (mode_write) {
        
        // Create a pipe for the child process's STDIN. 
        
        if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) {
            ErrorExit("Stdin pipe creation failed\n");
            goto cleanup;
        }
        
        // Duplicate the write handle to the pipe so it is not inherited. 
        fSuccess = DuplicateHandle(GetCurrentProcess(), hChildStdinWr, 
                                   GetCurrentProcess(), &hChildStdinWrDup,
                                   0, 
                                   FALSE,                  // not inherited 
                                   DUPLICATE_SAME_ACCESS); 
        if (!fSuccess) {
            ErrorExit("DuplicateHandle failed");
            goto cleanup;
        }
        
        CloseHandle(hChildStdinWr); 
    }
    // spawn program with redirected handles as appropriate
    bpipe->worker_pid = (pid_t)
        CreateChildProcess(prog, // commandline
                           hChildStdinRd, // stdin HANDLE
                           hChildStdoutWr, // stdout HANDLE
                           hChildStdoutWr);// stderr HANDLE
    
    if ((HANDLE) bpipe->worker_pid == INVALID_HANDLE_VALUE)
        goto cleanup;
    
    bpipe->wait = wait;
    bpipe->worker_stime = time(NULL);
    
    if (mode_read) {
        CloseHandle(hChildStdoutWr); // close our write side so when
                                     // process terminates we can
                                     // detect eof.
        // ugly but convert WIN32 HANDLE to FILE*
        int rfd = _open_osfhandle((long)hChildStdoutRdDup, O_RDONLY);
        bpipe->rfd = _fdopen(rfd, "r");
    }
    if (mode_write) {
        CloseHandle(hChildStdinRd); // close our read side so as not
                                    // to interfre with child's copy
        // ugly but convert WIN32 HANDLE to FILE*
        int wfd = _open_osfhandle((long)hChildStdinWrDup, O_WRONLY);
        bpipe->wfd = _fdopen(wfd, "w");
    }
    
    if (wait > 0) {
        bpipe->timer_id = start_child_timer(bpipe->worker_pid, wait);
    }
    
    return bpipe;
    
cleanup:
    
    CloseIfValid(hChildStdoutRd);
    CloseIfValid(hChildStdoutRdDup);
    CloseIfValid(hChildStdinWr);
    CloseIfValid(hChildStdinWrDup);
    
    free((void *) bpipe);
    
    return NULL;
}

int
kill(int pid, int signal)
{
    int rval = 0;
    if (!TerminateProcess((HANDLE)pid, (UINT) signal))
        rval = -1;
    CloseHandle((HANDLE)pid);
    return rval;
}

int
close_bpipe(BPIPE *bpipe)
{
    int rval = 0;
    if (bpipe->rfd) fclose(bpipe->rfd);
    if (bpipe->wfd) fclose(bpipe->wfd);

    if (bpipe->wait) {
        int remaining_wait = bpipe->wait;
        do 
        {
            DWORD exitCode;
            if (!GetExitCodeProcess((HANDLE)bpipe->worker_pid, &exitCode))
            {
                const char *err = errorString();
                rval = GetLastError();
                d_msg(__FILE__, __LINE__, 0, 
                      "GetExitCode error %s\n", err);
                LocalFree((void *)err);
                break;
            }
            
            if (exitCode == STILL_ACTIVE) {
                bmicrosleep(1, 0);             /* wait one second */
                remaining_wait--;
            }
            else break;
        } while(remaining_wait);
    }
    
    if (bpipe->timer_id) {
        stop_child_timer(bpipe->timer_id);
    }
    free((void *)bpipe);    
    return rval;
}

int
close_wpipe(BPIPE *bpipe)
{
    int stat = 1;
    
    if (bpipe->wfd) {
        fflush(bpipe->wfd);
        if (fclose(bpipe->wfd) != 0) {
            stat = 0;
      }
        bpipe->wfd = NULL;
    }
    return stat;
}

#include "findlib/find.h"

int
utime(const char *fname, struct utimbuf *times)
{
    FILETIME acc, mod;
    char tmpbuf[1024];

    cygwin_conv_to_win32_path(fname, tmpbuf);

    cvt_utime_to_ftime(times->actime, acc);
    cvt_utime_to_ftime(times->modtime, mod);

    HANDLE h = CreateFile(tmpbuf,
                          FILE_WRITE_ATTRIBUTES,
                          FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          0,
                          NULL);

    if (h == INVALID_HANDLE_VALUE) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "Cannot open file for utime(%s,...):%s\n", tmpbuf, err);
        LocalFree((void *)err);
        return -1;
    }

    int rval = SetFileTime(h, NULL, &acc, &mod) ? 0 : -1;

    CloseHandle(h);
    
    return rval;
}

#if USE_WIN32_COMPAT_IO

int
open(const char *file, int flags, int mode)
{
    return _open(file, flags|_O_BINARY, mode);

}

int
close(int fd)
{
    return _close(fd);
}

ssize_t
read(int fd, void *buf, size_t len)
{
    return _read(fd, buf, len);
}

ssize_t
write(int fd, const void *buf, size_t len)
{
    return _write(fd, buf, len);
}

off_t
lseek(int fd, off_t offset, int whence)
{
    return _lseeki64(fd, offset, whence);
}

int
dup2(int fd1, int fd2)
{
    return _dup2(fd1, fd2);
}
#else
int
open(const char *file, int flags, int mode)
{
    DWORD access = 0;
    DWORD shareMode = 0;
    DWORD create = 0;
    DWORD msflags = 0;
    HANDLE foo;
    const char *remap = file;

    if (flags & O_WRONLY) access = GENERIC_WRITE;
    else if (flags & O_RDWR) access = GENERIC_READ|GENERIC_WRITE;
    else access = GENERIC_READ;
    
    if (flags & O_CREAT) create = CREATE_NEW;
    else create = OPEN_EXISTING;
    
    if (flags & O_TRUNC) create = TRUNCATE_EXISTING;
    
    if (!(flags & O_EXCL))
        shareMode = FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE;
    
    if (flags & O_APPEND)
    {
        printf("open...APPEND not implemented yet.");
        exit(-1);
    }
    
    foo = CreateFile(file, access, shareMode, NULL, create, msflags, NULL);
    if (INVALID_HANDLE_VALUE == foo)
        return(int) -1;

    return (int)foo;

}


int
close(int fd)
{
    if (!CloseHandle((HANDLE)fd))
        return -1;

    return 0;
}

ssize_t
write(int fd, const void *data, size_t len)
{
    BOOL status;
    DWORD bwrite;
    status = WriteFile((HANDLE)fd, data, len, &bwrite, NULL);
    if (status) return bwrite;
    return -1;
}


ssize_t
read(int fd, void *data, size_t len)
{
    BOOL status;
    DWORD bread;

    status = ReadFile((HANDLE)fd, data, len, &bread, NULL);
    if (status) return bread;
    return -1;
}

off_t
lseek(int fd, off_t offset, int whence)
{
    DWORD method = 0;
    switch (whence) {
    case SEEK_SET :
        method = FILE_BEGIN;
        break;
    case SEEK_CUR:
        method = FILE_CURRENT;
        break;
    case SEEK_END:
        method = FILE_END;
        break;
    default:
        return -1;
    }
    
    return SetFilePointer((HANDLE)fd, (DWORD)offset, NULL, method);
}

int
dup2(int, int)
{
    return -1;
}


#endif
