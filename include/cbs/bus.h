#ifndef _BUS_H_
#define _BUS_H_

#include <stdint.h>
#include <stdio.h>

typedef struct Component {
    int has_addr_space;     // Indicates whether the component has any address space.
    
    // CALLBACK FUNCTIONS: (To be implemented by the component)
    int (*is_in_addr_space)(void *inst, uint64_t addr);
        // Returns 1 when the given address is within the component's address space.
    
    int (*read)(void *inst, uint64_t addr, int byte_size, void *dst);
        // Copies byte_size bytes of data from the component's address space to dst.
    int (*write)(void *inst, uint64_t addr, int byte_size, void *src);
        // Copies copy byte_size bytes of data from src to the component's address space.
    
    void **intf;   // Interface. Here, the implementer can implement additional func-
        // tion(alities) of the component aside from the standard read and write.
        // Though to use them properly, the user must have an agreement with the impl-
        // ementer what these are (preferably a header file), since this library does
        // not provide a standard for that. Can be NULL.
    
    void *inst;     // Component instance, which is passed to the callback functions.
                    // Can be NULL.
    
    struct Component *next;     // Next list element. MUST NOT BE SET BY USER/IMPLEMENTER.
} Component;

typedef struct Bus {
	Component *first;   // Pointer to the first component
    Component *last;    // Pointer to the last component
    int comp_amount;    // Total amount of components
    
    FILE *debug;    // Debug handle. If not NULL, log messages will be written to this handle.
} Bus;


void Bus_attachComponent(Bus *bus, Component *comp);        // Attaches a component to the bus.
int Bus_detachComponent(Bus *bus, Component *comp);         // Detaches a component from the bus.
Component *Bus_findComponentByAddr(Bus *bus, uint64_t addr);    // Finds a component index by
    // the component's address space. Overlapping address spaces cause the component which
    // was registered first for this address space to be returned. (Might change in the future)

int Bus_read(Bus *bus, uint64_t addr, int byte_size, void *dst);    // Reads from the bus.
int Bus_write(Bus *bus, uint64_t addr, int byte_size, void *src);   // Writes to the bus.

#endif
