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

const char *cls_to_str(u16 cls) {
	static char res[100];
	static const struct subclass{
		u8 sub;
		const char *str;
	} subcls_mass_storage[] = {
		{0x01, "IDE"},
		{0x06, "SATA"},
		{0, 0}
	}, subcls_network[] = {
		{0x00, "Ethernet"},
		{0, 0}
	}, subcls_display[] = {
		{0x00, "VGA"},
		{0, 0}
	}, subcls_bridge[] = {
		{0x00, "Host"},
		{0x01, "ISA"},
		{0x80, "Other"},
		{0, 0}
	}, subcls_serial_bus[] = {
		{0x03, "USB"},
		{0x05, "SMBus"},
		{0, 0}
	};
	static const struct {
		u8 cls;
		const char *str;
		const struct subclass* sub;
	} classes[] = {
		{0x01, "Mass storage", subcls_mass_storage},
		{0x02, "Network", subcls_network},
		{0x03, "Display", subcls_display},
		{0x06, "Bridge", subcls_bridge},
		{0x0c, "Serial bus", subcls_serial_bus},
	};
	strcpy(res, "Unknown");
	for (int i = 0; i < sizeof(classes)/sizeof(classes[0]); i++) {
		if (classes[i].cls != (cls >> 8)) {
			continue;
		}
		strcpy(res, classes[i].str);
		strcat(res, ": ");
		;
		for (const struct subclass *sub = classes[i].sub; sub->str; sub++) {
			if (sub->sub == (cls & 0xff)) {
				strcat(res, sub->str);
				return res;
			}
		}
		strcat(res, "Unknown");
		return res;
	}
	return res;
}

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
				printk("%d:%d.%d\t%04x:%04x [%04x] sub=%04x:%04x\t%s\n", bus, dev, func, vendorid, deviceid, cls, sub&0xffff, sub>>16, cls_to_str(cls));
			}
		}
	}
	return 0;
}
