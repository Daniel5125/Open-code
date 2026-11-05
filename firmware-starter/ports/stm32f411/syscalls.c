/* Llamadas al sistema mínimas para newlib-nano. El firmware no usa archivos ni procesos;
   la salida de texto va por hal_uart_write(), no por printf/stdout. */
#include <errno.h>
#include <sys/stat.h>

int _close(int fd);
int _fstat(int fd, struct stat *st);
int _getpid(void);
int _isatty(int fd);
int _kill(int pid, int sig);
int _lseek(int fd, int off, int dir);
int _read(int fd, char *p, int n);
int _write(int fd, const char *p, int n);

int _close(int fd)
{
    (void)fd;
    return -1;
}
int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}
int _getpid(void)
{
    return 1;
}
int _isatty(int fd)
{
    (void)fd;
    return 1;
}
int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}
int _lseek(int fd, int off, int dir)
{
    (void)fd;
    (void)off;
    (void)dir;
    return 0;
}
int _read(int fd, char *p, int n)
{
    (void)fd;
    (void)p;
    (void)n;
    return 0;
}
int _write(int fd, const char *p, int n)
{
    (void)fd;
    (void)p;
    return n;
}
