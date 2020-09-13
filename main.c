#include<pspkernel.h>
#include<pspctrl.h>
#include<pspdebug.h>
#include<pspdisplay.h>
#include<dirent.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "callback.h"

#define	VERS	1
#define	REVS	0
#define printf	pspDebugScreenPrintf
#define RGB(r,g,b)	((r)|((g)<<8)|((b)<<16))

#define MAX_FIND_NUMOF_FILES 64
#define MAX_PRINT_NUMOF_FILES 28
#define MAX_FILENAME_SIZE 256
#define MAX_FILEPATH_SIZE 512
#define MAX_BUFFER_SIZE 256

PSP_MODULE_INFO("binviewer",0x1000,1,1);
PSP_MAIN_THREAD_ATTR(0);

static char *root_path = "ms0:/";
static char current_path[MAX_FILEPATH_SIZE];
static char filename_list[MAX_FIND_NUMOF_FILES][MAX_FILENAME_SIZE];
static char dirname_list[MAX_FIND_NUMOF_FILES][MAX_FILENAME_SIZE];
static char read_buffer[MAX_BUFFER_SIZE];

static int cnt_dirs = 0;
static int cnt_files = 0;
static int cnt_all = 0;
static int filenmame_point = 0;


void listdir(const char* path){
	DIR *dir;
	struct dirent* en;
	SceIoStat d_stat;
	int i;

	cnt_files = 0;
	cnt_dirs = 0;
	cnt_all = 0;

	for(i=0;i<MAX_FIND_NUMOF_FILES;i++){
		memset(filename_list[i],0,MAX_FILENAME_SIZE);
		memset(dirname_list[i],0,MAX_FILENAME_SIZE);
	}
	
	dir = opendir(path);
	if(!dir){
		pspDebugScreenClear();
		printf("failed opendir ...\n");
		return;
	}

	for(i=0;i<MAX_FIND_NUMOF_FILES;i++){
		en = readdir(dir);

		if(!en)
			break;

		d_stat = en->d_stat;

		if(d_stat.st_attr & FIO_SO_IFDIR){
			strncpy(dirname_list[cnt_dirs],en->d_name,MAX_FILENAME_SIZE);
			cnt_dirs++;
		}

		if(d_stat.st_attr & FIO_SO_IFREG){
			strncpy(filename_list[cnt_files],en->d_name,MAX_FILENAME_SIZE);
			cnt_files++;
		}
	}
	
	cnt_all = cnt_files + cnt_dirs;
}


void printheader(void){
	printf("Simple Binary Viewer ver 1.0.0\n\n");
}


void printfilelist(int scroll_index){
	int i;

	pspDebugScreenSetTextColor(RGB(255,255,255));

	for(i=scroll_index;i<scroll_index+MAX_PRINT_NUMOF_FILES;i++){
		if(i == filenmame_point)
			pspDebugScreenSetTextColor(RGB(255,0,0));
		else
			pspDebugScreenSetTextColor(RGB(255,255,255));

		if(i < cnt_dirs)
			printf("%s\n",dirname_list[i]);
		else
			printf("%s\n",filename_list[i-cnt_dirs]);
	}

	pspDebugScreenSetTextColor(RGB(255,255,255));
}


void _printfiledetails(void){
	int i;
	
	for(i=0;i<MAX_BUFFER_SIZE;i++){
		unsigned char hex = read_buffer[i];
		if(i % 16 == 0)
			printf("\n");
		printf("%02X",hex);
		printf(" ");
	}

}
	

