#include "pic_handle.h"
#include "PIC131.h"
#include <math.h>
#include <time.h>
static int get_info(PT_PixelDatas ptPixelDatas, PT_hand_pic ptHandPic);
int  test (PT_hand_pic ptHandPic);
static int  find_laser(PT_hand_pic ptHandPic);
static  int  otsu_method(PT_hand_pic ptHandPic);
static int  find_bound_x(PT_hand_pic ptHandPic,int  start_x, int end_x ,
						int start_y, int end_y  );
static int  find_bound_y(PT_hand_pic ptHandPic,int  start_x, int end_x ,
						int start_y, int end_y  );

static int find_bound(PT_hand_pic ptHandPic,int  *start_x, int *end_x ,
						int *start_y, int *end_y  );

static int check_laser(PT_hand_pic ptHandPic,int start_x, 
					int end_x, int start_y, int end_y);


static int find_coordinate(PT_hand_pic ptHandPicint,int start_x, int end_x,
										 int start_y, int end_y);
static int find_coordinate_x(PT_hand_pic ptHandPic,int start_x, 
					int end_x, int start_y, int interval);

static void calculate_grade(PT_hand_pic ptHandPic , int start_x, int end_x,
									       int start_y, int end_y);

//static void display(PT_PixelDatas ptPixelDatas, PT_hand_pic ptHandPic);
static void display(PT_PixelDatas ptPixelDatas, PT_hand_pic ptHandPic,
						int start_x, int end_x, int start_y, int end_y);

static int set_parity(int fd, int databits, int stopbits, int parity) ;
static int set_speed(int fd, int speed);

static void check_uart1(PT_hand_pic green_info);
int alloc_handle(PT_hand_pic ptHandPic )
{

	//ptHandPic=malloc(sizeof(T_hand_pic));
	//if(ptHandPic==NULL)
	//	return -1;
	ptHandPic->last_grade=99;
	ptHandPic->green_pix=malloc(320*240 );
	if(ptHandPic->green_pix==NULL)
		return -1;
	
	ptHandPic->red_pix=malloc(320*240 );
	if(ptHandPic->red_pix==NULL)
		return -1;

	return 0;


}

int  handle_pic(PT_PixelDatas ptPixelDatas, PT_hand_pic green_info)
{	
	int ret;
	int bound_x1, bound_x2;
	int bound_y1, bound_y2;

//	time_t timep;
//	time (&timep);
//	printf("%s\n",ctime(&timep));
	/*
	memset(green_info->serial_info.send_buf, 0xff, 1);
	memset(green_info->serial_info.send_buf, 0xfe, 1);
	write(green_info->serial_info.fd, green_info->serial_info.send_buf, strlen(green_info->serial_info.send_buf));
	memset(green_info->serial_info.recv_buf, 0, sizeof(green_info->serial_info.recv_buf));
	if(read(green_info->serial_info.fd, green_info->serial_info.recv_buf, 1024)>0)
            printf("Pri: %s\n", green_info->serial_info.recv_buf);
*/
	//printf("iBpp:%d totle%d\n",ptPixelDatas->iBpp,ptPixelDatas->iTotalBytes);
	check_uart1(green_info);

	if(10){
	//	printf("start\n");
		ret=get_info(ptPixelDatas,green_info);
		if(ret)
			return -1;

	}else
		test(green_info);

 	//printf("point1\n");
	ret = find_laser(green_info);
	if(ret){
		green_info->red_x  =  320;
		green_info->red_y  = 240;
		green_info->grade  = MISS ;//脱靶
		//memset(green_info->serial_info.send_buf+2, 0xee, 1);
	//	printf("can not find laser\n");
		return 0;
	}
//	printf("point2\n");
	ret=otsu_method(green_info);
	if(ret){
		//memset(green_info->serial_info.send_buf+2, 0xed, 1);
		printf("otsu_method error\n");
        return -1;
	}
	
  //  printf("point3\n");
	ret=find_bound( green_info,&bound_x1,  &bound_x2,
				&bound_y1, &bound_y2);
	if(ret){
		//memset(green_info->serial_info.send_buf+2, 0xec, 1);
		return -1;
	}
		
//	 printf("point4\n");
	if((green_info->red_x > bound_x2) ||(green_info->red_x < bound_x1) ||
		(green_info->red_y > bound_y2) ||(green_info->red_y < bound_y1) ) {
		green_info->grade=MISS ;//脱靶
	//	memset(green_info->serial_info.send_buf+2, 0xea, 1);
		return 0;
	}
	//printf("point3\n");
	check_laser(green_info,bound_x1, bound_x2, bound_y1, bound_y2);
    //printf("laser cope %d\n",green_info->laser_scope);
	
	find_coordinate(green_info,bound_x1, bound_x2,bound_y1,bound_y2);
//	printf("point4\n");
	calculate_grade(green_info,bound_x1, bound_x2,bound_y1,bound_y2);
	display(ptPixelDatas,green_info,bound_x1,bound_x2,bound_y1,bound_y2);	
	return 0;
    
}



