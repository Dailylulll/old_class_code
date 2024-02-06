// handle directories differently, use vacantfile as size of directory, 
// dynamic array to handle taking and removing



#include "dyn_array.h"
#include "bitmap.h"
#include "block_store.h"
#include "FS.h"

#define BLOCK_STORE_NUM_BLOCKS 65536    // 2^16 blocks.
#define BLOCK_STORE_AVAIL_BLOCKS 65534  // Last 2 blocks consumed by the FBM
#define BLOCK_SIZE_BITS 32768           // 2^12 BYTES per block *2^3 BITS per BYTES
#define BLOCK_SIZE_BYTES 4096           // 2^12 BYTES per block
#define BLOCK_STORE_NUM_BYTES (BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES)  // 2^16 blocks of 2^12 bytes.

#define S(x) UNUSED(x);
#define E(x) UNUSED(x);
#define EXIT //
#define ENTER //

// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)


#define number_inodes 256
#define inode_size 64
#define number_fd 256
#define fd_size 6	// any number as you see fit

#define folder_number_entries 31

#define ROOT 0
//#define NAME name
//#define S(x) status_print(NAME, x);
//#define E(x) error_print(NAME, x);  // expand this to entere and exit ENTER EXIT
//#define EXIT exit_print(NAME);
//#define ENTER enter_print(NAME);
#define FUNCTION char * name =



int count = 0;

void enter_print(char * name) {
    printf("ENTERING: %s\n", name);
}
void exit_print(char * name) {
    printf("EXITING: %s\n", name);
}
void status_print(char * name, char * arg) {
    printf("S: In %s, %s\n",name, arg);
}
void error_print(char * name, char * arg) {
    printf("ERROR: In %s, %s\n",name, arg);
}

// each inode represents a regular file or a directory file
struct inode 
{
    uint32_t vacantFile;    // this parameter is only for directory. Used as a bitmap denoting availibility of entries in a directory file.
    char owner[18];         // for alignment purpose only   

    uint8_t fileType;          // '0' denotes regular file, '1' denotes directory file // switch this to type enum!!!

    size_t inodeNumber;			// for FS, the range should be 0-255
    size_t fileSize; 			  // the unit is in byte	
    size_t linkCount;

    // to realize the 16-bit addressing, pointers are acutally block numbers, rather than 'real' pointers.
    uint16_t directPointer[6];
    uint16_t indirectPointer[1];
    uint16_t doubleIndirectPointer;
};


struct fileDescriptor 
{
    uint8_t inodeNum;	// the inode # of the fd

    // usage, locate_order and locate_offset together locate the exact byte at which the cursor is 
    uint8_t usage; 		// inode pointer usage info. Only the lower 3 digits will be used. 1 for direct, 2 for indirect, 4 for dbindirect
    uint16_t locate_order;		// serial number or index of the block within direct, indirect, or dbindirect range
    uint16_t locate_offset;		// offset of the cursor within a block
};

int init_fd(fileDescriptor_t * fd, uint8_t inode_num, uint8_t usage, uint16_t locate_order, uint16_t locate_offset) {
    FUNCTION "init_fd";
    UNUSED(name);
    ENTER
    if(!fd) {
        E("invalid initital param")
        EXIT
        return -1;
    }
    fd->inodeNum = inode_num;
    fd->usage = usage;
    fd->locate_offset = locate_offset;
    fd->locate_order = locate_order;
    return 0;
}


struct directoryFile {
    char filename[127];
    uint8_t inodeNumber;
};


struct FS {
    block_store_t * BlockStore_whole;
    block_store_t * BlockStore_inode;
    block_store_t * BlockStore_fd;
};

typedef struct {
    char path[number_inodes * (FS_FNAME_MAX + 1)];
    char p_path[number_inodes][FS_FNAME_MAX];
    size_t size;
} path_t;


///////////////////////////////////////////////////////////////////////////////////////////////////////////

void print_inode(inode_t * inode) {
    printf("Inode size = %ld  ", sizeof(inode_t));
    printf("Inode num = %ld Inode type : %d  VacantFile: %d   Direct ptr: %d  Links: %ld\n",inode->inodeNumber, inode->fileType, inode->vacantFile, inode->directPointer[0], inode->linkCount);
}

