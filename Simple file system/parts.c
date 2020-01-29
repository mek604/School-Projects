#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <endian.h>
#include <string.h>	
#include <time.h>
#include <libgen.h>
/*
	Author: Mek Obchey
	Student#: V00880355
	CSC 360 A01
	Fall 2019
	
	Assignment 3 - A simple file system
*/


struct __attribute__((__packed__)) superblock_t {
	uint8_t fs_id[8];
	uint16_t block_size;
	uint32_t file_system_block_count;
	uint32_t fat_start_block;
	uint32_t fat_block_count;
	uint32_t root_dir_start_block;
	uint32_t root_dir_block_count;
};

struct __attribute__((__packed__)) dir_entry_timedate_t {	
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

struct __attribute__((__packed__)) dir_entry_t {  
	uint8_t status;
	uint32_t starting_block;  
	uint32_t block_count;
	uint32_t size;
	struct dir_entry_timedate_t create_time; 
	struct dir_entry_timedate_t modify_time;  
	uint8_t filename[31];  
	uint8_t unused[6];
};


/*
	Print File Allocation Table (FAT) information
*/
void print_fat(int fat_start, int fat_blocks, int block_size, char* addr) {
	int free_blocks = 0, allocated_blocks = 0, reserved_blocks = 0;
	uint32_t f_block;
    for (int i = fat_start * block_size; i < (fat_start + fat_blocks) * block_size; i += 4) {
    	memcpy(&f_block, addr + i, sizeof(f_block));
    	if(htobe32(f_block) == 0)
    		free_blocks++;
    	else if(htobe32(f_block) == 1)
    		reserved_blocks++;
    	else
    		allocated_blocks++;
    	f_block = 0;
    }
	printf("\nFAT information:\n");
	printf("Free blocks: %d\n", free_blocks);
	printf("Reserved blocks: %d\n", reserved_blocks);
	printf("Allocated blocks: %d\n", allocated_blocks);	
}

void print_fs_info(struct superblock_t* sb_info, char* addr) {
	int block_size = htobe16(sb_info->block_size);
	int fs_blocks = htobe32(sb_info->file_system_block_count);
	int fat_start = htobe32(sb_info->fat_start_block);
	int fat_blocks = htobe32(sb_info->fat_block_count);
	int root_start = htobe32(sb_info->root_dir_start_block);
	int root_blocks = htobe32(sb_info->root_dir_block_count);

	printf("Super block information:\n");
	printf("Block size: %d\n", block_size);
	printf("Block count: %d\n", fs_blocks);
	printf("FAT starts: %d\n", fat_start);
	printf("FAT blocks: %d\n", fat_blocks);
	printf("Root directory start: %d\n", root_start); 
	printf("Root directory blocks: %d\n", root_blocks);

	print_fat(fat_start, fat_blocks, block_size, addr);
}


/*
	Only check bit 1 and 2 if the directory entry status is a file or a directory.
*/
char get_file_status(int x) {
	int k = 1;
	if(x & (k << 1))
		return 'F';
	else if(x & (k << 2))
		return 'D';
	else
		return '-';
}


void printTimestamp(struct dir_entry_timedate_t* timestamp) {
	printf("%02d/%02d/%02d ", htobe16(timestamp->year), timestamp->month, timestamp->day);
	printf("%02d:%02d:%02d\n", timestamp->hour, timestamp->minute, timestamp->second);
}

/*
	Find a subdir from a given path, if not found returns dir_info.
	
	@dir_name must be an  relative path to a directory other than roo.
	For example,
		if given path is /subdir/subdir1/subdir2, return subdir2

*/
struct dir_entry_t* find_subdir(char* addr, int block_size, char* dir_name, struct dir_entry_t* dir_info, int start, int end) {
	int i = start;
	int n = end;
	int dir_size = sizeof(struct dir_entry_t);	//size of directory entry
	char* next_dir = strtok(dir_name, "/");

	while(i < n && next_dir != NULL) {
		memcpy(dir_info, addr + i, dir_size);
		if(strcmp(dir_info->filename, next_dir) == 0 && get_file_status(dir_info->status) - 'D' == 0 && htobe32(dir_info->size) != 0) {
			i = htobe32(dir_info->starting_block) * block_size;
			n = i + htobe32(dir_info->block_count) * block_size;
			next_dir = strtok(NULL, "/");
			continue;
		}
		i += dir_size;
	}

