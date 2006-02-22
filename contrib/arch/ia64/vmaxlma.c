/*
 * Swap VMA and LMA in ELF header.
 *
 *  by Jakub Jermar <jermar@itbs.cz>
 *
 *  GPL'ed, copyleft
 */

/*
 * HP's IA-64 simulator Ski seems to confuse VMA and LMA in the ELF header.
 * Instead of using LMA, Ski loads sections at their VMA addresses.
 * This short program provides a workaround for this bug by simply
 * swapping VMA and LMA in the ELF header of the executable.
 *
 * Note that after applying this workaround, you will be able to load
 * ELF objects with different VMA and LMA in Ski, but the executable
 * will become wronged for other potential uses.
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

void syntax(char *prg)
{
	printf("%s ELF-file\n", prg);
	exit(1);
}

void error(char *msg)
{
	printf("Error: %s\n", msg);
	exit(2);
}

#define ELF_VMA	(0x50/sizeof(unsigned long long))
#define ELF_LMA (0x58/sizeof(unsigned long long))
#define ELF_ENTRY (0x18/sizeof(unsigned long long))

#define LENGTH	0x98

int main(int argc, char *argv[])
{
	int fd;
	unsigned long long vma, lma,entry;
	unsigned long long *elf;

	if (argc != 2)
		syntax(argv[0]);
	
	fd = open(argv[1], O_RDWR);
	if (fd == -1)
		error("open failed");

	elf = mmap(NULL, LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ((void *) elf  == (void *) -1)
		error("map failed");
		
	/*vma = elf[ELF_VMA];*/
	lma = elf[ELF_LMA];
	elf[ELF_VMA] = lma;
	entry = elf[ELF_ENTRY];
	entry &= ((~0LL)>>3);
	elf[ELF_ENTRY] = entry;
	elf[ELF_ENTRY] = 0x100000;
	/*elf[ELF_LMA] = vma;*/
	
	if (munmap(elf, LENGTH) == -1)
		error("munmap failed");
	
	if (close(fd) == -1)
		error("close failed");
		
	return 0;
}
