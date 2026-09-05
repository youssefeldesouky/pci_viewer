#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ncurses.h>

#include "core.h"

bool domain_index_mismatch = false;

int _sys_read_mcfg_table(const char *path, struct mcfg_entry **mcfg_entries, int *n) {
    char *p = (char *)path;
    uint32_t len;
    int ret = 0;
    int nents;
    int fd;

    if (!p)
        p = "/sys/firmware/acpi/tables/MCFG";

    fd = open(p, O_RDONLY);
    if (fd < 0)
        return -1;
    
    ret = pread(fd, &len, sizeof(len), 4);
    if (ret < 0) {
        close(fd);
        return -2;
    }
    nents = (len - MCFG_ENTRY_OFFSET) / MCFG_ENTRY_SIZE;
    *mcfg_entries = calloc(nents + 1, sizeof(**mcfg_entries));
    if (!*mcfg_entries) {
        close(fd);
        return -3;
    }

    for (int i = 0; i < nents; i++) {
        ret = pread(fd, &(*mcfg_entries)[i], sizeof(**mcfg_entries), MCFG_ENTRY_OFFSET + i * MCFG_ENTRY_SIZE);
        if (ret < 0) {
            free(*mcfg_entries);
            close(fd);
            return -4;
        }
    }
    *n = nents;
    close(fd);
    return 0;
}

uint64_t pci_dev_to_address(PCI_DEV_t dev) {
    uint16_t dom = PCI_DEV_GET_DOM(dev);
    
    if (dom != mcfg_base[dom].pci_segment_group_number) {
        domain_index_mismatch = true;
    }

    if (domain_index_mismatch) {
        struct mcfg_entry *current = mcfg_base;
        for (; current->base_address; current++) {
            if (current->pci_segment_group_number == dom)
		return current->base_address + PCI_DEV_TO_OFFSET(dev);
        }
        return PCI_INVALID;
    }
    return mcfg_base[dom].base_address + PCI_DEV_TO_OFFSET(dev);
}

void *pci_dev_to_mmio(PCI_DEV_t dev, int mem_fd) {
    uint64_t addr = pci_dev_to_address(dev);
    void *map = NULL;

    if (addr == PCI_INVALID || mem_fd < 0)
        return NULL;

    map = mmap(NULL, PCIE_CFG_SPACE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr);
    if (map == MAP_FAILED)
        return NULL;

    if(*(uint32_t *)map == PCI_INVALID) {
        fprintf(stdout, "Invalid PCI device at address %s\n", pci_dev_to_str(dev));
        munmap(map, PCIE_CFG_SPACE_SIZE);
        return NULL;
    }
    return map;
}

pci_cfg_hdr_t *pci_dev_to_cfg_hdr(PCI_DEV_t dev, int mem_fd) {
    void *mmio = pci_dev_to_mmio(dev, mem_fd);
    if (!mmio)
        return NULL;
    return (pci_cfg_hdr_t *)mmio;
}

void pci_dev_unmap_mmio(void *map) {
    if (map)
        munmap(map, PCIE_CFG_SPACE_SIZE);
}

char *pci_dev_to_str(PCI_DEV_t dev) {
    char *buf;

    buf = malloc(16);
    if (!buf)
        return NULL;
    snprintf(buf, 16, "%04x:%02x:%02x.%x", PCI_DEV_GET_DOM(dev),
             PCI_DEV_GET_BUS(dev), PCI_DEV_GET_SLOT(dev), PCI_DEV_GET_FN(dev));
    return buf;
}

PCI_DEV_t pci_str_to_dev(const char *str) {
    uint16_t dom = 0, bus = 0, slot = 0, fn = 0;
    int _1 = -1, _2 = -1, _3 = -1, _4 = -1;
    char *dot = strchr((char *)str, '.');
    int ret;

    ret = sscanf(str, "%x:%x:%x", &_1, &_2, &_3);
    if (ret < 2)
        return PCI_INVALID;
    if (ret == 2)
        bus = _1, slot = _2;
    else if (ret == 3)
        dom = _1, bus = _2, slot = _3;
    if (dot) {
        ret = sscanf(dot + 1, "%x", &_4);
        if (_4 != -1)
            fn = _4;
    }
    return TO_PCI_DEV(dom, bus, slot, fn);
}