void printfiledetails(void){
	SceUID fdin;
	int bytes_read;
	char open_path[MAX_FILEPATH_SIZE];

	memset(open_path,0,MAX_FILEPATH_SIZE);
	strncpy(open_path,current_path,strlen(current_path));

	if(strcmp(open_path,"ms0:/") != 0)
		strncat(open_path,"/",sizeof(char));
	strncat(open_path,filename_list[filenmame_point-cnt_dirs],strlen(filename_list[filenmame_point-cnt_dirs]));
	
	pspDebugScreenSetTextColor(RGB(255,255,255));
	printf("filename: %s\n",filename_list[filenmame_point-cnt_dirs]);
	
	fdin = sceIoOpen(open_path,PSP_O_RDONLY,0777);

	if(fdin > 0){
		// SceOff filesize = sceIoLseek32(fdin,0,SEEK_END);
		// sceIoLseek32(fdin,0,SEEK_SET);

		/*
		while(1){
			memset(read_buffer,0,MAX_BUFFER_SIZE);
			bytes_read = sceIoRead(fdin,read_buffer,sizeof(read_buffer));
			if(bytes_read > 0){
				_printfiledetails();
			}else{
				break;
			}
		}
		*/
	
		printf("open sucess: fds is %d\n",fdin);
		memset(read_buffer,0,MAX_BUFFER_SIZE);
		bytes_read = sceIoRead(fdin,read_buffer,sizeof(read_buffer));

		if(bytes_read > 0)
			_printfiledetails();

		sceIoClose(fdin);

	}else{
		printf("open failed fds is %d\n",fdin);
		return;
	}
}


int main(int argc,char *argv[]){
	SceCtrlData pad_data;
	unsigned int current_status;
	int scroll_index = 0;

	pspDebugScreenInit();
	setupExitCallback();
	
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(1);

	memset(current_path,0,MAX_FILEPATH_SIZE);
	strncpy(current_path,root_path,strlen(root_path));

	listdir(current_path);
	printheader();
	printfilelist(scroll_index);

	while(isRunning()){
		sceCtrlReadBufferPositive(&pad_data,1);
		current_status = pad_data.Buttons;

		if(current_status & PSP_CTRL_UP){
			pspDebugScreenClear();
			printheader();		

			if(filenmame_point > 0)
				filenmame_point--;
	
			if(cnt_all > MAX_PRINT_NUMOF_FILES && scroll_index > 0)
				scroll_index--;
		
			printfilelist(scroll_index);
		}

		if(current_status & PSP_CTRL_DOWN){
			pspDebugScreenClear();
			printheader();
			
			if(filenmame_point < cnt_all-1)
				filenmame_point++;
			
			if(cnt_all > MAX_PRINT_NUMOF_FILES && scroll_index < cnt_all-1)
				scroll_index++;

			printfilelist(scroll_index);
		}

		if(current_status & PSP_CTRL_RIGHT){
			pspDebugScreenClear();
			printheader();

			if(strlen(current_path) + strlen(dirname_list[filenmame_point]) < MAX_FILEPATH_SIZE){
				if(strcmp(current_path,"ms0:/") != 0 && filenmame_point < cnt_dirs)
					strncat(current_path,"/",sizeof(char));
			}else{
				memset(current_path,0,MAX_FILENAME_SIZE);
				strncpy(current_path,root_path,MAX_FILENAME_SIZE);
				continue;
			}

			if(filenmame_point < cnt_dirs){
				// printf("next path: %s\n",current_path);
				strncat(current_path,dirname_list[filenmame_point],strlen(dirname_list[filenmame_point]));
				filenmame_point = 0;
				scroll_index = 0;
				listdir(current_path);
				printfilelist(scroll_index);
			}else{
				printfiledetails();
			}
		}

		/* right keys */
		// PSP_CTRL_TRIANGLE
		// PSP_CTRL_CIRCLE
		// PSP_CTRL_CROSS
		// PSP_CTRL_SQUARE
	
		/* left keys */
		// PSP_CTRL_UP
		// PSP_CTRL_DOWN
		// PSP_CTRL_RIGHT
		// PSP_CTRL_LEFT

		/* back keys */
		// PSP_CTRL_LTRIGGER
		// PSP_CTRL_RTRIGGER

		sceDisplayWaitVblankStart();
	}
	
	sceKernelExitGame();
	return 0;
}