static void check_uart1(PT_hand_pic green_info)
{
	int ret;
	PT_serial_arg serial ;
	serial=&green_info->serial_info;
	
	//printf("fd %d  ",green_info->serial_info.fd);
	//printf("%d \n",serial->fd);
	 //write(serial->fd, serial->send_buf, strlen(serial->send_buf));
	ret = read(serial->fd, serial->recv_buf, 1024);
	//if(ret>0)
	//	printf("len %d",ret);
	if((ret==7)&& (serial->recv_buf[0]=0xfe) &&(serial->recv_buf[6]=0xfe)){
	
		if ((serial->recv_buf[1]==3)&&(serial->recv_buf[2]==3)&&
				(serial->recv_buf[3]==3)&&(serial->recv_buf[4]==3))
			 serial->send_flag=REPORT;
			
		//	printf("come in\n");
				
		if ((serial->recv_buf[1]==2)&&(serial->recv_buf[2]==2)
							&&(serial->recv_buf[3]==2)){
							
		//	printf("come here\n");
			serial->send_flag=CONTRAL;
			serial->send_buf[0] = 0xfe;
			serial->send_buf[1] = 0x8;
			serial->send_buf[2] = 0x8;
			serial->send_buf[3] = 0x8;
			serial->send_buf[4] = 0x8;
			serial->send_buf[5] = green_info->serial_info.recv_buf[5];
			serial->send_buf[6] = 0xef;
		//	serial->send_buf[7] = '\n';
					
			serial->target_grade=serial->recv_buf[4];
			serial->wait_flag      =0;			
				
	//		write(serial->fd, serial->send_buf, 7);
			printf("receive grade%d \n",serial->target_grade);		

		}
		
		if ((serial->recv_buf[1]==8)&&(serial->recv_buf[2]==8)&&
			(serial->recv_buf[3]==8)&&(serial->recv_buf[4]==8)
			&&(serial->recv_buf[5]==serial->send_count)){
				serial->wait_flag      =0;
				printf("receive motor info\n");		
		}
	}		
	
}



//下面是对串口方面的操作


static int set_speed(int fd, int speed)
{ 
	int i;   
	int status;   
	static int speed_arr[] = {B230400, B115200, B57600, B38400, 	
				B19200, B9600, B4800, B2400, B1800, B1200, B600, B300};

	static int name_arr[]  = {230400,  115200,  57600,  38400,  
				19200,  9600,  4800,  2400,  1800,  1200,  600,  300};
	struct termios Opt; 
	
	tcgetattr(fd, &Opt);   
	
	for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++)
	{   
		if (speed == name_arr[i]) 
		{   
			tcflush(fd, TCIOFLUSH);   
			cfsetispeed(&Opt, speed_arr[i]);   
			cfsetospeed(&Opt, speed_arr[i]);  
			
			status = tcsetattr(fd, TCSANOW, &Opt);   
			if (status != 0) 
			{   
				perror("tcsetattr fd1");   
				return -1;
			}  
			
			tcflush(fd,TCIOFLUSH);  
			return 0;
		}  
    } 
	
	return -1;
}



