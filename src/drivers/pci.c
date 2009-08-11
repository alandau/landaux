#include <stddef.h>
#include <arch.h>
#include <kernel.h>

/*
 * port 0xcf8 = index
 * port 0xcfc = data
 * bit 31 = 1
 * bits 30:24 = 0 
 * bits 23:16 = bus
 * bits 15:11 = device
 * bits 10:8 = function
 * bits 7:0 = register
 */
#define PCI_INDEX	0xcf8
#define PCI_DATA	0xcfc

#define pci_read_config_X(name, type) \
static inline type pci_read_config_##name(u8 bus, u8 device, u8 func, u8 reg) \
{ \
	u32 index = (1<<31) | (bus<<16) | (device<<11) | (func<<8) | (reg & 0xfc); \
	outl(PCI_INDEX, index); \
	u32 value = inl(PCI_DATA); \
	return (type)(value >> ((reg & 3) << 3)); \
}

pci_read_config_X(byte, u8)
pci_read_config_X(word, u16)
pci_read_config_X(long, u32)

int init_pci(void)
{
	int bus, dev, func;
	printk("init_pci:\n");
	for (bus = 0; bus < 256; bus++) {
		for (dev = 0; dev < 32; dev++) {
			for (func = 0; func < 8; func++) {
				u16 vendorid = pci_read_config_word(bus, dev, func, 0);
				u16 deviceid = pci_read_config_word(bus, dev, func, 2);
				if (vendorid == 0xffff || deviceid == 0xffff)
					continue;
				u16 cls = pci_read_config_word(bus, dev, func, 10);
				u32 sub = pci_read_config_long(bus, dev, func, 0x2c);
				printk("%d:%d.%d\t%04x:%04x [%04x] sub=%04x:%04x\n", bus, dev, func, vendorid, deviceid, cls, sub&0xffff, sub>>16);
			}
		}
	}
	return 0;
}
