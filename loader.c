#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <elf.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

void loader_cleanup() {
    if (ehdr != NULL) {
        free(ehdr);
    }
    if (phdr != NULL) {
        free(phdr);
    }
    if (fd != -1) {
        close(fd);
    }
}

void load_and_run_elf(char *exe) {
    fd = open(exe, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    // Read ELF header
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    read(fd, ehdr, sizeof(Elf32_Ehdr));

    // Find the PT_LOAD segment with entrypoint
    phdr = (Elf32_Phdr *)malloc(ehdr->e_phentsize);
    lseek(fd, ehdr->e_phoff, SEEK_SET);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        read(fd, phdr, ehdr->e_phentsize);
        if (phdr->p_type == PT_LOAD &&
            ehdr->e_entry >= phdr->p_vaddr &&
            ehdr->e_entry < phdr->p_vaddr + phdr->p_memsz) {
            break;
        }
    }

    // Allocate memory using mmap
    void *virtual_memory = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Copy segment content
    lseek(fd, phdr->p_offset, SEEK_SET);
    read(fd, virtual_memory, phdr->p_filesz);

    // Calculate entrypoint address within the loaded segment
    void *entrypoint = virtual_memory + (ehdr->e_entry - phdr->p_vaddr);

    // Define function pointer type for _start and use it to typecast function pointer properly
    typedef int (*StartFunc)();
    StartFunc _start = (StartFunc)entrypoint;

    // Call the "_start" method and print the value returned from "_start"
    int result = _start();
    printf("User _start return value = %d\n", result);

    // Cleanup whatever opened
    munmap(virtual_memory, phdr->p_memsz);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    load_and_run_elf(argv[1]);
    loader_cleanup();
    return 0;
}