int block_write(FS_t * fs, const size_t block_id, void * buffer, block_t action) {
    FUNCTION "block_write";
    UNUSED(name);
    ENTER
    if(!(fs && buffer)) {
        E("invalid initial params");
        return -1;
    }
    switch(action) {
        case INODE:
            block_store_inode_write(fs->BlockStore_inode, block_id, buffer);
            EXIT
            return 0;
        case FD:
            block_store_fd_write(fs->BlockStore_fd, block_id, buffer);
            EXIT
            return 0;
        case WHOLE:
            block_store_write(fs->BlockStore_whole, block_id, buffer);
            EXIT
            return 0;
        default:
            E("undefined switch behavior")
            EXIT
            return -1;
    }

}

int block_read(FS_t * fs, const size_t block_id, void * buffer, block_t action) {
    FUNCTION"block_read";
    UNUSED(name);
    ENTER
    if(!(fs && buffer)) {
        E("passed invalid initial params")
        return -1;
    }
    switch(action) {
        case INODE:
            block_store_inode_read(fs->BlockStore_inode, block_id, buffer);
            EXIT
            return 0;
        case FD:
            block_store_fd_read(fs->BlockStore_fd, block_id, buffer);
            EXIT
            return 0;
        case WHOLE:
            block_store_read(fs->BlockStore_whole, block_id, buffer);
            EXIT
            return 0;
        default:
            E("undefined behavior in switch statement")
            EXIT
            return -1;
    }

}

dyn_array_t * read_dir(FS_t * fs, inode_t * inode) {
    FUNCTION "read_dir";
    UNUSED(name);
    ENTER
    if(!(fs && inode)) {
        E("passed invalid initial params")
        EXIT
        return NULL;
    }
    uint8_t block[BLOCK_SIZE_BYTES];
    block_read(fs, inode->directPointer[0], block, WHOLE);
    dyn_array_t * dir = dyn_array_import(block, folder_number_entries + 1, sizeof(directoryFile_t), NULL);
    EXIT
    return dir;
}

int write_dir(FS_t * fs, dyn_array_t * dir, inode_t * inode) {
    FUNCTION "write_dir";
    UNUSED(name);
    ENTER
    if(!(fs && dir && inode)) {
        E("passed initial invalid params");
        EXIT
        return -1;
    }
    uint8_t block[BLOCK_SIZE_BYTES];
    const void * const dir_ptr = dyn_array_export(dir);
    memcpy(block, dir_ptr, sizeof(directoryFile_t) * inode->vacantFile);
    block_write(fs, inode->directPointer[0], block, WHOLE);
    dyn_array_destroy(dir);
    EXIT
    return 0;
}

void print_dir(dyn_array_t * dir, inode_t * inode) {
    for(size_t i = 0; i < inode->vacantFile; ++i) {
        puts(((directoryFile_t*)dyn_array_at(dir, i))->filename);
    }
    if(dyn_array_size(dir) == 0) {
        puts("Empty dir");
    }
}

int parse_path(path_t * s_path, const char * path) {
    FUNCTION "parse path";
    UNUSED(name);
    ENTER
    if(!(path && s_path)) {
        E("passed invalid initial params")
        EXIT
        return -1;
    }
    strcpy(s_path->path, path);
    if(s_path->path[0] != '/' || (strlen(s_path->path) > 1 && s_path->path[strlen(s_path->path) - 1] == '/')) {
        E("path does not start with /")
        EXIT
        return -1;
    }
    char * delim = "/";
    strcpy(s_path->p_path[s_path->size++], "root");
    char * token = NULL;
    token = strtok(s_path->path, delim);
    while(token) {  // problem here, the way you are incementing is making your size too big
        if(strlen(token) > 126 || !strcmp(token, "\0")) {
            E("invalid token")
            EXIT
            return -1;
        }
        //puts(token);
        strcpy(s_path->p_path[s_path->size++], token);
        token = strtok(NULL, delim);
    }
    EXIT
    return 0;
}

