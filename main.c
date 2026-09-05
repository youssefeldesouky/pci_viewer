#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "core.h"

struct mcfg_entry *mcfg_base;

int main(int argc, char *argv[]) {
    PCI_DEV_t device;
    int nents;
    int ret;
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

    ret = _sys_read_mcfg_table(NULL, &mcfg_base, &nents);
    if (ret < 0) {
        fprintf(stderr, "Failed to read MCFG table: %d\n", ret);
        return 1;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <PCI_DEVICE>\n", argv[0]);
        return 1;
    }

    device = pci_str_to_dev(argv[1]);
    if (device == PCI_INVALID) {
        fprintf(stderr, "Invalid PCI device: %s\n", argv[1]);
        return 1;
    }

    pci_cfg_hdr_t *cfg_hdr = pci_dev_to_cfg_hdr(device, mem_fd);
    if (cfg_hdr) {
        // Do something with the configuration header
        printf("command: %#x status: %#x header_type: %#x\n", cfg_hdr->type_x.command, cfg_hdr->type_x.status, cfg_hdr->type_x.header_type);
    }

    pci_dev_unmap_mmio(cfg_hdr);
    free(mcfg_base);
    close(mem_fd);
    return 0;
}