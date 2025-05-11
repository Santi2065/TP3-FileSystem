#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include "direntv6.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Busca entrada 'name' en el directorio 'dirinumber' y guarda en dirEnt
int directory_findname(struct unixfilesystem *fs, const char *name,
        int dirinumber, struct direntv6 *dirEnt) {
    struct inode dir_in;
    if (inode_iget(fs, dirinumber, &dir_in) != 0) return -1;  // error inode

    if ((dir_in.i_mode & IFDIR) == 0) return -1;  // no es directorio

    int dir_size = inode_getsize(&dir_in);  // tamaño dir
    int blk = 0, bytes = 0;

    char pad[14];  // nombre ajustado
    memset(pad, 0, sizeof(pad));
    strncpy(pad, name, sizeof(pad));

    while (bytes < dir_size) {
        char buf[DISKIMG_SECTOR_SIZE];
        int n = file_getblock(fs, dirinumber, blk, buf);
        if (n <= 0) return -1;  // error bloque

        int cnt = n / sizeof(struct direntv6);
        struct direntv6 *ents = (struct direntv6 *)buf;
        for (int i = 0; i < cnt; i++) {
            if (ents[i].d_inumber == 0) continue;  // vacío
            if (memcmp(ents[i].d_name, pad, sizeof(ents[i].d_name)) == 0) {
                *dirEnt = ents[i];
                return 0;  // encontrado
            }
        }
        bytes += n;
        blk++;
    }
    return -1;  // no encontrado
}