/***************************************************
Name:       set_parity
Function:   璁剧疆涓插彛鏁版嵁浣嶏紝鍋滄浣嶅拰鏁堥獙浣?
Input:      1.fd          鎵撳紑鐨勪覆鍙ｆ枃浠跺彞鏌?
      		2.databits    鏁版嵁浣? 鍙栧€? 涓?7  鎴栬€?8
      		3.stopbits    鍋滄浣? 鍙栧€间负 1 鎴栬€?
      		4.parity      鏁堥獙绫诲瀷  鍙栧€间负 N,E,O,S 
Return:     
****************************************************/
static int set_parity(int fd, int databits, int stopbits, int parity) 
{  
	struct termios options;   
	
	if ( tcgetattr( fd,&options) != 0)
	{   
		perror("SetupSerial 1");   
		return(-1);  
    } 
	options.c_cflag &= ~CSIZE; 
	
	/* 璁剧疆鏁版嵁浣嶆暟*/ 
	switch (databits) 
	{  
		case 7:   
			options.c_cflag |= CS7;   
			break; 
			
		case 8:   
			options.c_cflag |= CS8; 
			break;  
			
		default:  
			fprintf(stderr,"Unsupported data sizen");
			return (-1);   
	} 
	
	switch (parity)   
	{  
		case 'n': 
		case 'N':   
			options.c_cflag &= ~PARENB; 				/* Clear parity enable */ 
			options.c_iflag &= ~INPCK;					/* Enable parity checking */   
			break;  
			
		case 'o':   
		case 'O':   
			options.c_cflag |= (PARODD | PARENB); 		
			options.c_iflag |= INPCK;					/* Disnable parity checking */   
			break;  
			
		case 'e':   
		case 'E':   
			options.c_cflag |= PARENB;					/* Enable parity */   
			options.c_cflag &= ~PARODD; 				
			options.c_iflag |= INPCK; 					/* Disnable parity checking */ 
			break; 
			
		case 'S':   
		case 's': 										/*as no parity*/   
			options.c_cflag &= ~PARENB; 
			options.c_cflag &= ~CSTOPB;
			break;   
			
		default:  
			fprintf(stderr,"Unsupported parityn");   
			return (-1);   
	}  
	
    
	switch (stopbits) 
	{  
		case 1:   
			options.c_cflag &= ~CSTOPB;   
			break;  
			
		case 2:   
			options.c_cflag |= CSTOPB;   
			break; 
			
		default:  
			fprintf(stderr,"Unsupported stop bitsn");   
			return (-1);   
	}  
	
	/* Set input parity option */   
	if (parity != 'n')   
		options.c_iflag |= INPCK;   
	
	tcflush(fd,TCIFLUSH); 
	options.c_iflag = 0;
    options.c_oflag = 0;
    options.c_lflag = 0;
	options.c_cc[VTIME] = 150;							// 璁剧疆瓒呮椂 15 seconds
	options.c_cc[VMIN] = 0;								// Update the options and do it NOW
	if (tcsetattr(fd,TCSANOW,&options) != 0)   
	{  
		perror("SetupSerial 3");   
		return (-1);   
	}  
	
	return (0);   
}



int open_dev(char *dev) 
{ 
	int fd = open(dev, O_RDWR );   					
   
	if (-1 == fd)   
	{  
		printf("Can't Open Serial Port:%s\n",dev); 
		return -1;   
	}  
	else  
		return fd; 
} 


int init_uart1(PT_hand_pic ptHandPic)
{
	PT_serial_arg serial_info ;
	serial_info= &ptHandPic->serial_info;
	
	//strcpy(serial_info->send_buf, "this is a Serial_Port2 test!\n");
	serial_info->fd = open_dev("/dev/ttySAC1"); 
	
	set_speed(serial_info->fd, 115200);								//璁剧疆娉㈢壒鐜?
	set_parity(serial_info->fd, 8, 1, 'N'); 							


	//printf("Serial test start.\n");

	fcntl(serial_info->fd,F_SETFL,FNDELAY); 

	printf("Serial test start.\n");
	// 寰幆鍙戦€佹暟鎹?
	return 0;
}








//对串口操作结束


static void display(PT_PixelDatas ptPixelDatas, PT_hand_pic ptHandPic,
					int start_x,int end_x,int start_y, int end_y)
{
		int x,y;

	unsigned int *pwSrc;
	unsigned char  *pdwDest; 
	unsigned char  *red_pix;
	
	pwSrc      =  (unsigned int *)ptPixelDatas->aucPixelDatas;;
	pdwDest    = ptHandPic->green_pix;
	red_pix    = ptHandPic->red_pix;
	
	
	for(y=0;y<ptHandPic->iHeight;y++)
		*(pdwDest+ptHandPic->centre_x+y*ptHandPic->iWidth)=1;
	
	for(x=0;x<ptHandPic->iWidth;x++)
		*(pdwDest+ptHandPic->centre_y*ptHandPic->iWidth+x)=1;
	
	for(x=0;x<ptHandPic->iWidth;x++)
		*(pdwDest+end_y*ptHandPic->iWidth+x)=1;

	for(y=0;y<ptHandPic->iHeight;y++)
		*(pdwDest+end_x+y*ptHandPic->iWidth)=1;

	for (y = 0; y < ptHandPic->iHeight; y++){
 		for (x = 0; x < ptHandPic->iWidth; x++){
			
			if((y>=start_y)&&(y<=end_y)&&(x>=start_x)&&(x<=end_x)){
				if(*pdwDest==0)
					*pwSrc=0x00ff00;
				if(*red_pix==ptHandPic->red_max)
					*pwSrc=0xff0000;
			}
			pwSrc++;
			pdwDest++;	
			red_pix++;
			
			
            }
        }


	

//	return 0;



}


