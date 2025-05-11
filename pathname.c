#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// Busca el inodo de una ruta absoluta
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    // Verifica si es ruta absoluta
    if (pathname[0] != '/') return -1;
    // Caso raíz
    if (pathname[1] == '\0') return ROOT_INUMBER;

    int cur = ROOT_INUMBER;
    char *copia = strdup(pathname);
    if (!copia) return -1;

    // Divide componentes
    char *ctx;
    char *comp = strtok_r(copia + 1, "/", &ctx);
    while (comp) {
        // Ignora vacíos
        if (*comp == '\0') {
            comp = strtok_r(NULL, "/", &ctx);
            continue;
        }
        // Busca en directorio actual
        struct direntv6 ent;
        if (directory_findname(fs, comp, cur, &ent) != 0) {
            free(copia);
            return -1;
        }
        // Actualiza inodo
        cur = ent.d_inumber;
        // Siguiente componente
        comp = strtok_r(NULL, "/", &ctx);
    }

    free(copia);
    return cur;
}