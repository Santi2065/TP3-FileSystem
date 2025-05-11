#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include "inode.h"
#include "diskimg.h"

// Carga el inodo 'inumber' en 'inp'
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (inumber < 1) return -1;

    int ipb = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int blk = INODE_START_SECTOR + (inumber - 1) / ipb;
    int off = (inumber - 1) % ipb;

    char buf[DISKIMG_SECTOR_SIZE];
    if (diskimg_readsector(fs->dfd, blk, buf) != DISKIMG_SECTOR_SIZE)
        return -1;

    struct inode *inodes = (struct inode *)buf;
    *inp = inodes[off];
    return 0;
}

// Devuelve el bloque físico de índice 'blockNum'
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if (blockNum < 0) return -1;
    int large = (inp->i_mode & ILARG) != 0;

    if (!large) {
        if (blockNum < 8) return inp->i_addr[blockNum];
        blockNum -= 8;
        // indirecto simple
        if (blockNum < 256) {
            if (!inp->i_addr[8]) return 0;
            uint16_t tbl[256];
            if (diskimg_readsector(fs->dfd, inp->i_addr[8], (char *)tbl) != DISKIMG_SECTOR_SIZE)
                return -1;
            return tbl[blockNum];
        }
        blockNum -= 256;
        // indirecto doble
        if (blockNum < 256 * 256) {
            if (!inp->i_addr[9]) return 0;
            uint16_t outer[256];
            if (diskimg_readsector(fs->dfd, inp->i_addr[9], (char *)outer) != DISKIMG_SECTOR_SIZE)
                return -1;
            int oi = blockNum / 256;
            int ii = blockNum % 256;
            if (!outer[oi]) return 0;
            uint16_t inner[256];
            if (diskimg_readsector(fs->dfd, outer[oi], (char *)inner) != DISKIMG_SECTOR_SIZE)
                return -1;
            return inner[ii];
        }
        return -1;
    } else {
        // archivos grandes: addr[0..6] indirectos simples
        if (blockNum < 7 * 256) {
            int ti = blockNum / 256;
            int ei = blockNum % 256;
            if (!inp->i_addr[ti]) return 0;
            uint16_t tbl[256];
            if (diskimg_readsector(fs->dfd, inp->i_addr[ti], (char *)tbl) != DISKIMG_SECTOR_SIZE)
                return -1;
            return tbl[ei];
        }
        blockNum -= 7 * 256;
        // doble indirecto en addr[7]
        if (blockNum < 256 * 256) {
            if (!inp->i_addr[7]) return 0;
            uint16_t outer[256];
            if (diskimg_readsector(fs->dfd, inp->i_addr[7], (char *)outer) != DISKIMG_SECTOR_SIZE)
                return -1;
            int oi = blockNum / 256;
            int ii = blockNum % 256;
            if (!outer[oi]) return 0;
            uint16_t inner[256];
            if (diskimg_readsector(fs->dfd, outer[oi], (char *)inner) != DISKIMG_SECTOR_SIZE)
                return -1;
            return inner[ii];
        }
        return -1;
    }
}

int inode_getsize(struct inode *inp) {
    return (inp->i_size0 << 16) | inp->i_size1;
}