#include <stdint.h>

#define MCFG_ENTRY_SIZE 16
#define MCFG_ENTRY_OFFSET 0x2C

struct mcfg_entry {
    uint64_t base_address;
    uint16_t pci_segment_group_number;
    uint8_t start_bus_number;
    uint8_t end_bus_number;
    uint32_t reserved;
};