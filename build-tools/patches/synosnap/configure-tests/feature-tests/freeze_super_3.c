// SPDX-License-Identifier: GPL-2.0-only

// kernel_version >= 6.17: freeze_super gains a third 'owner' argument

#include "includes.h"
MODULE_LICENSE("GPL");

static inline void dummy(void){
    struct super_block *sb;
    freeze_super(sb, FREEZE_HOLDER_KERNEL, NULL);
}
