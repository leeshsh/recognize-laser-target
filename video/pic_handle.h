#ifndef _PIC_HANDLE_H
#define _PIC_HANDLE_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <config.h>
#include <disp_manager.h>
#include <video_manager.h>
#include <convert_manager.h>
#include <render.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <termios.h> 
#include <errno.h>

//#include "PIC131.h"

typedef enum {
    NOTHING  =0,
    REPORT    =1,
    CONTRAL  =2,
} SEND_FLAG;


typedef struct serial_arg
{
	int fd;
	char recv_buf[1024];
	char send_buf[1024];
	SEND_FLAG  send_flag;//发送标记
	unsigned  char  target_grade;//目标成绩
	unsigned char  wait_flag;//等待标记
	unsigned char  send_count;	
	
}T_serial_arg ,*PT_serial_arg;


typedef enum {
	MISS 	=0,
	RING5	=5,
	RING6	=6,
	RING7	=7,
	RING8	=8,
	RING9	=9,
	RING10	=10,
	
} GRADE_RESULT;

typedef enum {
	BLACK	=0,
	WHITE	=1
	
} LAND_POINT ;

typedef struct hand_pic {
	int iWidth;   /* 宽度: 一行有多少个象素 */
	int iHeight;  /* 高度: 一列有多少个象素 */
	int iTotalBytes; /* 所有字节数 */ 
	int iBpp;
	int red_max;
	int red_x;
	int red_y;
	int centre_x;
	int centre_y;
	GRADE_RESULT grade;
	LAND_POINT    laser_scope;
	int last_grade;
	int same_grade_count;
	
	T_serial_arg   serial_info;
	unsigned char *red_pix;//指向红色的点
	unsigned char *green_pix;  //指向绿色的点
}T_hand_pic, *PT_hand_pic;


extern int  handle_pic(PT_PixelDatas ptPixelDatas, PT_hand_pic green_info );
extern int alloc_handle(PT_hand_pic ptHandPic );
extern int init_uart1(PT_hand_pic ptHandPic);

#endif

