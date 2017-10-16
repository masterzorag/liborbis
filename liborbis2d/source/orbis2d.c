#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <kernel.h>

#include <gnmdriver.h>
#include <systemservice.h>
#include <videoout.h>
#include <types/userservice.h>

#include "orbis2d.h"
#include "logdebug.h"
#include "font.h"

Orbis2dConfig *orbconf = NULL;
int orbis2d_external_conf = -1;
int64_t flipArgCounter = 0;
uint32_t font_color = 0x80000000;
uint32_t backfont_color = 0x80FFFFFF;

uint32_t orbis2dGetRGB(int r, int g, int b) {
	r=r%256;
	g=g%256;
	b=b%256;
	return 0x80000000|r<<16|g<<8|b;
}

void orbis2dFinish()
{
	int ret;
	if(orbis2d_external_conf != 1)
	{
		if(orbconf->orbis2d_initialized == 1)
		{
			ret = sceVideoOutClose(orbconf->videoHandle);

			sys_log("liborbis2d sceVideoOutClose return 0x%8x\n",ret);
		}
		orbconf->orbis2d_initialized=-1;
		sys_log("liborbis2d finished");
	}
}

Orbis2dConfig *orbis2dGetConf()
{
	if(orbconf)
	{
		return orbconf;
	}
	
	return NULL; 
}
int orbis2dCreateConf()
{	
	if(!orbconf)
	{
		orbconf=(Orbis2dConfig *)malloc(sizeof(Orbis2dConfig));
		
		orbconf->bgColor=0x80ffffff; //white
		orbconf->videoMemStackAddr=0;
		orbconf->videoMemStackSize=1024*1024* 192;
		orbconf->videoMemStackTopAddr=0;
		orbconf->videoMemStackBaseAddr=0;
		orbconf->videoMemOffset=0;
		orbconf->width=ATTR_WIDTH;
		orbconf->pitch=ATTR_WIDTH;
		orbconf->height=ATTR_HEIGHT;
		orbconf->pixelFormat=0x80000000;
		orbconf->bytesPerPixel=4;
		orbconf->tilingMode=ORBIS2D_MODE_LINEAR;
		orbconf->flipMode=ORBIS2D_FLIP_MODE_VSYNC;
		orbconf->flipRate=ORBIS2D_FLIP_RATE;
		orbconf->videoHandle=-1;
		orbconf->currentBuffer=0;
		orbconf->orbis2d_initialized=-1;
		
		return 0;
	}
	
	if(orbconf->orbis2d_initialized==1)
	{
		return orbconf->orbis2d_initialized;
	}
	//something weird happened
	return -1;
}

int orbis2dSetConf(Orbis2dConfig *conf)
{
	if(conf)
	{
		orbconf=conf;
		orbis2d_external_conf=1;
		return orbconf->orbis2d_initialized;
	}
	
	return 0; 
}
int orbis2dInitWithConf(Orbis2dConfig *conf)
{
	int ret;
	ret=orbis2dSetConf(conf);
	if(ret)
	{
		sys_log("liborbis2d already initialized using configuration external\n");
		sys_log("orbis2d_initialized=%d\n",orbconf->orbis2d_initialized);
		sys_log("ready to have a lot of fun...\n");
		return orbconf->orbis2d_initialized;
	}
	else
	{
		return 0;
	}
}

int orbis2dWaitFlipArg(SceKernelEqueue *flipQueue)
{
	int ret;
	int event_out;
	SceKernelEvent event;
	
	
	while(1)
	{
		SceVideoOutFlipStatus status;
		ret=sceVideoOutGetFlipStatus(orbconf->videoHandle, &status);
		if(ret>=0)
		{
			
			if(status.flipArg>=(orbconf->flipArgLog[orbconf->currentBuffer] +1))
			{
				return 0;
			}

			ret=sceKernelWaitEqueue(*flipQueue, &event, 1, &event_out, 0);
			if(ret>=0)
			{
				sys_log("liborbis2d sceKernelWaitEqueue return %d\n",ret);
				
			}
			else
			{
				sys_log("liborbis2d sceKernelWaitEqueue return error 0x%8x\n",ret);
			}
		}
		else
		{
			sys_log("liborbis2d sceVideoOutGetFlipStatus return error 0x%8x\n",ret);
				
		}

	}
	return 0;
}