static void calculate_grade(PT_hand_pic ptHandPic , int start_x, int end_x,
									       int start_y, int end_y)
{
	double interval;
	static unsigned int miss_count;
	double laser_x, laser_y;
	double distance;
	int move_x;
	int move_y;
	char move_up,move_down,move_left,move_right;
	
 	laser_x=ptHandPic->red_x -ptHandPic->centre_x;
	laser_y=ptHandPic->red_y -ptHandPic->centre_y;
	
//	printf("x%lf y%lf \n",laser_x,laser_y);
	distance= sqrt( laser_x*laser_x+ laser_y*laser_y)+0.5;
	// printf("x%lf \n",distance);
	
	interval=( (double)(end_y-start_y+end_x-start_x)/2)/(double)51;
	// printf("x%lf \n",interval);
	distance=distance/interval;
	//distance=distance/(double)( (double)(end_y-start_y+end_x-start_x)/2)*51;
	
//	interval=5.1;
//	printf("aaaa %f \n",distance);
	 switch ((int )(distance/5.1)){
        case 0:ptHandPic->grade=RING10;break;
        case 1:ptHandPic->grade=RING9;break;
        case 2:ptHandPic->grade=RING8;break;
        case 3:
			if(ptHandPic->laser_scope==WHITE){
				ptHandPic->grade=MISS;
				break;
			}else			
				ptHandPic->grade=RING7;
			break;
        case 4:
			if(ptHandPic->laser_scope==WHITE){
				ptHandPic->grade=MISS;
				break;
			}else			
				ptHandPic->grade=RING6;
			break;
        case 5:
			if(ptHandPic->laser_scope==WHITE){
				ptHandPic->grade=MISS;
				break;
			}else			
				ptHandPic->grade=RING5;
			break;
        default:
			ptHandPic->grade=MISS;
			break;
    	}
		
	
//	memset(ptHandPic->serial_info.send_buf, ptHandPic->grade, 1);
//	memset(ptHandPic->serial_info.send_buf+1, (distance/interval), 1);
	
	if(ptHandPic->serial_info.send_flag==CONTRAL){//计算发送电机转动
		
			 
			//ptHandPic->serial_info.target_grade
			if((ptHandPic->serial_info.target_grade != ptHandPic->grade)&&
				((ptHandPic->serial_info.wait_flag ==0)||(miss_count>16))){
				miss_count =0;
				laser_y=5.1*interval*(double)(ptHandPic->serial_info.target_grade-10);
				laser_x=0;
				
				move_y = laser_y-(ptHandPic->red_y -ptHandPic->centre_y);
				move_x = laser_x-(ptHandPic->red_x -ptHandPic->centre_x);
				//printf("ly%f,mx%d,my%d\n",laser_y,move_x,move_y);
				if(move_y<0){
					move_up     = 1;
					move_down = 0;
				}else{
					move_up     = 0;
					move_down = 1;
				}
				if(move_x>0){
					move_right  = 1;
					move_left    = 0;
				}else{
					move_right  = 0;
					move_left    = 1;
				}
				
				ptHandPic->serial_info.send_count++;
				
				ptHandPic->serial_info.send_buf[0] = 0xfe;
				ptHandPic->serial_info.send_buf[1] = move_up;
				ptHandPic->serial_info.send_buf[2] = move_down;
				ptHandPic->serial_info.send_buf[3] = move_left;
				ptHandPic->serial_info.send_buf[4] = move_right;
				ptHandPic->serial_info.send_buf[5] = ptHandPic->serial_info.send_count;
				ptHandPic->serial_info.send_buf[6] = 0xef;
				ptHandPic->serial_info.wait_flag     =1;
				
				write(ptHandPic->serial_info.fd, ptHandPic->serial_info.send_buf, 7);
				printf("send motor info %d %d %d %d %d\n",move_up,move_down,move_left,move_right,ptHandPic->serial_info.send_count);
				return ;
				

			}else{
				miss_count++;
				ptHandPic->serial_info.send_buf[0] = 0xfe;
        		ptHandPic->serial_info.send_buf[1] = ptHandPic->grade;
        		ptHandPic->serial_info.send_buf[2] = 0x3;
        		ptHandPic->serial_info.send_buf[3] = 0x3;
        		ptHandPic->serial_info.send_buf[4] = 0x3;
        		ptHandPic->serial_info.send_buf[5] = 0x3;
        		ptHandPic->serial_info.send_buf[6] = 0xef;
    			//  ptHandPic->serial_info.send_buf[7] = '\n';

    
				if(ptHandPic->same_grade_count > 33){
	
					if(ptHandPic->grade==ptHandPic->serial_info.target_grade)
        				write(ptHandPic->serial_info.fd, ptHandPic->serial_info.send_buf, 7);
					printf("ndPic->serial_info.send_buf %d \n",ptHandPic->grade);

    //    		ptHandPic->last_grade= ptHandPic->serial_info.send_buf[1] ;
        		ptHandPic->same_grade_count=0;
    			}else{
        			ptHandPic->same_grade_count++;
   				}		

				
				return;
			}
			
		
       
	}else{
		
		ptHandPic->serial_info.send_buf[0] = 0xfe;
		ptHandPic->serial_info.send_buf[1] = ptHandPic->grade;
		ptHandPic->serial_info.send_buf[2] = 0x3;
		ptHandPic->serial_info.send_buf[3] = 0x3;
		ptHandPic->serial_info.send_buf[4] = 0x3;
		ptHandPic->serial_info.send_buf[5] = 0x3;
		ptHandPic->serial_info.send_buf[6] = 0xef;
	//	ptHandPic->serial_info.send_buf[7] = '\n';

	}
	
	if((ptHandPic->last_grade != ptHandPic->serial_info.send_buf[1])
            ||(ptHandPic->same_grade_count > 33)){

        write(ptHandPic->serial_info.fd, ptHandPic->serial_info.send_buf, 7);
		
		
        ptHandPic->last_grade= ptHandPic->serial_info.send_buf[1] ;
        ptHandPic->same_grade_count=0;
    }
    else{
        ptHandPic->same_grade_count++;
    }

				
    //write(ptHandPic->serial_info.fd, ptHandPic->serial_info.send_buf, 7);
			
		//printf("%lf grade %d \n",distance,ptHandPic->grade);

}



