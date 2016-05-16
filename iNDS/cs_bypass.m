
#import <Foundation/Foundation.h>
#import <sys/syscall.h>
#import <dlfcn.h>
#include <mach-o/nlist.h>
#include <mach-o/dyld.h>
#include <sys/mman.h>
#include <mach/mach.h>

#ifdef __LP64__
#define mach_hdr struct mach_header_64
#define sgmt_cmd struct segment_command_64
#define sect_cmd struct section_64
#define nlist_ struct nlist_64
#define LC_SGMT LC_SEGMENT_64
#define MH_MAGIC_ MH_MAGIC_64
#else
#define mach_hdr struct mach_header
#define sgmt_cmd struct segment_command
#define sect_cmd struct section
#define nlist_ struct nlist
#define LC_SGMT LC_SEGMENT
#define MH_MAGIC_ MH_MAGIC
#endif
#define load_cmd struct load_command

#define SYSTEM_VERSION_GREATER_THAN(v)              ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedDescending)

sect_cmd *find_section(sgmt_cmd *seg, const char *name)
{
    sect_cmd *sect, *fs = NULL;
    uint32_t i = 0;
    for (i = 0, sect = (sect_cmd *)((uint64_t)seg + (uint64_t)sizeof(sgmt_cmd));
         i < seg->nsects;
         i++, sect = (sect_cmd*)((uint64_t)sect + sizeof(sect_cmd)))
    {
        if (!strcmp(sect->sectname, name)) {
            fs = sect;
            break;
        }
    }
    return fs;
}

load_cmd *find_load_command(mach_hdr *mh, uint32_t cmd)
{
    load_cmd *lc, *flc;
    lc = (load_cmd *)((uint64_t)mh + sizeof(mach_hdr));
    while ((uint64_t)lc < (uint64_t)mh + (uint64_t)mh->sizeofcmds) {
        if (lc->cmd == cmd) {
            flc = (load_cmd *)lc;
            break;
        }
        lc = (load_cmd *)((uint64_t)lc + (uint64_t)lc->cmdsize);
    }
    return flc;
}

sgmt_cmd *find_segment(mach_hdr *mh, const char *segname)
{
    load_cmd *lc;
    sgmt_cmd *s, *fs = NULL;
    lc = (load_cmd *)((uint64_t)mh + sizeof(mach_hdr));
    while ((uint64_t)lc < (uint64_t)mh + (uint64_t)mh->sizeofcmds) {
        if (lc->cmd == LC_SGMT) {
            s = (sgmt_cmd *)lc;
            if (!strcmp(s->segname, segname)) {
                fs = s;
                break;
            }
        }
        lc = (load_cmd *)((uint64_t)lc + (uint64_t)lc->cmdsize);
    }
    return fs;
}

void* find_sym(mach_hdr *mh, const char *name) {
    sgmt_cmd* first = (sgmt_cmd*) find_load_command(mh, LC_SGMT);
    sgmt_cmd* linkedit = find_segment(mh, SEG_LINKEDIT);
    struct symtab_command* symtab = (struct symtab_command*) find_load_command(mh, LC_SYMTAB);
    vm_address_t vmaddr_slide = (vm_address_t)mh - (vm_address_t)first->vmaddr;
    
    char* sym_str_table = (char*) linkedit->vmaddr - linkedit->fileoff + vmaddr_slide + symtab->stroff;
    nlist_* sym_table = (nlist_*)(linkedit->vmaddr - linkedit->fileoff + vmaddr_slide + symtab->symoff);
    
    for (int i = 0; i < symtab->nsyms; i++) {
        if (sym_table[i].n_value && !strcmp(name,&sym_str_table[sym_table[i].n_un.n_strx])) {
            return (void*) (uint64_t) (sym_table[i].n_value + vmaddr_slide);
        }
    }
    return 0;
}

vm_address_t find_dyld() {
    kern_return_t kr      = KERN_SUCCESS;
    vm_address_t  address = 0;
    vm_size_t     size    = 0;
    
    while (1) {
        mach_msg_type_number_t count;
        struct vm_region_submap_info_64 info;
        uint32_t nesting_depth;
        
        count = VM_REGION_SUBMAP_INFO_COUNT_64;
        kr = vm_region_recurse_64(mach_task_self(), &address, &size, &nesting_depth,
                                  (vm_region_info_64_t)&info, &count);
        if (kr == KERN_INVALID_ADDRESS) {
            break;
        } else if (kr) {
            mach_error("vm_region:", kr);
            break; /* last region done */
        }
        
        if (info.is_submap) {
            nesting_depth++;
        } else {
            if (info.protection & PROT_EXEC && info.protection & PROT_READ) {
                if (*(uint32_t*) (address) == MH_MAGIC_ ) {
                    mach_hdr* hd = (mach_hdr*) address;
                    if (hd->filetype == MH_DYLINKER) {
                        return address;
                    }
                }
            }
            address += size;
        }
    }
    return 0;
}

