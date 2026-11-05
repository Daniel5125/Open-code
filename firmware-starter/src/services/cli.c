#include "cli.h"

#include "hal/hal.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static const cli_cmd_t *s_cmds;
static size_t s_ncmds;
static char s_line[CLI_LINE_MAX];
static size_t s_len;

static void puts_raw(const char *s)
{
    hal_uart_write((const uint8_t *)s, strlen(s));
}

void cli_printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0) hal_uart_write((const uint8_t *)buf, (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1u);
}

static int cmd_help(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    cli_printf("Comandos:\r\n  %-10s %s\r\n", "help", "esta ayuda");
    for (size_t i = 0; i < s_ncmds; i++) cli_printf("  %-10s %s\r\n", s_cmds[i].name, s_cmds[i].help);
    return 0;
}

void cli_init(const cli_cmd_t *cmds, size_t n)
{
    s_cmds = cmds;
    s_ncmds = n;
    s_len = 0;
}

int cli_execute(char *line)
{
    char *argv[CLI_ARGS_MAX];
    int argc = 0;
    for (char *tok = strtok(line, " \t"); tok && argc < (int)CLI_ARGS_MAX; tok = strtok(NULL, " \t"))
        argv[argc++] = tok;
    if (argc == 0) return 0;
    if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "?") == 0) return cmd_help(argc, argv);
    for (size_t i = 0; i < s_ncmds; i++)
        if (strcmp(argv[0], s_cmds[i].name) == 0) return s_cmds[i].fn(argc, argv);
    cli_printf("Comando desconocido: %s (escribe help)\r\n", argv[0]);
    return -1;
}

void cli_poll(void)
{
    uint8_t buf[16];
    size_t n;
    while ((n = hal_uart_read(buf, sizeof buf)) > 0u) {
        for (size_t i = 0; i < n; i++) {
            char c = (char)buf[i];
            if (c == '\r' || c == '\n') {
                if (s_len == 0u) continue;
                puts_raw("\r\n");
                s_line[s_len] = '\0';
                s_len = 0;
                cli_execute(s_line);
                puts_raw("> ");
            } else if ((c == '\b' || c == 0x7f) && s_len > 0u) {
                s_len--;
                puts_raw("\b \b");
            } else if (c >= ' ' && c < 0x7f && s_len < CLI_LINE_MAX - 1u) {
                s_line[s_len++] = c;
                hal_uart_write((const uint8_t *)&c, 1); /* eco */
            }
        }
    }
}
