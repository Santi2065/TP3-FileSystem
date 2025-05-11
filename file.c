#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

// Carga en buf el bloque blockNum del inodo inumber
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    // Obtener inodo
    struct inode in;
    if (inode_iget(fs, inumber, &in) != 0) return -1;

    // Tamaño de archivo y último bloque válido
    int file_size = inode_getsize(&in);
    int blk_sz = DISKIMG_SECTOR_SIZE;
    int last = (file_size - 1) / blk_sz;

    if (blockNum < 0 || (file_size > 0 && blockNum > last)) return -1;
    if (file_size == 0) return 0; // archivo vacío

    // Número de bloque físico
    int phys = inode_indexlookup(fs, &in, blockNum);
    if (phys < 0) return -1;

    // Bloque disperso: rellena con ceros
    if (phys == 0) {
        memset(buf, 0, blk_sz);
        if (blockNum == last) {
            int rem = file_size % blk_sz;
            if (rem > 0) return rem;
        }
        return blk_sz;
    }

    // Leer sector
    if (diskimg_readsector(fs->dfd, phys, buf) != blk_sz) return -1;

    // Último bloque parcial
    if (blockNum == last) {
        int rem = file_size % blk_sz;
        if (rem > 0) return rem;
    }
    return blk_sz;
}