static int find_coordinate(PT_hand_pic ptHandPic,int start_x, int end_x,
										int start_y, int end_y)
{
	int centrel1,centrel2,centrel3;
	int tmp;
	int centrel;
	int centrel_low;
	int centrel_hight;
	int x,y;
	int count=0;
	int max;
	unsigned char *green_pix;
	green_pix      =   ptHandPic->green_pix;

	centrel1=find_coordinate_x(ptHandPic, start_x,end_x,0,7);
	centrel2=find_coordinate_x(ptHandPic, start_x,end_x,20,7);
	centrel3=find_coordinate_x(ptHandPic, start_x,end_x,40,7);

	//有小到大排序
	if(centrel1>centrel2){tmp=centrel1;centrel1=centrel2;centrel2=tmp;}
 	if(centrel1>centrel3){tmp=centrel1;centrel1=centrel3;centrel3=tmp;}
	if(centrel2>centrel3){tmp=centrel2;centrel2=centrel3;centrel3=tmp;}

	//if((centrel2-centrel1)>(centrel3-centrel2))
	//	centrel=(centrel3+centrel2)/2+0.5;
	//else
	//	centrel=(centrel1+centrel2)/2+0.5;
	if((centrel2-centrel1)>(centrel3-centrel2))
		centrel_hight=(centrel3+centrel2)/2+0.5;
	else
		centrel_hight=(centrel1+centrel2)/2+0.5;
	

       centrel1=find_coordinate_x(ptHandPic, start_x,end_x,end_y-10,7);
	centrel2=find_coordinate_x(ptHandPic, start_x,end_x,end_y-20,7);
	centrel3=find_coordinate_x(ptHandPic, start_x,end_x,end_y-30,7);
	
      if(centrel1>centrel2){tmp=centrel1;centrel1=centrel2;centrel2=tmp;}
 	if(centrel1>centrel3){tmp=centrel1;centrel1=centrel3;centrel3=tmp;}
	if(centrel2>centrel3){tmp=centrel2;centrel2=centrel3;centrel3=tmp;}

	if((centrel2-centrel1)>(centrel3-centrel2))
		centrel_low=(centrel3+centrel2)/2+0.5;
	else
		centrel_low=(centrel1+centrel2)/2+0.5;

	//printf();
	 centrel =(centrel_low+centrel_low)/2;
	
	//printf("       %d %d %d  \n",centrel_low,centrel_hight,centrel);
	ptHandPic->centre_x=centrel;
	for(y=end_y-1;y>=start_y;y--){
		for(x=start_x;x<end_x;x++){
			if(*(green_pix+x+y*ptHandPic->iWidth) == 0)
				 count++;
		}
		if(count>(double)(end_x-start_x)*0.9){
			max=y;
			break;

		}
			
	}
		
				
	ptHandPic->centre_y=(double)(max-start_y) * 0.604+0.5+start_y;

	//printf("%d %d  \n",ptHandPic->centre_y,ptHandPic->centre_x);
	return 0;
	

	

}


