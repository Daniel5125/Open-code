/**
 * @file cli.h
 * @brief Consola de comandos por UART: edición de línea, tabla de comandos y ayuda.
 */
#ifndef SVC_CLI_H
#define SVC_CLI_H

#include <stddef.h>

#define CLI_LINE_MAX 64u
#define CLI_ARGS_MAX 6u

typedef int (*cli_handler_t)(int argc, char **argv);

typedef struct {
    const char *name;
    const char *help;
    cli_handler_t fn;
} cli_cmd_t;

void cli_init(const cli_cmd_t *cmds, size_t n);
/** Procesa los bytes recibidos (llamar periódicamente). */
void cli_poll(void);
/** Ejecuta una línea completa (útil en pruebas). @return código del comando, o −1 si no existe. */
int cli_execute(char *line);
/** printf mínimo hacia la UART (sin dependencias de la libc completa). */
void cli_printf(const char *fmt, ...);

#endif