void print_p_path(path_t * path) {
    if(path) {
        printf("%ld  ", path->size);
        for(size_t i = 0; i < path->size; ++i) {
            printf("%s\n", path->p_path[i]);
        }
        puts("");
    }
}

// tripping up on the root case
int search_directory(char * search, int * inode_num, dyn_array_t * dir, inode_t * inode) {
    FUNCTION "search directory";
    UNUSED(name);
    ENTER
    if(!(search && dir && inode_num)) {
        E("invalid initial params")
        EXIT
        return -1;
    }
    //printf("Search filename: "); puts(search);
    //printf("directory contents: "); print_dir(dir, inode);
    for(size_t i = 0; i <= inode->vacantFile; ++i) {
        //printf("Directory entry:"); puts(((directoryFile_t*)dyn_array_at(dir, i))->filename);
        if(!strcmp(search, ((directoryFile_t*)dyn_array_at(dir, i))->filename)) {
            *inode_num = (int)((directoryFile_t*)dyn_array_at(dir, i))->inodeNumber;
            EXIT
            return 0;
        }
    }
    S("did not find file in directory");
    EXIT
    return 1;
}

int alloc_block(FS_t * fs, size_t * block_id, block_t flag) {
    FUNCTION "alloc block";
    UNUSED(name);
    ENTER
    if(!(fs && block_id)) {
        E("invalid initial params");
        EXIT
        return -1;
    }
    switch(flag) {
        case INODE:
            *block_id = block_store_sub_allocate(fs->BlockStore_inode);
            break;
        case FD:
            *block_id = block_store_sub_allocate(fs->BlockStore_fd);
            break;
        case WHOLE:
            *block_id = block_store_allocate(fs->BlockStore_whole);
            break;
        default:
            E("undefined switch behavior")
            EXIT
            return -1;
    }
    if(*block_id == SIZE_MAX) {
        E("ran out of blocks");
        EXIT
        return -1;
    }
    EXIT
    return 0;
}

int dealloc_block(FS_t * fs, size_t block_id, block_t flag) {
    FUNCTION "dealloc block";
    UNUSED(name);
    ENTER
    if(!(fs)) {
        E("invalid initial params")
        EXIT
        return -1;
    }
    switch(flag) {
        case INODE:
            block_store_sub_release(fs->BlockStore_inode, block_id);
            EXIT
            return 0;
        case FD:
            block_store_sub_release(fs->BlockStore_fd, block_id);
            EXIT
            return 0;
        case WHOLE:
            block_store_release(fs->BlockStore_whole, block_id);
            EXIT
            return 0;
        default:
            E("undefined switch behavior")
            EXIT
            return -1;
    }
}

int init_inode(inode_t * inode, file_t fileType, size_t inodeNumber, uint16_t block_id) {
    FUNCTION"init inode";
    UNUSED(name);
    ENTER
    if(!inode) {
        E("passed invalid initial param")
        EXIT
        return -1;
    }
    inode->vacantFile = 0;
    if(fileType == FS_DIRECTORY) inode->fileType = 1;
    else inode->fileType = 0;
    inode->inodeNumber = inodeNumber;
    inode->fileSize = 0;
    inode->linkCount++;
    inode->directPointer[0] = block_id;
    EXIT
    return 0;
}


//  get block function
// read and write, and root
// on root, alloc block no matter what
// on read, if block isnt allocated dont alloc, return -1;
// on write, if block isnt allocated, assign one
// test_block func