static int find_coordinate_x(PT_hand_pic ptHandPic,int start_x, 
					int end_x, int start_y, int interval)
{
	int x,y;
	int max;
	int max_count;
	int centrel;
	int left_most=0;
	unsigned char *green_pix;
	int count[320] ={0};
	int black_point;
	green_pix      =   ptHandPic->green_pix;
	max=0;
	for(x=start_x; x<end_x; x++){
		black_point=0;
		for(y=start_y;y<start_y+interval;y++){
			if(*(green_pix+x+y*ptHandPic->iWidth) == 0)
				black_point++;
		}
	
		count[x]=black_point;
		if(max<black_point){
			max=black_point;
			left_most=x;
		}
			
	}

	max_count=0;
	for(x=start_x; x<end_x; x++){
		if(count[x]==max)
			max_count++;
	}

	centrel=max_count/2+left_most;
	return centrel;
	

}





static int check_laser(PT_hand_pic ptHandPic,int start_x, 
					int end_x, int start_y, int end_y)
{
	
	int x,y;
	int start,end;
	unsigned char * green_pix;
	green_pix  = ptHandPic->green_pix;

	y     = ptHandPic->red_y;
	start = ptHandPic->red_x-5;
	end  = ptHandPic->red_x+5;
	if(start < start_x)
		start = start_x;
	if(end  > end_x)
		end  = end_x;
	
	ptHandPic->laser_scope=WHITE;
	for(x=start;x<end;x++){
		if(*(green_pix+x+y*ptHandPic->iWidth) == 0){
			ptHandPic->laser_scope=BLACK;
			break;

		}
			
	}
	return 0;

}


static int find_bound(PT_hand_pic ptHandPic,int  *start_x, int *end_x ,
						int *start_y, int *end_y  )
{
	int bound_x1,bound_x2;
	int bound_y1,bound_y2;
	int tmp;
	bound_y1=bound_y2=0;

	bound_x1=find_bound_x(ptHandPic,0,ptHandPic->iWidth,
                            0, ptHandPic->iHeight);

    if(bound_x1 !=0){
        if(bound_x1 > (ptHandPic->iWidth/2)){
            bound_x2=find_bound_x(ptHandPic,0,bound_x1-1,
                            0, ptHandPic->iHeight);
            tmp         =   bound_x2;
            bound_x2    =   bound_x1-1;
            bound_x1    =   tmp+1;

        }else{
            bound_x2=find_bound_x(ptHandPic, bound_x1+1, 0,
                            0, ptHandPic->iHeight);
            if(bound_x2==0)
                bound_x2=ptHandPic->iWidth;
            else{
                bound_x1 +=1;
                bound_x2 +=1;
            }

        }
    }else{
        bound_x1    =   0;
        bound_x2    =   320;
    }
	
	bound_y1=find_bound_y(ptHandPic,bound_x1,bound_x2,
							0,ptHandPic->iHeight);
	
	if(bound_y1 !=0){
        if(bound_y1 > (ptHandPic->iHeight/2)){
            bound_y2=find_bound_y(ptHandPic,bound_x1,bound_x2,
                            0, bound_y1-1);
            tmp         =   bound_y2;
            bound_y2    =   bound_y1-1;
            bound_y1    =   tmp+1;

        }else{
            bound_y2=find_bound_y(ptHandPic, bound_x1,bound_x2,
                            	bound_y1+1, ptHandPic->iHeight);
            if(bound_y2==0)
                bound_y2=ptHandPic->iHeight;
            else{
                bound_y1 +=1;
                bound_y2 +=1;
            }

        }
    }else{
        bound_y1    =   0;
        bound_y2    =   240;
    }
	
	*start_x = bound_x1;
	*end_x   = bound_x2;
	*start_y = bound_y1;
	*end_y  = bound_y2;
	
	if((bound_x2-bound_x1)>300)//不是靶图
		return -1;
		
   //printf("start_x %d %d %d %d \n",bound_x1,bound_x2,bound_y1,bound_y2);
   return 0;

}




static int  find_bound_y(PT_hand_pic ptHandPic,int  start_x, int end_x ,
						int start_y, int end_y  )
{

	int x,y;
	int count;
	int bound=0;
	unsigned char *greed_pix=ptHandPic->green_pix;
	unsigned int  middle=ptHandPic->iHeight/2;

	for(y=start_y; y < end_y; y++){
		count = 0;
		for(x=start_x; x<=end_x; x++){
			if(*(greed_pix+y*ptHandPic->iWidth + x)>0)
				count++;
		}
		
		if(( ( (y<middle )&& (count > (end_x-start_x-5) )) ||
			((y>middle )&& (count > (end_x-start_x-30) )) )
			&& (abs(bound-middle)>abs(y-middle) ))
			bound=y;
		
	}
	return  bound;


}






