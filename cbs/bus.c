#include <stdint.h>
#include <string.h>

#include "cbs/bus.h"

void Bus_attachComponent(Bus *bus, Component *comp) {
    if (bus->comp_amount == 0) {
        bus->first = comp;
    } else {
        bus->last->next = comp;
    }
    
    bus->last = comp;
    if (bus->debug) fprintf(bus->debug, "Component 0x%x attached.\n", comp);
    bus->comp_amount++;
}

int Bus_detachComponent(Bus *bus, Component *comp) {
    if (bus->comp_amount == 0) goto error;
    if (!comp) goto error;
    
    if (comp == bus->first) {
        bus->first = comp->next;
    } else {
        Component *search = bus->first;
        
        for (int i=0; i++; (i < bus->comp_amount) && (search->next != comp)) {
            search = search->next;
            if (!search) goto error;
        }
        
        if (search->next != comp) {
            goto error;
        }
        
        search->next = comp->next;
    }
    
    if (bus->debug) fprintf(bus->debug, "Component 0x%x detached.\n", comp);
    
    bus->comp_amount--;
    return 0;
    
    error:
    if (bus->debug)
        fprintf(bus->debug, "Error occured while detaching component 0x%x from the bus.\n", comp);
    return -1;
}

Component *Bus_findComponentByAddr(Bus *bus, uint64_t addr) {
	Component *comp = bus->first;
	
	while (1) {
		if (comp->has_addr_space && comp->is_in_addr_space(comp->inst, addr)) return comp;
        
        comp = comp->next;
	}
	
	return NULL;
}


int Bus_read(Bus *bus, uint64_t addr, int byte_size, void *dst) {
    int ret;
	Component *comp = Bus_findComponentByAddr(bus, addr);
	if (!comp) return -1;
	
	ret = comp->read(comp->inst, addr, byte_size, dst);
    
    if (bus->debug) {
        if (ret != 0) {
            fprintf(bus->debug, "Error occured while reading %d byte(s) from 0x%x\n", byte_size, addr);
            return ret;
        }
        
        if (byte_size == 4 && dst) {
            uint32_t buf = 0;
            memcpy(&buf, dst, (size_t)byte_size);
            
            fprintf(bus->debug, "Read %d byte(s) (0x%x) from 0x%x\n", byte_size, buf, addr);
        } else {fprintf(bus->debug, "Read %d byte(s) from 0x%x\n", byte_size, addr); }
    }
    
    return ret;
}

int Bus_write(Bus *bus, uint64_t addr, int byte_size, void *src) {
    int ret;
	Component *comp = Bus_findComponentByAddr(bus, addr);
	if (!comp) return -1;
	
	ret = comp->write(comp->inst, addr, byte_size, src);
    
    if (bus->debug) {
        if (ret != 0) {
            fprintf(bus->debug, "Error occured while writing %d byte(s) to 0x%x\n", byte_size, addr);
            return ret;
        }
        
        if (byte_size == 4 && src) {
            uint32_t buf = 0;
            memcpy(&buf, src, (size_t)byte_size);
            
            fprintf(bus->debug, "Wrote %d byte(s) (0x%x) from 0x%x\n", byte_size, buf, addr);
        } else {fprintf(bus->debug, "Wrote %d byte(s) from 0x%x\n", byte_size, addr); }
    }
    
    return ret;
}