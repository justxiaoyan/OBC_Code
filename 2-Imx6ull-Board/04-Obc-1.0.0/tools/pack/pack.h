


#ifndef __PACK_H__
#define __PACK_H__

// 定义结构体
typedef struct OBC_PACK_HEAD
{
    char pack_file[32];
    char file_name[32];
    int file_size;
    int crc;
    char res[512 - 72];
} OBC_PACK_HEAD_T;





#endif


