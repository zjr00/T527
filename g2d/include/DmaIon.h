#ifndef _DMAION_H_
#define _DMAION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <asm-generic/ioctl.h>
#include "ion_alloc_list.h"
#include <linux/types.h>
//#define CONFIG_IOMMU
#define USE_ION_DMA (1)
#define USE_ION_CUSTOM (1)

typedef int aw_dma_user_handle_t;

typedef struct aw_ion_dma_allocation_info {
    size_t aw_len;
    size_t aw_align;
    unsigned int aw_heap_id_mask;
    unsigned int flags;
    aw_dma_user_handle_t handle;
} aw_ion_dma_allocation_info_t;

typedef struct dma_handle_data {
    aw_dma_user_handle_t handle;
} dma_handle_data_t ;

typedef struct aw_dma_fd_data {
    aw_dma_user_handle_t handle;
    int aw_fd;
} dma_fd_data_t;

typedef struct aw_dma_custom_info {
    unsigned int aw_cmd;
    unsigned long aw_arg;
} dma_custom_data_t;

typedef struct {
    aw_dma_user_handle_t handle;
    unsigned long  phys_addr;
    unsigned int  size;
} dma_sunxi_phys_data;

typedef struct {
    long    start;
    long    end;
} dma_sunxi_cache_range;

#define SZ_64M      0x04000000
#define SZ_4M       0x00400000
#define SZ_1M       0x00100000
#define SZ_64K      0x00010000
#define SZ_4k       0x00001000
#define SZ_1k       0x00000400

enum dma_heap_type {
    AW_DMA_SYSTEM_HEAP_TYPE,
    AW_DMA_SYSTEM_CONTIG_HEAP_TYPE,
    AW_DMA_CARVEOUT_HEAP_TYPE,
    AW_DMA_TYPE_HEAP_CHUNK,
    AW_DMA_TYPE_HEAP_DMA,
#if(USE_ION_CUSTOM)
    AW_DMA_TYPE_HEAP_CUSTOM,
#endif
    AW_DMA_TYPE_HEAP_SECURE,
    AW_DMA_NUM_HEAPS = 16,/* must be last so device specific heaps always
                 are at the end of this enum */
};


enum ion_heap_type_new {
    ION_SYSTEM_HEAP_TYPE,
    ION_TYPE_HEAP_DMA,
};
#define ION_SYSTEM_HEAP_MASK (1 << ION_SYSTEM_HEAP_TYPE) //iommu use
#define ION_DMA_HEAP_MASK (1 << ION_TYPE_HEAP_DMA) //cma use


struct aw_ion_new_alloc_data {
    __u64 len;
    __u32 heap_id_mask;
    __u32 flags;
    __u32 fd;
    __u32 unused;
};


#define KERNEL_5_4


#define AW_MEM_ION_IOC_MAGIC        'I'

#define AW_ION_IOC_NEW_ALLOC _IOWR(AW_MEM_ION_IOC_MAGIC, 0, struct aw_ion_new_alloc_data)

#define AW_MEM_ION_IOC_ALLOC        _IOWR(AW_MEM_ION_IOC_MAGIC, 0, struct aw_ion_dma_allocation_info)
#define AW_MEM_ION_IOC_FREE     _IOWR(AW_MEM_ION_IOC_MAGIC, 1, struct dma_handle_data)
#define AW_MEM_ION_IOC_MAP      _IOWR(AW_MEM_ION_IOC_MAGIC, 2, struct aw_dma_fd_data)
#define AW_MEMION_IOC_SHARE     _IOWR(AW_MEM_ION_IOC_MAGIC, 4, struct aw_dma_fd_data)
#define AW_MEMION_IOC_IMPORT        _IOWR(AW_MEM_ION_IOC_MAGIC, 5, struct aw_dma_fd_data)
#define AW_MEMION_IOC_SYNC      _IOWR(AW_MEM_ION_IOC_MAGIC, 7, struct aw_dma_fd_data)
#define AW_MEM_ION_IOC_CUSTOM       _IOWR(AW_MEM_ION_IOC_MAGIC, 6, struct aw_dma_custom_info)

#define AW_ION_CACHED_FLAG 1        /* mappings of this buffer should be cached, ion will do cache maintenance when the buffer is mapped for dma */
#define AW_ION_CACHED_NEEDS_SYNC_FLAG 2 /* mappings of this buffer will created at mmap time, if this is set caches must be managed manually */

#define ION_IOC_SUNXI_FLUSH_RANGE           5
#define ION_IOC_SUNXI_FLUSH_ALL             6
#define ION_IOC_SUNXI_PHYS_ADDR             7
#define ION_IOC_SUNXI_DMA_COPY              8

#define AW_ION_SYSTEM_HEAP_MASK     (1 << AW_DMA_SYSTEM_HEAP_TYPE)
#define AW_ION_SYSTEM_CONTIG_HEAP_MASK  (1 << AW_DMA_SYSTEM_CONTIG_HEAP_TYPE)
#define AW_ION_CARVEOUT_HEAP_MASK   (1 << AW_DMA_CARVEOUT_HEAP_TYPE)
#define AW_ION_DMA_HEAP_MASK        (1 << AW_DMA_TYPE_HEAP_DMA)
#define AW_ION_SECURE_HEAP_MASK     (1 << AW_DMA_TYPE_HEAP_SECURE)

struct sunxi_pool_info {
    unsigned int total; /*unit kb*/
    unsigned int free_kb;   /*size kb*/
    unsigned int free_mb;   /*size mb*/
};

typedef struct ION_BUFFER_NODE {
    struct aw_mem_list_head i_list;
    unsigned long phy;      //phisical address
    uint8_t* vir;      //virtual address
    int size;       //buffer size
    unsigned long user_virt; //
    dma_fd_data_t fd_data;
} ion_buffer_node;

typedef struct ION_ALLOC_CONTEXT {
    int fd;         /* driver handle */
    struct aw_mem_list_head list; /* buffer list */
    int ref_cnt;    /* reference count */
} ion_alloc_context;


struct dma_buf_sync {
    __u64 flags;
};

#define DMA_BUF_BASE 'b'
#define DMA_BUF_IOCTL_SYNC _IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)

#define DMA_BUF_SYNC_READ (1 << 0)
#define DMA_BUF_SYNC_WRITE (2 << 0)
#define DMA_BUF_SYNC_RW (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START (0 << 2)
#define DMA_BUF_SYNC_END (1 << 2)
#define DMA_BUF_SYNC_VALID_FLAGS_MASK (DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END)

int IonAllocOpen();
int IonAllocClose();
uint8_t* IonAlloc(int size);
int IonFree(void *pbuf);
int IonVir2fd(void *pbuf);
unsigned long IonVir2ShareFd(void *pbuf);
unsigned long IonVir2phy(void *pbuf);
int mem_addr_vir2phy(unsigned long vir, unsigned long *phy);
uint8_t* IonPhy2vir(void *pbuf);
void IonFlushCache(void *startAddr, int size);
void IonFlushCacheAll();
uint8_t* IonAllocDrm(int size);
int GetIonTotalMem();
int IonDmaSync(int dmafd);


#ifdef __cplusplus
}
#endif

#endif