void orbis2dFinishDrawing(int64_t flipArg)
{
    sceGnmFlushGarlic();

    // request flip to the buffer
    int ret = sceVideoOutSubmitFlip(orbconf->videoHandle, orbconf->currentBuffer, orbconf->flipMode, flipArg);

    orbconf->flipArgLog[orbconf->currentBuffer]=flipArg;
}

void orbis2dStartDrawing()
{
    orbis2dWaitFlipArg(&orbconf->flipQueue);
}

void orbis2dDrawPixelColor(int x, int y, uint32_t pixelColor)
{
    int pixel = (y * orbconf->pitch) + x;
    int color = pixelColor;

    ((uint32_t *)orbconf->surfaceAddr[orbconf->currentBuffer])[pixel]=color;
}

void orbis2dSetFontColor(uint32_t color) {
    font_color = color;
}

void orbis2dSetBackFontColor(uint32_t color) {
    backfont_color = color;
}

void orbis2dDrawCharacter(int character, int x, int y)
{
    for (int yy = 0; yy < 10; yy++)
    {
        uint8_t charPos = font[character * 10 + yy];
        int off = 8;
        for (int xx = 0; xx < 8; xx++)
        {           // font color : background color
            uint32_t clr = ((charPos >> xx) & 1) ? font_color : backfont_color;  // 0x00000000
            orbis2dDrawPixelColor(x + off, y + yy, clr);
            off--;
        }
    }
}

void orbis2dSetBackgroundColor(uint32_t color)
{
    orbconf->bgColor = color;
}

void orbis2dDrawString(int x, int y, const char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
        orbis2dDrawCharacter(str[i], x + i * 8, y);
}

void orbis2dDrawRectColor(int x, int w, int y, int h, uint32_t color)
{
	int x0, y0;
	for(y0=y;y0<y+h;y0++) 
	{
		for(x0=x;x0<x+w;x0++) 
		{
			orbis2dDrawPixelColor(x0,y0,color);
		}
	}
}

void orbis2dClearBuffer()
{
	orbis2dDrawRectColor(0, orbconf->width, 0, orbconf->height, orbconf->bgColor);
}

void orbis2dSwapBuffers()
{
	orbconf->currentBuffer=(orbconf->currentBuffer+1)%ORBIS2D_DISPLAY_BUFFER_NUM;
	//sys_log("liborbis2d currentBuffer  %d\n",orbconf->currentBuffer);
}

void *orbis2dMalloc(int size)
{
	uint64_t offset=orbconf->videoMemStackAddr;

	if((orbconf->videoMemStackAddr+size)>orbconf->videoMemStackTopAddr)
	{
		sys_log("orbis2dMalloc videoMemStackAddr 0x%lx greater than videoMemStackTopAddr 0x%lx \n",orbconf->videoMemStackAddr,orbconf->videoMemStackTopAddr);
		
		return NULL;
	}
	orbconf->videoMemStackAddr+=size;
	
	return (void *)(offset);
}

void orbis2dAllocDisplayBuffer(int displayBufNum)
{
	int i;

	int bufSize=orbconf->pitch*orbconf->height*orbconf->bytesPerPixel;
	for (i=0;i<displayBufNum;i++) 
	{
		orbconf->surfaceAddr[i]= orbis2dMalloc(bufSize);
		sys_log("liborbis2d orbis2dMalloc buffer %d, new pointer %p \n",i,	orbconf->surfaceAddr[i]);
		
	}
	sys_log("liborbis2d orbis2dAllocDisplayBuffer done\n");
}

int orbis2dInitDisplayBuffer(int num, int bufIndexStart)
{
	SceVideoOutBufferAttribute attr;
	int ret;
	sceVideoOutSetBufferAttribute(&attr,orbconf->pixelFormat,orbconf->tilingMode,0,orbconf->width,orbconf->height,orbconf->pitch);

	sys_log("liborbis2d sceVideoOutSetBufferAttribute done\n");
	
	ret=sceVideoOutRegisterBuffers(orbconf->videoHandle, bufIndexStart, orbconf->surfaceAddr, num, &attr);

	sys_log("liborbis2d sceVideoOutRegisterBuffers return 0x%8x\n",ret);

	return ret;
}