static int fcntlhook(int a, int b) {
    return -1;
}

static void *
mmaphook(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    void* ret = mmap(addr, len, PROT_READ|PROT_WRITE, (flags & (~(MAP_FILE|MAP_SHARED))) | MAP_ANON | MAP_PRIVATE, 0, offset);
    if (((vm_address_t)ret) == -1) {
        return ret;
    }
    lseek(fd, offset, SEEK_SET);
    read(fd, ret, len);
    if (!(flags & PROT_WRITE)) {
        mlock(ret, len);
    }
    mprotect(ret, len, prot);
    return ret;
}

__attribute__((constructor))
void ayy_lmao() {
    if (SYSTEM_VERSION_GREATER_THAN(@"9.3.2")) {
        return; // Not supported
    }
    // Load PLT entries (munmap breaks dyld..)
    
    mmap(0, 0, 0, 0, 0, 0);
    mlock(0, 0);
    mprotect(0, 0, 0);
    
    mach_hdr* dyld_hdr = (mach_hdr*) find_dyld();
    assert(dyld_hdr);
    assert(dyld_hdr->filetype == MH_DYLINKER);
    // Copy original code

    vm_address_t fcntl = (vm_address_t) find_sym(dyld_hdr, "_fcntl");
    assert(fcntl);
    vm_address_t xmmap = (vm_address_t) find_sym(dyld_hdr, "_xmmap");
    assert(xmmap);
    
    char buf[PAGE_SIZE*2];
    memcpy(buf, (void*)(xmmap & (~PAGE_MASK)), PAGE_SIZE*2);
    
    // Patch.
    
    extern void _tramp_begin();
    extern void _tramp_end();
    char* xmb = &buf[xmmap & PAGE_MASK];
    memcpy(xmb, _tramp_begin, ((vm_address_t)_tramp_end)-((vm_address_t)_tramp_begin));
    
    vm_address_t* tramp_target = (vm_address_t*) &xmb[((vm_address_t)_tramp_end)-((vm_address_t)_tramp_begin)];
    tramp_target --;
    *tramp_target = (vm_address_t) mmaphook;

    // Replace code
    
    munmap((void*)(xmmap & (~PAGE_MASK)), PAGE_SIZE*2);
    mmap((void*)(xmmap & (~PAGE_MASK)), PAGE_SIZE*2, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, 0, 0);
    mlock((void*)(xmmap & (~PAGE_MASK)), PAGE_SIZE*2);
    memcpy((void*)(xmmap & (~PAGE_MASK)), buf, PAGE_SIZE*2);
    mprotect((void*)(xmmap & (~PAGE_MASK)), PAGE_SIZE*2, PROT_READ|PROT_EXEC);
    
    // Copy original code
    
    memcpy(buf, (void*)(fcntl & (~PAGE_MASK)), PAGE_SIZE*2);
    
    // Patch.
    
    xmb = &buf[fcntl & PAGE_MASK];
    memcpy(xmb, _tramp_begin, ((vm_address_t)_tramp_end)-((vm_address_t)_tramp_begin));
    
    tramp_target = (vm_address_t*) &xmb[((vm_address_t)_tramp_end)-((vm_address_t)_tramp_begin)];
    tramp_target --;
    *tramp_target = (vm_address_t) fcntlhook;
    
    // Replace code
    
    munmap((void*)(fcntl & (~PAGE_MASK)), PAGE_SIZE*2);
    mmap((void*)(fcntl & (~PAGE_MASK)), PAGE_SIZE*2, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, 0, 0);
    mlock((void*)(fcntl & (~PAGE_MASK)), PAGE_SIZE*2);
    memcpy((void*)(fcntl & (~PAGE_MASK)), buf, PAGE_SIZE*2);
    mprotect((void*)(fcntl & (~PAGE_MASK)), PAGE_SIZE*2, PROT_READ|PROT_EXEC);
}
