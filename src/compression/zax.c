// modified by Luigi Auriemma
// http://code.google.com/p/zax/

#include <stdlib.h>
#include <string.h>

int zax_uncompress(unsigned char *infd, int insz, unsigned char *outfd, int outsz) {

#define ZAX_WINSIZE 1024
#define ZAX_CODE_NUM 1024
#define ZAX_DECODE_BUF_SIZE 102400
#define ZAX_CODE_LENGTH 3
#define ZAX_CODE_BUF_SIZE  ZAX_CODE_LENGTH*ZAX_CODE_NUM
#define ZAX_MAX_80_NUM_DECODE 1023
#define ZAX_MAX_80_CODE 342

    unsigned char *o = outfd;
	int decode_counter=0,/*read_counter=0,*/code_remain=0;
	int i,code_type,offset,length,length80,/*next_char,*/repeat_times,code_num=0;
	unsigned char (* raw_code_p)[3];
	unsigned char *switch_read_buf;
 	
	//cause CODE may change ZAX_CODE_LENGTH calculate here
//	ZAX_CODE_LENGTH = sizeof(CODE);
//	ZAX_CODE_BUF_SIZE = ZAX_CODE_LENGTH*ZAX_CODE_NUM;
	
	unsigned char *file_read_buf1 = (unsigned char *)calloc(1, ZAX_CODE_BUF_SIZE);
	unsigned char *file_read_buf2 = (unsigned char *)calloc(1, ZAX_CODE_BUF_SIZE);
	unsigned char *decode_buf = (unsigned char *)calloc(1, ZAX_DECODE_BUF_SIZE);
	raw_code_p = (void *)file_read_buf1;
	unsigned char *window_back = decode_buf - 1;//initial the window_back ptr 
	
	//while((code_num = fread(file_read_buf2 + code_remain *3 ,ZAX_CODE_LENGTH,ZAX_CODE_NUM - code_remain,infd) + code_remain) > 0){
    while(insz > 0) {
        code_num = (ZAX_CODE_LENGTH) * (ZAX_CODE_NUM - code_remain);
        if(code_num <= 0) break;
        if(code_num > insz) code_num = insz;
        memcpy(
            file_read_buf2 + code_remain *3 ,
            infd,
            code_num);
        infd += code_num;
        insz -= code_num;
        code_num += code_remain;
        if(code_num <= 0) break;

		//printf("%d\n",*(raw_code_p+1)[0]);
		
		if (code_remain != 0){
			memcpy(file_read_buf2,raw_code_p,code_remain * 3);
		}
		//for efficiency here i switch the pointer of buf instead of memcpy the 
		//larger block of memory
		switch_read_buf = file_read_buf1;
		file_read_buf1 = file_read_buf2;
		file_read_buf2 = switch_read_buf;
		 
		raw_code_p = (void *)file_read_buf1;
		
		for(decode_counter=1;decode_counter <= code_num;decode_counter++){
		
			if((*raw_code_p)[0]&128){//code first bit is 1
				code_type = 1;
				i = repeat_times = ((*raw_code_p)[0] & 127)+1;
			}
			else{//code first bit is 0
				if ((length = (*raw_code_p)[1] & 31) || ((*raw_code_p)[0] == 0 && (*raw_code_p)[1] == 0)){//not the continuous "80" code
					code_type = 0;
					offset = (((*raw_code_p)[0] & 127)<<3) + (((*raw_code_p)[1] & 224) >> 5);
					
				}
				else{//continuous "80" or "backspace" code
					code_type = 2;
					length80 = (((*raw_code_p)[0] & 127)<<3) + (((*raw_code_p)[1] & 224) >> 5);
				}
			}
			
			//debug!!
			//code_type == 0 && offset == 0 )
				//(*(raw_code_p))[0] == 0 
		//	if ((*(raw_code_p+3))[0] == 10 && (*(raw_code_p+3))[1] == 82 && (*(raw_code_p+3))[2] == 203){
		//		printf("000000000 %d \n",code_type);
		//	}
			//debug!! 
			
			//process the rawcode
			switch (code_type){
			case 0: //code type 0
				//length != 0 ;length == 0 means it's continuous 80
			//	printf("0");
				memcpy(window_back+1,window_back - ZAX_WINSIZE + offset +1,length);
				window_back += length;
				
				*(++window_back) = (*raw_code_p)[2];
				
				raw_code_p++;
				break;
				
			case 1: //code type 1
			//	printf("1");
				for(;i > 0;i--){
					*(++window_back) =  (*raw_code_p)[1];
					*(++window_back) =  (*raw_code_p)[2];
					
				}
				raw_code_p++;
				break;	
		
			
			case 2://code type 2 continuous"80" code
			//	printf("2");
				if (length80 == 1){//an extra code in the end
						window_back--;
						raw_code_p++;
				}
				else{
					memcpy(window_back+1,(char *)(raw_code_p + 1),length80);
					raw_code_p += length80 % 3 ?(length80 /3 + 2):(length80 /3 + 1);
					window_back += length80;
					decode_counter += length80 % 3 ?(length80 /3 + 1):(length80 /3);
					
				}
				break;
			}	
			if ((decode_buf + ZAX_DECODE_BUF_SIZE - window_back -1 <  ZAX_MAX_80_NUM_DECODE + 1)){
				//fwrite(decode_buf,1,window_back-ZAX_WINSIZE-decode_buf+1,outfd);//write and reset decode_buf
                i = window_back-ZAX_WINSIZE-decode_buf+1;
                if(i > outsz) i = outsz;
                if(i > 0) { memcpy(o, decode_buf, i); o += i; outsz -= i; }

				memcpy(decode_buf,window_back-ZAX_WINSIZE+1,ZAX_WINSIZE);
				window_back = decode_buf+ZAX_WINSIZE-1;
			}
		
		
			if ((code_remain = code_num - decode_counter) < ZAX_MAX_80_CODE  && code_num == ZAX_CODE_NUM)
				break;
		}
	}
	//fwrite(decode_buf,1,window_back-decode_buf+1,outfd);//write the rest code
    i = window_back-decode_buf+1;
    if(i > outsz) i = outsz;
    if(i > 0) { memcpy(o, decode_buf, i); o += i; outsz -= i; }

    free(file_read_buf1);
	free(file_read_buf2);
	free(decode_buf);
	return (o - outfd);
}
