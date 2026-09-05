#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "mcfg.h"

#define PCI_INVALID 0xFFFFFFFFu
#define PCIE_CFG_SPACE_SIZE 0x1000ull

extern struct mcfg_entry *mcfg_base;
/*
 * The tool trusts that the index for an entry in mcfg_base matches its domain
 * number. If this is not the case, then the tool will walk the array every
 * time it tries to get a domain.
*/
extern bool domain_index_mismatch;

typedef uint32_t PCI_DEV_t;

#define TO_PCI_DEV(DOM,BUS,DEV,FN) ((DOM) << 16 | (BUS) << 8 | (DEV) << 3 | (FN))
#define PCI_DEV_GET_DOM(dev) (((dev) >> 16) & 0xFF)
#define PCI_DEV_GET_BUS(dev) (((dev) >> 8) & 0xFF)
#define PCI_DEV_GET_SLOT(dev) (((dev) >> 3) & 0x1F)
#define PCI_DEV_GET_FN(dev) (((dev)) & 0x07)
#define PCI_DEV_TO_OFFSET(dev) ((PCI_DEV_GET_BUS(dev) * 32 * 8 + \
                                PCI_DEV_GET_SLOT(dev) * 8 + \
                                PCI_DEV_GET_FN(dev)) * PCIE_CFG_SPACE_SIZE)

struct pci_cfg_command_reg {
    uint16_t io_space : 1;
    uint16_t mem_space : 1;
    uint16_t bus_master : 1;
    uint16_t special_cycles : 1;
    uint16_t mem_write_inv : 1;
    uint16_t vga_palette_snoop : 1;
    uint16_t parity_err_resp : 1;
    uint16_t reserved_0 : 1;
    uint16_t serr_enable : 1;
    uint16_t fast_back_to_back_enable : 1;
    uint16_t int_disable : 1;
    uint16_t reserved_1 : 4;
};

struct pci_cfg_status_reg {
    uint16_t reserved_0 : 3;
    uint16_t int_status : 1;
    uint16_t capabilities_list : 1;
    uint16_t sixy_six_mhz_capable : 1;
    uint16_t reserved_1 : 1;
    uint16_t fast_b2b_capable : 1;
    uint16_t master_data_parity_err : 1;
    uint16_t devsel_timing : 2;
    uint16_t signaled_target_abort : 1;
    uint16_t received_target_abort : 1;
    uint16_t received_master_abort : 1;
    uint16_t signaled_system_error : 1;
    uint16_t detected_parity_error : 1;
};

struct pci_cfg_hdr_type_x {
    uint16_t vendor_id;
    uint16_t device_id;
    union {
        uint16_t command;
        struct pci_cfg_command_reg command_fields;
    };
    union {
        uint16_t status;
        struct pci_cfg_status_reg status_fields;
    };
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    struct {
        uint8_t header_type :1;
        uint8_t reserved_0 :6;
        uint8_t multi_function :1;
    };
    uint8_t bist;
};

struct pci_cfg_bar_reg {
    uint32_t space : 1;
    uint32_t type : 2;
    uint32_t prefetchable : 1;
    uint32_t reserved_0 : 28;
};

struct pci_cfg_bridge_control_reg {
    uint16_t parity_error_response : 1;
    uint16_t serr_enable : 1;
    uint16_t isa_enable : 1;
    uint16_t vga_enable : 1;
    uint16_t vga_16bit_enable : 1;
    uint16_t master_abort_mode : 1;
    uint16_t secondary_bus_reset : 1;
    uint16_t fast_back_to_back_enable : 1;
};

struct pci_cfg_hdr_type_0 {
    struct pci_cfg_hdr_type_x common;
    union {
        uint32_t bar;
        struct pci_cfg_bar_reg bar_fields;
    } bars[6];
    uint32_t cardbus_cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_addr;
    uint8_t capabilities_ptr;
    uint8_t reserved_0[7];
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
};

struct pci_cfg_hdr_type_1 {
    struct pci_cfg_hdr_type_x common;
    union {
        uint32_t bar;
        struct pci_cfg_bar_reg bar_fields;
    } bars[2];
    uint8_t primary_bus_number;
    uint8_t secondary_bus_number;
    uint8_t subordinate_bus_number;
    uint8_t secondary_latency_timer;
    uint8_t io_base;
    uint8_t io_limit;
    uint16_t secondary_status;
    uint16_t memory_base;
    uint16_t memory_limit;
    uint16_t prefetchable_memory_base;
    uint16_t prefetchable_memory_limit;
    uint32_t prefetchable_base_upper32;
    uint32_t prefetchable_limit_upper32;
    uint16_t io_base_upper16;
    uint16_t io_limit_upper16;
    uint8_t capabilities_ptr;
    uint8_t reserved_0[3];
    uint32_t expansion_rom_base_addr;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    union {
        uint16_t bridge_control;
        struct pci_cfg_bridge_control_reg bridge_control_fields;
    };
};

typedef union {
    struct pci_cfg_hdr_type_x type_x;
    struct pci_cfg_hdr_type_0 type_0;
    struct pci_cfg_hdr_type_1 type_1;
} pci_cfg_hdr_t;


int _sys_read_mcfg_table(const char *path, struct mcfg_entry **mcfg_entries, int *n);
uint64_t pci_dev_to_address(PCI_DEV_t dev);
void *pci_dev_to_mmio(PCI_DEV_t dev, int mem_fd);
void pci_dev_unmap_mmio(void *map);
char *pci_dev_to_str(PCI_DEV_t dev);
PCI_DEV_t pci_str_to_dev(const char *str);
pci_cfg_hdr_t *pci_dev_to_cfg_hdr(PCI_DEV_t dev, int mem_fd);