static int  find_bound_x(PT_hand_pic ptHandPic,int  start_x, int end_x ,
						int start_y, int end_y  )
{

	int x,y;
	int count;
	int bound=0;
	unsigned char *greed_pix=ptHandPic->green_pix;
	unsigned int  middle=ptHandPic->iWidth/2;

	for(x=start_x; x < end_x; x++){
		count = 0;
		for(y=start_y; y<=end_y; y++){
			if(*(greed_pix + y * ptHandPic->iWidth + x) > 0)
				count++;
		}
		
		if( (count>(end_y-start_y-20)) && (abs(bound-middle)>abs(x-middle) ))
			bound=x;
		
	}
	return  bound;


}






static  int  otsu_method(PT_hand_pic ptHandPic)
{
	unsigned int max,min;
	int x,y;
	unsigned int   color;
    unsigned char  *greed_pix=ptHandPic->green_pix;
	unsigned int   *count;
	double         probability[330]={0};
	double         sum_probability;
	max=min=*greed_pix;

	for (y = 0; y < ptHandPic->iHeight; y++){//get mix & min
 		for (x = 0; x < ptHandPic->iWidth; x++){
			color = *(greed_pix+x+y * ptHandPic->iWidth);
			if(color > max)
				max = color;
			if(color < min)
				min = color;
			
        	}
 	}
	
	count=(unsigned int *)calloc(max-min+1,sizeof(int));

	for (y = 0; y < ptHandPic->iHeight; y++){
 		for (x = 0; x < ptHandPic->iWidth; x++){
	
			color = *(greed_pix+x+y* ptHandPic->iWidth);
			*(count+color-min) +=1;
				
        	}
 	}

	/*(if(((max-min)>250)||((max-min)<210)){
		free(count);
		return -1;
	}	
	*/
			
//	printf("min%d  max :%d \n" ,min,max );
//	for(x=0;x<max-min+1;x++)
//		printf("%d data :%d \n" ,x,count[x] );

	y= ptHandPic->iWidth * ptHandPic->iHeight;
	for (x = 0; x <max-min+1; x++){
		*(probability+x) = (double)*(count+x)*100/(double)y;
    }

	sum_probability=0;
	//for (x = 0; x <max-min+1; x++){
	//	sum_probability += *(probability+x) * (double)(*(count+x));
    //}
	for (x = min,y=0; x <max+1; x++,y++){
		//printf("myyyyyyy :%d \n" ,y );
		sum_probability += *(probability+y) *(double)x;
			
    }
	
	sum_probability = sum_probability/100;
//	printf("hhhh%f",sum_probability);
	if(ptHandPic->iBpp==32)
		if((sum_probability<60)||(sum_probability>190)){
	 		free(count);
			return -1;
		}

		

	//printf("here sum_probability %f\n" ,sum_probability);
	
	//binaryzation
	for (y = 0; y < ptHandPic->iHeight; y++){
 		for (x = 0; x < ptHandPic->iWidth; x++){
			if (*(greed_pix+x+y* ptHandPic->iWidth) >= sum_probability )
				*(greed_pix+x+y* ptHandPic->iWidth)=1;
			else
				*(greed_pix+x+y* ptHandPic->iWidth)=0;
			
           
        }
   	}

	//printf("here 1\n" );


	free(count);

	return 0;

}




