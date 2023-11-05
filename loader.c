#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <elf.h>
#include <signal.h>
#include <string.h>

Elf32_Ehdr *ehdr;
int fd;
int page_faults = 0;
int page_allocations = 0;
long internal_fragmentation = 0;
unsigned char *allocated_pages = NULL; 
unsigned long total_allocated_memory = 0;
unsigned long total_memory_allocated = 0;

void loader_cleanup() {
    if (allocated_pages != NULL) {
        free(allocated_pages);
    }
    if (ehdr != NULL) {
        free(ehdr);
    }
    if (fd != -1) {
        close(fd);
    }
}

void segv_handler(int signo, siginfo_t *si, void *context) {
    if (si->si_signo == SIGSEGV) {
        void *fault_addr = si->si_addr;
        unsigned int page_size = getpagesize();
        void *page_start = (void *)((uintptr_t)fault_addr & ~(page_size - 1));

        if (allocated_pages[(uintptr_t)page_start / page_size] == 0) {
            

            for (int i = 0; i < ehdr->e_phnum; i++) {
                Elf32_Phdr phdr;
                lseek(fd, ehdr->e_phoff + i * ehdr->e_phentsize, SEEK_SET);
                read(fd, &phdr, ehdr->e_phentsize);

                if (phdr.p_type == PT_LOAD &&
                    (uintptr_t)page_start >= phdr.p_vaddr &&
                    (uintptr_t)page_start < phdr.p_vaddr + phdr.p_memsz) {

        
                    size_t map_size = ((phdr.p_memsz + page_size - 1) / page_size) * page_size;
                    void *segment_memory = mmap(page_start, map_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

                    if (segment_memory == MAP_FAILED) {
                        perror("Error mapping memory for segment");
                        exit(1);
                    }

                    total_allocated_memory += map_size;

                    lseek(fd, phdr.p_offset, SEEK_SET);
                    read(fd, segment_memory, phdr.p_filesz);

                    page_faults++;
                    page_allocations++;
                    allocated_pages[(uintptr_t)page_start / page_size] = 1;
                }
            }
        }
    }
}

void load_and_run_elf(char *exe) {
    fd = open(exe, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (ehdr == NULL) {
        perror("Error allocating memory for ELF header");
        loader_cleanup();
        exit(1);
    }
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) == -1) {
        perror("Error reading ELF header");
        loader_cleanup();
        exit(1);
    }

    unsigned int max_address = 0;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf32_Phdr phdr;
        lseek(fd, ehdr->e_phoff + i * ehdr->e_phentsize, SEEK_SET);
        read(fd, &phdr, ehdr->e_phentsize);

        if (phdr.p_type == PT_LOAD) {
            if (phdr.p_vaddr + phdr.p_memsz > max_address) {
                max_address = phdr.p_vaddr + phdr.p_memsz;
            }
        }
    }

    size_t page_count = (size_t)(max_address / getpagesize()) + 1;

    allocated_pages = (unsigned char *)calloc(page_count, sizeof(unsigned char));

    if (allocated_pages == NULL) {
        perror("Error allocating memory for tracking pages");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segv_handler;
    sigaction(SIGSEGV, &sa, NULL);

    void *entrypoint = (void *)(ehdr->e_entry);

    typedef int (*StartFunc)();
    StartFunc _start = (StartFunc)entrypoint;

    int result = _start();

    internal_fragmentation = total_allocated_memory - total_memory_allocated;

    loader_cleanup();

    printf("User _start return value = %d\n", result);
    printf("Page Faults: %d\n", page_faults);
    printf("Page Allocations: %d\n", page_allocations);
    printf("Internal Fragmentation: %.2f KB\n", (float)internal_fragmentation / 1024.0);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv[1]);

    return 0;
}