int orbis2dInitMemory()
{
	int ret;
	off_t start;

	const uint32_t align=2*1024*1024;

	ret=sceKernelAllocateDirectMemory(0,sceKernelGetDirectMemorySize(),orbconf->videoMemStackSize,align,3,&start);
	if(ret==0)
	{

		orbconf->videoMemOffset=start;

		void* pointer=NULL;
		
		ret=sceKernelMapDirectMemory(&pointer,orbconf->videoMemStackSize,0x33,0,start,align);
		if(ret==0)
		{
			orbconf->videoMemStackBaseAddr=(uintptr_t)pointer;
			orbconf->videoMemStackTopAddr=orbconf->videoMemStackBaseAddr+orbconf->videoMemStackSize;
			orbconf->videoMemStackAddr=orbconf->videoMemStackBaseAddr;
		}
	}
	return ret;
}

int orbis2dInitVideoHandle()
{
	int handle;
	int ret;

	handle=sceVideoOutOpen(0xff, 0, 0, NULL);
	
	if(handle<0)
	{
		sys_log("liborbis2d sceVideoOutOpen return error 0x%8x\n",handle);
	}
	else
	{
		sys_log("liborbis2d sceVideoOutOpen return video handle 0x%8x\n",handle);

		if(handle>0)
		{
			ret=sceKernelCreateEqueue(&orbconf->flipQueue,"queue to wait flip");
			if(ret==0)
			{
				sys_log("sceKernelCreateEqueue return %d\n",ret);
				
				ret=sceVideoOutAddFlipEvent(orbconf->flipQueue, handle, NULL);
				if(ret<0)
				{
					sys_log("liborbis2d sceVideoOutAddFlipEvent return 0x%8x\n",ret);
					ret=sceVideoOutClose(handle);
					sys_log("liborbis2d sceVideoOutClose return %d\n",ret);
					handle=-1;
				}
				else
				{
					sys_log("liborbis2d sceVideoOutAddFlipEvent return %d\n",ret);		
				}
			}
			else
			{
				sys_log("sceKernelCreateEqueue return error 0x%8x\n",ret);
				ret=sceVideoOutClose(handle);
				sys_log("liborbis2d sceVideoOutClose return %d\n",ret);
				
				handle=-1;
			}
		}	
	}		
	return handle;
}

int orbis2dInit()
{
	int ret;
	int bufIndex;

	if(orbis2dCreateConf()==1)
	{
		return orbconf->orbis2d_initialized;
	}
	if (orbconf->orbis2d_initialized==1) 
	{
		sys_log("liborbis2d is already initialized!\n");
		return orbconf->orbis2d_initialized;
	}
	//Get video handle
	orbconf->videoHandle=orbis2dInitVideoHandle();


	if(orbconf->videoHandle>0)
	{
		//init memory
		ret=orbis2dInitMemory();

		if(ret==0)
		{	
			sys_log("liborbis2d video memory initialized\n");

			//init memory for display buffers
			orbis2dAllocDisplayBuffer(ORBIS2D_DISPLAY_BUFFER_NUM);
			
			// register display buffers
			orbis2dInitDisplayBuffer(ORBIS2D_DISPLAY_BUFFER_NUM,0);

			// set status of each buffer with flipArg
			for(bufIndex=0;bufIndex<ORBIS2D_DISPLAY_BUFFER_NUM;bufIndex++) 
			{
				orbconf->flipArgLog[bufIndex]= -2; 
			}

			// prepare initial clear color to the display buffers
			for (bufIndex=0;bufIndex<ORBIS2D_DISPLAY_BUFFER_NUM;bufIndex++) 
			{
				orbis2dClearBuffer();
				orbis2dSwapBuffers();
			}
			
			ret=sceVideoOutSetFlipRate(orbconf->videoHandle,ORBIS2D_FLIP_RATE);
		}
		orbconf->orbis2d_initialized=1;
		sys_log("liborbis2d initialized\n");
	}
	return orbconf->orbis2d_initialized;
}