static int  find_laser(PT_hand_pic ptHandPic)
{
	int x,y;
	unsigned char  *red_pix;
	unsigned int sum_x;
	unsigned int sum_y;
	unsigned int x_max=0;
	unsigned int x_min=500;
	int               count=0;
	static int   miss_count;
	sum_x=0;
	sum_y=0;
	if((ptHandPic->iBpp==16)&&(ptHandPic->red_max<28))
		goto miss;
			//return  -1;
	if((ptHandPic->iBpp==32)&&(ptHandPic->red_max<220))
		goto miss;
				//return -1;
	//printf("red max:%d",ptHandPic->red_max);
	red_pix=ptHandPic->red_pix;
	for (y = 0; y < ptHandPic->iHeight; y++){
 		for (x = 0; x < ptHandPic->iWidth; x++){
			if (*(red_pix+x+y* ptHandPic->iWidth)==ptHandPic->red_max){
				sum_x +=x;
				sum_y +=y;
				//printf("x%d y%d\n",x,y);
				count++;
				if(x>x_max)
					x_max=x;
				if(x<x_min)
					x_min=x;
			}	
            }
        }
	//printf("count %d %d\n",count,x_max-x_min);
	if((count>400)||(count<4))
		goto miss;
	
	if((x_max-x_min)>50)
		goto miss;;
	
	ptHandPic->red_x  =  sum_x/count;
	ptHandPic->red_y  =   sum_y/count;
	
	ptHandPic->serial_info.send_buf[4]=ptHandPic->red_max;
	ptHandPic->serial_info.send_buf[5]=ptHandPic->red_x;
	ptHandPic->serial_info.send_buf[6]=ptHandPic->red_y;
	//printf("%drx_min%d x_max%d\n",x_max-x_min,x_max,x_min );
	return 0;

miss:
	
    ptHandPic->serial_info.send_buf[0] = 0xfe;
    ptHandPic->serial_info.send_buf[1] = 0; //脱靶
    ptHandPic->serial_info.send_buf[2] = 0x3;
    ptHandPic->serial_info.send_buf[3] = 0x3;
    ptHandPic->serial_info.send_buf[4] = 0x3;
    ptHandPic->serial_info.send_buf[5] = 0x3;
    ptHandPic->serial_info.send_buf[6] = 0xef;
	
	if(ptHandPic->last_grade != ptHandPic->serial_info.send_buf[1]){
//		if(miss_count>10)
			miss_count++;
		if(miss_count<10){
			return -1;
		}
		
	}

	miss_count=0;

	if((ptHandPic->last_grade != ptHandPic->serial_info.send_buf[1])
			||(ptHandPic->same_grade_count > 33)){
		
		write(ptHandPic->serial_info.fd, ptHandPic->serial_info.send_buf, 7);
		ptHandPic->last_grade= ptHandPic->serial_info.send_buf[1] ;
		ptHandPic->same_grade_count=0;
	}
	else{
		ptHandPic->same_grade_count++;
	}

	return -1; 

}



int test (PT_hand_pic ptHandPic)
{
	int x,y;
	unsigned int  color,red;
	unsigned short *pwSrc;
	unsigned char  *pdwDest; 
	unsigned char  *red_pix;
	

	ptHandPic->iWidth        = 320;
	ptHandPic->iHeight       = 240;
	ptHandPic->iTotalBytes 	 = 240*320;
	ptHandPic->iBpp          = 16;
	if(ptHandPic->iTotalBytes !=320*240)
		return -1;
	
	pwSrc      = (unsigned short *)pic_data;
	pdwDest=ptHandPic->green_pix;
	red_pix=ptHandPic->red_pix;
	ptHandPic->red_max=0;
	for (y = 0; y < ptHandPic->iHeight; y++){
 		for (x = 0; x < ptHandPic->iWidth; x++){
			
			color = *pwSrc++;
        
			red=color >> 11;
			if(red > ptHandPic->red_max )
					ptHandPic->red_max=red;
			
            *pdwDest = (color >> 5) & (0x3f);
	
			*red_pix  =red;
	//		printf("test green i:%x, color:%x, pdwDest:%x \n ",color,*red_pix,*pdwDest);
			pdwDest++;
			red_pix++;
            }
        }
	return 0;

}












static int get_info(PT_PixelDatas ptPixelDatas, PT_hand_pic ptHandPic)
{	
	int x,y;
	unsigned int  color,red;
	unsigned int *pwSrc      = (unsigned int *)ptPixelDatas->aucPixelDatas;
	unsigned char  *pdwDest; 
	unsigned char  *red_pix  =ptHandPic->red_pix;
	
	ptHandPic->iWidth        =  ptPixelDatas->iWidth;
	ptHandPic->iHeight       =  ptPixelDatas->iHeight;
	ptHandPic->iBpp          =  ptPixelDatas->iBpp;
	
	
	if(ptPixelDatas->iTotalBytes !=320*240*4)
		return -1;
	
	pdwDest=ptHandPic->green_pix;
	red_pix=ptHandPic->red_pix;
	ptHandPic->red_max=0;

	for (y = 0; y < ptHandPic->iHeight; y++){
 		for (x = 0; x < ptHandPic->iWidth; x++){
			
			color = *pwSrc++;   
			red=color>>16&0xff;
			if(red > ptHandPic->red_max )
				ptHandPic->red_max=red;

	        *pdwDest = (color>>8&0xff);
			*red_pix  =red;
			//printf("%x %x %x \n",color,*red_pix,*pdwDest );
			pdwDest++;
			red_pix++;
            }
        }

	
	return 0;

}