int test_block(FS_t * fs, inode_t * inode) {
    FUNCTION"test block";
    UNUSED(name);
    if(!inode) {
        E("passed invalid initial params");
        return -1;
    }
    //print_inode(inode);
    if(inode->directPointer[0] == 0) {
        size_t block_id = 0;
        size_t * block_id_ptr = &block_id;
        if(alloc_block(fs, block_id_ptr, WHOLE)) {
            E("alloc block returned failure");
            EXIT
            return -1;
        }
        inode->directPointer[0] = (uint16_t)block_id;
        EXIT
        return 0;
    } else {
        EXIT
        return 0;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////








/// Formats (and mounts) an FS file for use
/// \param fname The file to format
/// \return Mounted FS object, NULL on error
///
FS_t *fs_format(const char *path)
{
    if(path != NULL && strlen(path) != 0)
    {
        FS_t * ptr_FS = (FS_t *)calloc(1, sizeof(FS_t));	// get started
        ptr_FS->BlockStore_whole = block_store_create(path);				// pointer to start of a large chunck of memory

        // reserve the 1st block for bitmap of inode
        size_t bitmap_ID = block_store_allocate(ptr_FS->BlockStore_whole);
        //		printf("bitmap_ID = %zu\n", bitmap_ID);

        // 2rd - 5th block for inodes, 4 blocks in total
        size_t inode_start_block = block_store_allocate(ptr_FS->BlockStore_whole);
        //		printf("inode_start_block = %zu\n", inode_start_block);		
        for(int i = 0; i < 3; i++)
        {
            block_store_allocate(ptr_FS->BlockStore_whole);
            //			printf("all the way with block %zu\n", block_store_allocate(ptr_FS->BlockStore_whole));
        }

        // install inode block store inside the whole block store
        ptr_FS->BlockStore_inode = block_store_inode_create(block_store_Data_location(ptr_FS->BlockStore_whole) + bitmap_ID * BLOCK_SIZE_BYTES, block_store_Data_location(ptr_FS->BlockStore_whole) + inode_start_block * BLOCK_SIZE_BYTES);

        // the first inode is reserved for root dir
        block_store_sub_allocate(ptr_FS->BlockStore_inode);
        //		printf("first inode ID = %zu\n", block_store_sub_allocate(ptr_FS->BlockStore_inode));

        // update the root inode info.
        uint8_t root_inode_ID = 0;	// root inode is the first one in the inode table
        inode_t * root_inode = (inode_t *) calloc(1, sizeof(inode_t));
        //		printf("size of inode_t = %zu\n", sizeof(inode_t));
        root_inode->vacantFile = 0x00000000;
        root_inode->fileType = 1;								
        root_inode->inodeNumber = root_inode_ID;
        root_inode->linkCount = 1;
        //		root_inode->directPointer[0] = root_data_ID;	// not allocate date block for it until it has a sub-folder or file
        block_store_inode_write(ptr_FS->BlockStore_inode, root_inode_ID, root_inode);		
        free(root_inode);

        // now allocate space for the file descriptors
        ptr_FS->BlockStore_fd = block_store_fd_create();

        return ptr_FS;
    }

    return NULL;	
}

///
/// Mounts an FS object and prepares it for use
/// \param fname The file to mount

/// \return Mounted FS object, NULL on error

///
FS_t *fs_mount(const char *path)
{
    if(path != NULL && strlen(path) != 0)
    {
        FS_t * ptr_FS = (FS_t *)calloc(1, sizeof(FS_t));	// get started
        ptr_FS->BlockStore_whole = block_store_open(path);	// get the chunck of data	

        // the bitmap block should be the 1st one
        size_t bitmap_ID = 0;

        // the inode blocks start with the 2nd block, and goes around until the 5th block, 4 in total
        size_t inode_start_block = 1;

        // attach the bitmaps to their designated place
        ptr_FS->BlockStore_inode = block_store_inode_create(block_store_Data_location(ptr_FS->BlockStore_whole) + bitmap_ID * BLOCK_SIZE_BYTES, block_store_Data_location(ptr_FS->BlockStore_whole) + inode_start_block * BLOCK_SIZE_BYTES);

        // since file descriptors are allocated outside of the whole blocks, we can simply reallocate space for it.
        ptr_FS->BlockStore_fd = block_store_fd_create();

        return ptr_FS;
    }

    return NULL;		
}




///
/// Unmounts the given object and frees all related resources
/// \param fs The FS object to unmount
/// \return 0 on success, < 0 on failure
///
int fs_unmount(FS_t *fs)
{
    if(fs != NULL)
    {	
        block_store_inode_destroy(fs->BlockStore_inode);

        block_store_destroy(fs->BlockStore_whole);
        block_store_fd_destroy(fs->BlockStore_fd);

        free(fs);
        return 0;
    }
    return -1;
}

// in error case print off variables
int fs_create(FS_t *fs, const char *path, file_t type)  
{
    FUNCTION "fs_create";
    UNUSED(name);
    if(!(fs && path && strlen(path) > 1 && type < 2)) {
        E("Invalid initial params");
        EXIT
        return -1;
    }
    //puts(path);
    path_t path_var;
    path_var.size = 0;
    path_t * path_ptr = &path_var;
    if(parse_path(path_ptr, path)) {
        E("parse path returned error");
        EXIT
        return -1;
    }
    //print_p_path(path_ptr);
    inode_t inode_var;
    inode_t * inode_ptr = &inode_var;
    block_read(fs, ROOT, inode_ptr, INODE);
    if(test_block(fs, inode_ptr)) {
        E("test block returned error");
        EXIT
        return -1;
    }
    dyn_array_t* dir_ptr = read_dir(fs, inode_ptr);
    if(!dir_ptr) {
        E("read dir returned null")
        EXIT
        return -1;
    }
    int inode_number = 0;
    int * inode_number_ptr = &inode_number;
    for(size_t i = 1; i < path_ptr->size - 1; ++i) {
        if(!search_directory(path_ptr->p_path[i], inode_number_ptr, dir_ptr, inode_ptr)) {
            block_read(fs, inode_number, inode_ptr, INODE);
            if(test_block(fs, inode_ptr)) {
                E("test block error")
                EXIT                
                return -1;
            }
            dyn_array_destroy(dir_ptr);
            dir_ptr = read_dir(fs, inode_ptr);
        } else {
            S("file not found");
            EXIT
            return -1;
        }
    }
    if(inode_ptr->fileType == 0) {
        E("path terminal not a directory")
        EXIT
        return -1;
    }
    if(inode_ptr->vacantFile >= folder_number_entries) {
        E("directory full");
        EXIT
        return -1;
    }
    if(!search_directory(path_ptr->p_path[path_ptr->size -1], inode_number_ptr, dir_ptr, inode_ptr)) {
        E("conflicting file name");
        EXIT
        return -1;
    }
    inode_t new_node;
    inode_t * new_node_ptr = &new_node;
    size_t new_inode_id = 0;
    size_t * new_inode_id_ptr = &new_inode_id;
    if(alloc_block(fs, new_inode_id_ptr, INODE)) {
        E("alloc block returned error");
        EXIT
        return -1;
    }
    if(init_inode(new_node_ptr, type, *new_inode_id_ptr, 0)) {
        E("failed to init new node");
        EXIT
        return -1;
    }
    //print_inode(inode_ptr);
    //print_inode(new_node_ptr);
    directoryFile_t new_dir_entry;
    strcpy((&new_dir_entry)->filename, path_ptr->p_path[path_ptr->size - 1]);
    new_dir_entry.inodeNumber = new_inode_id;
    dyn_array_push_front(dir_ptr, &new_dir_entry);
    inode_ptr->vacantFile++;
    block_write(fs, new_inode_id, new_node_ptr, INODE);
    write_dir(fs, dir_ptr, inode_ptr);
    block_write(fs, inode_ptr->inodeNumber, inode_ptr, INODE);
    EXIT
    return 0;
}

int fs_open(FS_t *fs, const char *path)  // needs to create a new file if not dir
{
    FUNCTION "fs_open";
    UNUSED(name);
    ENTER
    if(!(fs && path && strlen(path))) {
        E("Invalid initial params");
        EXIT
        return -1;
    }
    if(strlen(path) == 1 && path[0] == '/') {
        E("cannot open root")
        EXIT
        return -1;
    }
    //puts(path);
    path_t path_var;
    path_var.size = 0;
    path_t * path_ptr = &path_var;
    if(parse_path(path_ptr, path)) {
        E("parse path returned error");
        EXIT
        return -1;
    }
    //print_p_path(path_ptr);
    inode_t inode_var;
    inode_t * inode_ptr = &inode_var;
    block_read(fs, ROOT, inode_ptr, INODE);
    if(test_block(fs, inode_ptr)) {
        E("test block returned error");
        EXIT
        return -1;
    }
    dyn_array_t* dir_ptr = read_dir(fs, inode_ptr);
    if(!dir_ptr) {
        E("read dir returned null")
        EXIT
        return -1;
    }
    int inode_number = 0;
    int * inode_number_ptr = &inode_number;
    for(size_t i = 1; i < path_ptr->size - 1; ++i) {
        if(!search_directory(path_ptr->p_path[i], inode_number_ptr, dir_ptr, inode_ptr)) {
            block_read(fs, inode_number, inode_ptr, INODE);
            if(test_block(fs, inode_ptr)) {
                E("test block error")
                EXIT                
                return -1;
            }
            dyn_array_destroy(dir_ptr);
            dir_ptr = read_dir(fs, inode_ptr);
            
        } else {
            S("file not found");
            EXIT
            return -1;
        }
    }
    if(inode_ptr->fileType == 0) {
        E("path terminal not a directory")
        EXIT
        return -1;
    }

    inode_t new_node;
    inode_t * new_node_ptr = &new_node;
    size_t new_inode_id = 0;
    size_t * new_inode_id_ptr = &new_inode_id;
    
    int foundFile = 0;
    if(!search_directory(path_ptr->p_path[path_ptr->size -1], inode_number_ptr, dir_ptr, inode_ptr)) {
        foundFile = 1;
        block_read(fs, inode_number, new_node_ptr, INODE);
        if(new_node_ptr->fileType == 1) {
            E("cannot open directory filetype")
            EXIT
            return -1;
        }
    }

    switch(foundFile) {
        case 0:
            if(inode_ptr->vacantFile >= folder_number_entries) {
                E("directory full");
                EXIT
                return -1;
            }
            if(alloc_block(fs, new_inode_id_ptr, INODE)) {
                E("alloc block returned error");
                EXIT
                return -1;
            }
            if(init_inode(new_node_ptr, 0, *new_inode_id_ptr, 0)) {
                E("failed to init new node");
                EXIT
                return -1;
            }
            directoryFile_t new_dir_entry;
            strcpy((&new_dir_entry)->filename, path_ptr->p_path[path_ptr->size - 1]);
            new_dir_entry.inodeNumber = new_inode_id;
            dyn_array_push_front(dir_ptr, &new_dir_entry);
            inode_ptr->vacantFile++;
            block_write(fs, new_inode_id, new_node_ptr, INODE);
            write_dir(fs, dir_ptr, inode_ptr);
            block_write(fs, inode_ptr->inodeNumber, inode_ptr, INODE);            
        case 1:
            break;
        default:
            E("undefined switch behavior in open file")
            EXIT
            return -1;
    }
    size_t fd_id = 0;
    if(alloc_block(fs, &fd_id, FD)) {
        E("error allocing fd block");
        EXIT
        return -1;
    }
    fileDescriptor_t fd_var;
    if(init_fd(&fd_var, new_node_ptr->inodeNumber, 0, new_node_ptr->directPointer[0], 0)) {  // this procedure will be edited later for indirect
        E("problem with init fd")
        EXIT
        return -1;
    }
    block_write(fs, fd_id, &fd_var, FD);
    return fd_id;
    
}

int fs_close(FS_t *fs, int fd)
{
    FUNCTION "fs_close";
    UNUSED(name);
    ENTER
    if(!(fs || fd < 0 || fd > 255)) {
        E("invalid initial params")
        EXIT
        return -1;
    }
    if(block_store_sub_test(fs->BlockStore_fd, fd)) {
        block_store_sub_release(fs->BlockStore_fd, fd);
        return 0;
    } else {
        return -1;
    }

}

off_t fs_seek(FS_t *fs, int fd, off_t offset, seek_t whence)
{
    /*
        check iniitial params
        check fd
        load inode
        from whence apply offset
        check offset to make sure it falls within BOF and EOF
        update fd
        return new offset
    */
    UNUSED(fs);
    UNUSED(fd);
    UNUSED(offset);
    UNUSED(whence);
    return 0;
}

ssize_t fs_read(FS_t *fs, int fd, void *dst, size_t nbyte)
{
    /*
        Checks init params
        checks to make sure fd is properly initiated
        loads up inode for the file
        get variable to keep track of dst offset
        write file to new buffer size of block
        from the offset in fd, read up to end of block
        then read blocks nbyte % BLOCK size
        and lastly read the remaining bytes in the last block
        if eof is found, read stops, and buffer is eof'd
    */
    UNUSED(fs);
    UNUSED(fd);
    UNUSED(dst);
    UNUSED(nbyte);
    return 0;
}

ssize_t fs_write(FS_t *fs, int fd, const void *src, size_t nbyte)
{
    /*
        Checks init params
        checks to make sure fd is properly initiated
        loads up inode for the file
        get variable to keep track of dst offset
        load block to new buffer size of block,
        then rewrite buffer to block
        from offset write buffer data into file blocks
        if end of block, allocate new block to inode
        update fd offsets and file size in inode
    */
    UNUSED(fs);
    UNUSED(fd);
    UNUSED(src);
    UNUSED(nbyte);
    return 0;
}

int fs_remove(FS_t *fs, const char *path)
{
    /*
        check initial params
        navigate path to file
        check link count of file
        check contents if directory
        check for open fd
        free block and inode
        update directory
    */
    UNUSED(fs);
    UNUSED(path);
    return 0;
}

dyn_array_t *fs_get_dir(FS_t *fs, const char *path)
{
    FUNCTION "fs_get_dir";
    UNUSED(name);
    ENTER
    if(!(fs && path && strlen(path))) {
        E("Invalid initial params");
        EXIT
        return NULL;
    }
    //puts(path);
    path_t path_var;
    path_var.size = 0;
    path_t * path_ptr = &path_var;
    if(parse_path(path_ptr, path)) {
        E("parse path returned error");
        EXIT
        return NULL;
    }
    //print_p_path(path_ptr);
    inode_t inode_var;
    inode_t * inode_ptr = &inode_var;
    block_read(fs, ROOT, inode_ptr, INODE);
    if(test_block(fs, inode_ptr)) {
        E("test block returned error");
        EXIT
        return NULL;
    }
    dyn_array_t* dir_ptr = read_dir(fs, inode_ptr);
    if(!dir_ptr) {
        E("read dir returned null")
        EXIT
        return NULL;
    }
    int inode_number = 0;
    int * inode_number_ptr = &inode_number;
    for(size_t i = 1; i <= path_ptr->size - 1; ++i) {
        if(!search_directory(path_ptr->p_path[i], inode_number_ptr, dir_ptr, inode_ptr)) {
            block_read(fs, inode_number, inode_ptr, INODE);
            if(test_block(fs, inode_ptr)) {
                E("test block error")
                EXIT                
                return NULL;
            }
            dyn_array_destroy(dir_ptr);
            dir_ptr = read_dir(fs, inode_ptr);
        } else {
            S("file not found");
            EXIT
            return NULL;
        }
    }
    if(inode_ptr->fileType == 0) {
        E("path terminal not a directory")
        EXIT
        return NULL;
    }
    dyn_array_t * new_dir = NULL;
    if(inode_ptr->vacantFile == 0) {
        new_dir = dyn_array_create(0, sizeof(directoryFile_t), NULL);
    } else {
        new_dir = dyn_array_import(dyn_array_export(dir_ptr), inode_ptr->vacantFile, sizeof(directoryFile_t), NULL);
    }    
    dyn_array_destroy(dir_ptr);
    EXIT
    return new_dir;
}

int fs_move(FS_t *fs, const char *src, const char *dst)
{
    /*
        check initial params
        navigate to source and check for path validity
        navigate to destination and check for path validity
        delete old directory entry and rewrite directory info
        insert new directory entry at source and update directory info
        do not update links
    */
    UNUSED(fs);
    UNUSED(src);
    UNUSED(dst);
    return 0;
}

int fs_link(FS_t *fs, const char *src, const char *dst)
{
    /*
        check initial params
        navigate to source and check for path validity
        navigate to destination and check for path validity
        copy directory entry and add it destination directory
        write new directory info
        increment link in file inode and rewrite file to block
    */
    UNUSED(fs);
    UNUSED(src);
    UNUSED(dst);
    return 0;
}

// link not zeroing out link bewtween allocs?