	return dir_info;
}

/*
	Return the address of the file from a given relative path other than root, if exists.
*/
struct dir_entry_t* find_file(char* addr, int block_size, char* dir_name, struct dir_entry_t* dir_info, int start, int end) {
	int i = start;
	int n = end;
	int dir_size = sizeof(struct dir_entry_t);	//size of directory entry
	char* next_dir = strtok(dir_name, "/");

	while(i < n && next_dir != NULL) {
		memcpy(dir_info, addr + i, dir_size);
		if(strcmp(dir_info->filename, next_dir) == 0 && htobe32(dir_info->size) != 0) {
			i = htobe32(dir_info->starting_block) * block_size;
			n = i + htobe32(dir_info->block_count) * block_size;
			next_dir = strtok(NULL, "/");
			continue;
		}
		i += dir_size;
	}
	if(get_file_status(dir_info->status) - 'F' != 0 && strcmp(dir_info->filename, next_dir) != 0){
		printf("Error: File not Found\n");
		exit(EXIT_FAILURE);
	}
		
	return dir_info;
}

void print_directory(char* path, char* addr, struct dir_entry_t* dir_info, struct superblock_t* sb_info) {
	int block_size = htobe16(sb_info->block_size);
	int root_start = htobe32(sb_info->root_dir_start_block) * block_size;
	int root_end = root_start + htobe32(sb_info->root_dir_block_count) * block_size;
	int dir_size = sizeof(struct dir_entry_t);	//size of directory entry

	struct dir_entry_t* matching_dir;
	int i, n = 0;
	//if path is not given (or given as "/") point to root block
	if(path == NULL || (strcmp(path,"/") == 0)) {
	 	matching_dir = dir_info;
	 	i = root_start;
	 	n = root_end;
	}
	else {
		//point to subdirectory with matching path, if exists
		matching_dir = find_subdir(addr, block_size, path, dir_info, root_start, root_end);
		i = htobe32(matching_dir->starting_block) * block_size;
		n = i + htobe32(matching_dir->block_count) * block_size;
	}
	while(i < n) {
		memcpy(matching_dir, addr + i, dir_size);
		if(htobe32(matching_dir->size) != 0) {
			int i = htobe32(matching_dir->starting_block) * block_size;
			int n = i + htobe32(matching_dir->block_count) * block_size;
			struct dir_entry_timedate_t* timestamp = &matching_dir->modify_time;
			printf("%c %10d %30s ", get_file_status(matching_dir->status), htobe32(matching_dir->size), matching_dir->filename);
			printTimestamp(timestamp);
		}
		i += dir_size;
	}

}


void diskinfo(char* addr, int argc, char* argv[]) {
	struct superblock_t* sb_info = malloc(sizeof(struct superblock_t));
	memcpy(sb_info, addr, sizeof(struct superblock_t));
	print_fs_info(sb_info, addr);

	free(sb_info);
}

void disklist(char* addr, int argc, char* argv[]) {
	struct superblock_t* sb_info = malloc(sizeof(struct superblock_t));
	struct dir_entry_t* dir_info = malloc(sizeof(struct dir_entry_t));
	memcpy(sb_info, addr, sizeof(struct superblock_t));

	printf("\n");
	print_directory(argv[2], addr, dir_info, sb_info);

	free(sb_info);
	free(dir_info);
}


/*
	argv[2] = absolute path to the file in file system
	argv[3] = output file name 

*/
void diskget(char* addr, int argc, char* argv[]) {
	char* path = argv[2];
	char* output = argv[3];

	if(path == NULL) {
		printf("Error: the  relative path of a file to be copied is not specified.\n");
		exit(EXIT_FAILURE);
	}
	if(output == NULL) {
		printf("Error: the name of output file is not specified.\n");
		exit(EXIT_FAILURE);
	}

	struct superblock_t* sb_info = malloc(sizeof(struct superblock_t));
	struct dir_entry_t* dir_info = malloc(sizeof(struct dir_entry_t));
	memcpy(sb_info, addr, sizeof(struct superblock_t));

	int block_size = htobe16(sb_info->block_size);	//size of super block
	int root_start = htobe32(sb_info->root_dir_start_block) * block_size;
	int root_end = root_start + htobe32(sb_info->root_dir_block_count) * block_size;
	int dir_size = sizeof(struct dir_entry_t);	//size of directory entry
	
	int i = root_start;
	int n = root_end;
	struct dir_entry_t* matching_dir;

	//find the directory of the file to be copied
	if(path == NULL || (strcmp(path,"/") == 0))
	 	matching_dir = dir_info;	//set directory entry to root
	else {
		matching_dir = find_file(addr, block_size, path, dir_info, root_start, root_end);
		i = htobe32(matching_dir->starting_block) * block_size;
	}

	FILE* output_file = fopen(output, "w");

	if(output_file == NULL) {
		printf("Error: Cannot write to output file.\n");
		exit(EXIT_FAILURE);
	}

	fwrite(addr + i, htobe32(matching_dir->size), 1, output_file);
	printf("Successfully written to %s\n", output);

	fclose(output_file);
	free(sb_info);
	free(dir_info);
}



/*
	Count the number of free FAT blocks.
	Since uint512_t type does not exist, calculate free blocks using uint64_t instead.

	@block_size is the size of superblock, eg. 512 bytes

*/
int count_fat_blocks(char* addr, int fat_start, int fat_end, int block_size) {
	uint64_t f_block;
	int count = 0;
	int is_free;
    for (int i = fat_start; i < fat_end; i += block_size) {
   		is_free = 1;
    	for(int j=0; j<(block_size/sizeof(f_block)); j++) {
    		memcpy(&f_block, addr + i + (j*sizeof(f_block)), sizeof(f_block));
    		if(f_block != 0)
    			is_free = 0;
    	}
    	if(is_free)
    		count++;
    }
    return count;
}


/*
	Get the first location of free FAT block.

*/
int first_free_fat(char* addr, int fat_start, int fat_end, int block_size) {
	uint64_t f_block;
	int count = 0;
	int is_free;
    for (int i = fat_start; i < fat_end; i += block_size) {
   		is_free = 1;
    	for(int j=0; j<(block_size/sizeof(f_block)); j++) {
    		memcpy(&f_block, addr + i + (j*sizeof(f_block)), sizeof(f_block));
    		if(f_block != 0)
    			is_free = 0;
    	}
    	if(is_free)
    		return i;
    }
    return -1;
}

void get_current_time(struct dir_entry_timedate_t* ctime) {
	time_t rawtime;
	time(&rawtime);
	struct tm* time = localtime(&rawtime);
	ctime->year = htobe16(1900 + time->tm_year);
	ctime->month = time->tm_mon;
	ctime->day = time->tm_mday;
	ctime->hour = time->tm_hour;
	ctime->minute = time->tm_min;
	ctime->second = time->tm_sec;
}


void create_dir_entry(char* addr, struct dir_entry_t* new_entry, char* name, int start, off_t size, int blocks) {
	struct dir_entry_timedate_t* ctime = malloc(sizeof(struct dir_entry_timedate_t));
	get_current_time(ctime);
	new_entry->status = 'F';
	new_entry->starting_block = start;
	new_entry->block_count = blocks;
	new_entry->create_time = *ctime;
	new_entry->modify_time = *ctime;
	new_entry->size = htobe32(size);
	char* unused_label = "FFFFF";
	strcpy(new_entry->unused, unused_label);
	strcpy(new_entry->filename, name);
	//debug
	// printf("%c %10d %30s ", get_file_status(new_entry->status), htobe32(new_entry->size), new_entry->filename);
	// printTimestamp(ctime);
}


/*
	Put a file from local directory into disk image file.

*/
void diskput(char* addr, int argc, char* argv[]) {
	char* filename = argv[2];	//file name in the current directory
	char* path = argv[3];	//absolute path to a file in file system
	char* tmp = malloc(sizeof(char));
	

	if(filename == NULL) {
		printf("Error: the name of the input file is not specified.\n");
		exit(EXIT_FAILURE); 
	}

	if(path == NULL) {
		printf("Error: the  relative path in the file system is not specified.\n");
		exit(EXIT_FAILURE);
	}

	//separate path and filename at the end of path
	strcpy(tmp, argv[3]);
	path = dirname(path);
	char* path_f_name = basename(tmp);

	if(strcmp(path, ".") == 0)
		path = "/";

	//------------------------------- init ----------------------------------------------
	struct superblock_t* sb_info = malloc(sizeof(struct superblock_t));
	struct dir_entry_t* dir_info = malloc(sizeof(struct dir_entry_t));
	memcpy(sb_info, addr, sizeof(struct superblock_t));

	int block_size = htobe16(sb_info->block_size);	//size of super block
	int root_start = htobe32(sb_info->root_dir_start_block) * block_size;
	int root_end = root_start + htobe32(sb_info->root_dir_block_count) * block_size;
	int fat_start = htobe32(sb_info->fat_start_block) * block_size;
	int fat_end = fat_start + htobe32(sb_info->fat_block_count) * block_size;
	int dir_size = sizeof(struct dir_entry_t);	//size of directory entry
	
	int i = root_start;
	int n = root_end;
	struct dir_entry_t* matching_dir;


	if((strcmp(path,"/") == 0) || strtok(path, "/") == NULL) {
	 	matching_dir = dir_info;
	 	i = root_start;
	 	n = root_end;
	}
	else {
		matching_dir = find_subdir(addr, block_size, path, dir_info, root_start, root_end);
		i = htobe32(matching_dir->starting_block) * block_size;
		n = i + htobe32(matching_dir->block_count) * block_size;
	}

	int fd_in = open(filename, O_RDONLY);
	struct stat sb_in;
	char* addr_in;
	if(fd_in == -1) {
		printf("Error: Input file cannot be opened.\n");
		exit(EXIT_FAILURE);
	}
	if(fstat(fd_in, &sb_in) == -1) {
    	printf("Error: Cannot obtain file size\n");
    	exit(EXIT_FAILURE);
	}
    addr_in = mmap(NULL, sb_in.st_size, PROT_READ, MAP_SHARED, fd_in, 0);
	if(addr_in == MAP_FAILED) {
		printf("Error: MMAP failed\n");		
		exit(EXIT_FAILURE);
	}
	//-----------------------------------------------------------------------------------


	//find free FAT and the starting address of free FAT
	int fat_need = sb_in.st_size / block_size;
	int free_fat_blocks = count_fat_blocks(addr, fat_start, fat_end, block_size);	
	int loc = first_free_fat(addr, fat_start, fat_end, block_size);
	
	if(free_fat_blocks < fat_need) {
		printf("Error: Not enough free FAT blocks\n");		
		exit(EXIT_FAILURE);
	}
	if(loc == -1) {
		printf("Error: Fail to locate first free FAT block\n");		
		exit(EXIT_FAILURE);
	}

	//construct directory entry if free FAT is found
	struct dir_entry_t* new_entry = malloc(sizeof(struct dir_entry_t));
	create_dir_entry(addr, new_entry, path_f_name, loc, sb_in.st_size, fat_need);

	while(i < n) {
		memcpy(matching_dir, addr + i, dir_size);
		if(htobe32(matching_dir->size) == 0) {
			memcpy(addr + i, new_entry, dir_size);
			memcpy(addr + loc, addr_in, sb_in.st_size);
			break;
		}
		i += dir_size;
	}
	printf("Successfully written to %s\n", argv[1]);

	close(fd_in);
	free(sb_info);
	free(dir_info);
}

void diskfix(char* addr, int argc, char* argv[]) {

}

/*

Reference: http://man7.org/linux/man-pages/man2/mmap.2.html

*/
int main(int argc, char* argv[]) {
	int fd = open(argv[1], O_RDWR);
	struct stat sb;
	char* addr;

	if(fd == -1) {
		printf("Error: Input file cannot be opened.\n");
		exit(EXIT_FAILURE);
	}
	if (fstat(fd, &sb) == -1) {
    	printf("Error: Cannot obtain file size\n");
    	exit(EXIT_FAILURE);
	}

    addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (addr == MAP_FAILED) {
		printf("Error: MMAP failed\n");		
		exit(EXIT_FAILURE);
	}

	#if defined(PART1)
		diskinfo(addr, argc, argv);
	#elif defined(PART2)
		disklist(addr, argc, argv);
	#elif defined(PART3)
		diskget(addr, argc, argv);
	#elif defined(PART4)
		diskput(addr, argc, argv);
	#elif defined(PART5)
		diskfix(addr, argc, argv);
	#else
		printf("PART[12345] must be defined\n");
	#endif

	return 0;
}
