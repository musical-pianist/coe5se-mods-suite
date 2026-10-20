// The mach_vm calls our Mac dylibs make, declared as in Apple's SDK. zig cc's own macOS
// headers have the types (mach/mach_types.h) and its libSystem stub has the functions, but not this
// header. A build with Apple's clang finds the SDK's own header first.
#ifndef COE5SE_MAC_SDK_MACH_VM_H
#define COE5SE_MAC_SDK_MACH_VM_H

#include <mach/mach_types.h>

kern_return_t mach_vm_allocate(vm_map_t target, mach_vm_address_t *address, mach_vm_size_t size, int flags);
kern_return_t mach_vm_deallocate(vm_map_t target, mach_vm_address_t address, mach_vm_size_t size);
kern_return_t mach_vm_protect(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size,
                              boolean_t set_maximum, vm_prot_t new_protection);
kern_return_t mach_vm_region(vm_map_read_t target_task, mach_vm_address_t *address, mach_vm_size_t *size,
                             vm_region_flavor_t flavor, vm_region_info_t info, mach_msg_type_number_t *infoCnt,
                             mach_port_t *object_name);

#endif
