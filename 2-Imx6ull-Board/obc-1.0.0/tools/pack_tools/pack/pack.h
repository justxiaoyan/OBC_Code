


#ifndef __PACK_H__
#define __PACK_H__

// 定义结构体
typedef struct OBC_PACK_HEAD
{
    char magic[6];          /*OBCFS*/
    int file_size;
    int crc;
    char pack_file[32];
    char file_name[32];
    char res[512 - 78];
}__attribute__((packed)) OBC_PACK_HEAD_T;





#endif


