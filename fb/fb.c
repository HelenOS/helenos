/*
 * Copyright (C) 2006 Jakub Vana
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <ddi.h>
#include <task.h>
#include <stdlib.h>
#include <ddi.h>
#include <sysinfo.h>
#include <align.h>
#include <as.h>
#include <ipc/fb.h>


#include <ipc/ipc.h>
#include <ipc/services.h>
#include <unistd.h>
#include <stdlib.h>
#include <ipc/ns.h>

#include <kernel/errno.h>
#include <async.h>

#include "fb.h"

#define EFB (-1)

#define DEFAULT_BGCOLOR		0x000080
#define DEFAULT_FGCOLOR		0xffff00
#define DEFAULT_LOGOCOLOR	0x0000ff

#define MAIN_BGCOLOR		0x404000
#define MAIN_FGCOLOR		0x000000
#define MAIN_LOGOCOLOR	0x404000

#define SPACING (2)

#define H_NO_VFBS 3
#define V_NO_VFBS 3


static void fb_putchar(int item,char ch);
int create_window(int item,unsigned int x, unsigned int y,unsigned int x_size, unsigned int y_size,
	unsigned int BGCOLOR,unsigned int FGCOLOR,unsigned int LOGOCOLOR);
void fb_init(int item,__address addr, unsigned int x, unsigned int y, unsigned int bpp, unsigned int scan,
	unsigned int BGCOLOR,unsigned int FGCOLOR,unsigned int LOGOCOLOR);


unsigned int mod_col(unsigned int col,int mod);



static int init_fb(void)
{
	__address fb_ph_addr;
	unsigned int fb_width;
	unsigned int fb_height;
	unsigned int fb_bpp;
	unsigned int fb_scanline;
	__address fb_addr;
	int a=0;
	int i,j,k;
	int w;
	char text[]="HelenOS Framebuffer driver\non Virtual Framebuffer\nVFB ";

	fb_ph_addr=sysinfo_value("fb.address.physical");
	fb_width=sysinfo_value("fb.width");
	fb_height=sysinfo_value("fb.height");
	fb_bpp=sysinfo_value("fb.bpp");
	fb_scanline=sysinfo_value("fb.scanline");

	fb_addr=ALIGN_UP(((__address)set_maxheapsize(USER_ADDRESS_SPACE_SIZE_ARCH>>1)),PAGE_SIZE);


	
	map_physmem(task_get_id(),(void *)((__address)fb_ph_addr),(void *)fb_addr,
		    (fb_scanline*fb_height+PAGE_SIZE-1)>>PAGE_WIDTH,1);
	
	fb_init(0,fb_addr, fb_width, fb_height, fb_bpp, fb_scanline,
		MAIN_BGCOLOR,MAIN_FGCOLOR,MAIN_LOGOCOLOR);

	fb_putchar(0,'\n');
	fb_putchar(0,' ');

	for(i=0;i<H_NO_VFBS;i++)
		for(j=0;j<V_NO_VFBS;j++) {
			w = create_window(0,(fb_width/H_NO_VFBS)*i+SPACING,
					  (fb_height/V_NO_VFBS)*j+SPACING,(fb_width/H_NO_VFBS)-2*SPACING ,
					  (fb_height/V_NO_VFBS)-2*SPACING,mod_col(DEFAULT_BGCOLOR,/*i+j*H_NO_VFBS*/0),
					  mod_col(DEFAULT_FGCOLOR,/*i+j*H_NO_VFBS*/0),
					  mod_col(DEFAULT_LOGOCOLOR,/*i+j*H_NO_VFBS)*/0));
			
			if( w== EFB)
				return -1;
			
			for(k=0;text[k];k++) 
				fb_putchar(w,text[k]);
			fb_putchar(w,w+'0');
			fb_putchar(w,'\n');
		}
	return 0;
}

int vfb_no = 1;
void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int vfb = vfb_no++;

	if (vfb > 9) {
		ipc_answer_fast(iid, ELIMIT, 0,0);
		return;
	}
	ipc_answer_fast(iid, 0, 0, 0);

	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			ipc_answer_fast(callid,0,0,0);
			return; /* Exit thread */

		case FB_PUTCHAR:
			ipc_answer_fast(callid,0,0,0);
			fb_putchar(vfb,IPC_GET_ARG2(call));
			break;
		default:
			ipc_answer_fast(callid,ENOENT,0,0);
		}
	}
}

int main(int argc, char *argv[])
{
	ipc_call_t call;
	ipc_callid_t callid;
	char connected = 0;
	int res;
	int c;
	ipcarg_t phonead;
	
	ipcarg_t retval, arg1, arg2;

	if(!sysinfo_value("fb")) return -1;


	if ((res = ipc_connect_to_me(PHONE_NS, SERVICE_VIDEO, 0, &phonead)) != 0) 
		return -1;
	
	init_fb();

	async_manager();
	/* Never reached */
	return 0;
}
/*
 * Copyright (C) 2006 Ondrej Palkovsky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "font-8x16.h"
#include <string.h>

#include "helenos.xbm"






#define GRAPHICS_ITEMS 1024


/***************************************************************/
/* Pixel specific fuctions */

typedef void (*putpixel_fn_t)(int item,unsigned int x, unsigned int y, int color);
typedef int (*getpixel_fn_t)(int item,unsigned int x, unsigned int y);

typedef struct framebuffer_descriptor
{
	__u8 *fbaddress ;

	unsigned int xres ;
	unsigned int yres ;
	unsigned int scanline ;
	unsigned int pixelbytes ;

	unsigned int position ;
	unsigned int columns ;
	unsigned int rows ;
	
	unsigned int BGCOLOR;
	unsigned int FGCOLOR;
	unsigned int LOGOCOLOR;
	
	putpixel_fn_t putpixel;
	getpixel_fn_t getpixel;
	
}framebuffer_descriptor_t;

void * graphics_items[GRAPHICS_ITEMS+1]={NULL};

#define FB(__a__,__b__) (((framebuffer_descriptor_t*)(graphics_items[__a__]))->__b__)

#define COL_WIDTH	8
#define ROW_BYTES	(FB(item,scanline) * FONT_SCANLINES)
#define RED(x, bits)	((x >> (16 + 8 - bits)) & ((1 << bits) - 1))
#define GREEN(x, bits)	((x >> (8 + 8 - bits)) & ((1 << bits) - 1))
#define BLUE(x, bits)	((x >> (8 - bits)) & ((1 << bits) - 1))

#define POINTPOS(x, y)	(y * FB(item,scanline) + x * FB(item,pixelbytes))




/** Put pixel - 24-bit depth, 1 free byte */
static void putpixel_4byte(int item,unsigned int x, unsigned int y, int color)
{
	*((__u32 *)(FB(item,fbaddress) + POINTPOS(x, y))) = color;
}

/** Return pixel color - 24-bit depth, 1 free byte */
static int getpixel_4byte(int item,unsigned int x, unsigned int y)
{
	return *((__u32 *)(FB(item,fbaddress) + POINTPOS(x, y))) & 0xffffff;
}

/** Put pixel - 24-bit depth */
static void putpixel_3byte(int item,unsigned int x, unsigned int y, int color)
{
	unsigned int startbyte = POINTPOS(x, y);

#if (defined(BIG_ENDIAN) || defined(FB_BIG_ENDIAN))
	FB(item,fbaddress)[startbyte] = RED(color, 8);
	FB(item,fbaddress)[startbyte + 1] = GREEN(color, 8);
	FB(item,fbaddress)[startbyte + 2] = BLUE(color, 8);
#else
	FB(item,fbaddress)[startbyte + 2] = RED(color, 8);
	FB(item,fbaddress)[startbyte + 1] = GREEN(color, 8);
	FB(item,fbaddress)[startbyte + 0] = BLUE(color, 8);
#endif

}

/** Return pixel color - 24-bit depth */
static int getpixel_3byte(int item,unsigned int x, unsigned int y)
{
	unsigned int startbyte = POINTPOS(x, y);



#if (defined(BIG_ENDIAN) || defined(FB_BIG_ENDIAN))
	return FB(item,fbaddress)[startbyte] << 16 | FB(item,fbaddress)[startbyte + 1] << 8 | FB(item,fbaddress)[startbyte + 2];
#else
	return FB(item,fbaddress)[startbyte + 2] << 16 | FB(item,fbaddress)[startbyte + 1] << 8 | FB(item,fbaddress)[startbyte + 0];
#endif
								

}

/** Put pixel - 16-bit depth (5:6:5) */
static void putpixel_2byte(int item,unsigned int x, unsigned int y, int color)
{
	/* 5-bit, 6-bits, 5-bits */ 
	*((__u16 *)(FB(item,fbaddress) + POINTPOS(x, y))) = RED(color, 5) << 11 | GREEN(color, 6) << 5 | BLUE(color, 5);
}

/** Return pixel color - 16-bit depth (5:6:5) */
static int getpixel_2byte(int item,unsigned int x, unsigned int y)
{
	int color = *((__u16 *)(FB(item,fbaddress) + POINTPOS(x, y)));
	return (((color >> 11) & 0x1f) << (16 + 3)) | (((color >> 5) & 0x3f) << (8 + 2)) | ((color & 0x1f) << 3);
}

/** Put pixel - 8-bit depth (3:2:3) */
static void putpixel_1byte(int item,unsigned int x, unsigned int y, int color)
{
	FB(item,fbaddress)[POINTPOS(x, y)] = RED(color, 3) << 5 | GREEN(color, 2) << 3 | BLUE(color, 3);
}

/** Return pixel color - 8-bit depth (3:2:3) */
static int getpixel_1byte(int item,unsigned int x, unsigned int y)
{
	int color = FB(item,fbaddress)[POINTPOS(x, y)];
	return (((color >> 5) & 0x7) << (16 + 5)) | (((color >> 3) & 0x3) << (8 + 6)) | ((color & 0x7) << 5);
}

/** Fill line with color BGCOLOR */
static void clear_line(int item,unsigned int y)
{
	unsigned int x;
	for (x = 0; x < FB(item,xres); x++)
		FB(item,putpixel)(item,x, y, FB(item,BGCOLOR));
}


/** Fill screen with background color */
static void clear_screen(int item)
{
	unsigned int y;
	for (y = 0; y < FB(item,yres); y++)
	{
		clear_line(item,y);
	}	
}


/** Scroll screen one row up */
static void scroll_screen(int item)
{
	unsigned int i;
	unsigned int j;

	for(j=0;j<FB(item,yres)-FONT_SCANLINES;j++)
		memcpy((void *) FB(item,fbaddress)+j*FB(item,scanline), 
			(void *) &FB(item,fbaddress)[ROW_BYTES+j*FB(item,scanline)], FB(item,pixelbytes)*FB(item,xres));

	//memcpy((void *) FB(item,fbaddress), (void *) &FB(item,fbaddress)[ROW_BYTES], FB(item,scanline) * FB(item,yres) - ROW_BYTES);

	/* Clear last row */
	for (i = 0; i < FONT_SCANLINES; i++)
		clear_line(item,(FB(item,rows) - 1) * FONT_SCANLINES + i);
}


static void invert_pixel(int item,unsigned int x, unsigned int y)
{
	FB(item,putpixel)(item, x, y, ~FB(item,getpixel)(item, x, y));
}


/** Draw one line of glyph at a given position */
static void draw_glyph_line(int item,unsigned int glline, unsigned int x, unsigned int y)
{
	unsigned int i;

	for (i = 0; i < 8; i++)
		if (glline & (1 << (7 - i))) {
			FB(item,putpixel)(item,x + i, y, FB(item,FGCOLOR));
		} else
			FB(item,putpixel)(item,x + i, y, FB(item,BGCOLOR));
}

/***************************************************************/
/* Character-console functions */

/** Draw character at given position */
static void draw_glyph(int item,__u8 glyph, unsigned int col, unsigned int row)
{
	unsigned int y;

	for (y = 0; y < FONT_SCANLINES; y++)
		draw_glyph_line(item ,fb_font[glyph * FONT_SCANLINES + y], col * COL_WIDTH, row * FONT_SCANLINES + y);
}

/** Invert character at given position */
static void invert_char(int item,unsigned int col, unsigned int row)
{
	unsigned int x;
	unsigned int y;

	for (x = 0; x < COL_WIDTH; x++)
		for (y = 0; y < FONT_SCANLINES; y++)
			invert_pixel(item,col * COL_WIDTH + x, row * FONT_SCANLINES + y);
}

/** Draw character at default position */
static void draw_char(int item,char chr)
{
	draw_glyph(item ,chr, FB(item,position) % FB(item,columns), FB(item,position) / FB(item,columns));
}

static void draw_logo(int item,unsigned int startx, unsigned int starty)
{
	unsigned int x;
	unsigned int y;
	unsigned int byte;
	unsigned int rowbytes;

	rowbytes = (helenos_width - 1) / 8 + 1;

	for (y = 0; y < helenos_height; y++)
		for (x = 0; x < helenos_width; x++) {
			byte = helenos_bits[rowbytes * y + x / 8];
			byte >>= x % 8;
			if (byte & 1)
				FB(item,putpixel)(item,startx + x, starty + y, FB(item,LOGOCOLOR));
		}
}

/***************************************************************/
/* Stdout specific functions */

static void invert_cursor(int item)
{
	invert_char(item,FB(item,position) % FB(item,columns), FB(item,position) / FB(item,columns));
}

/** Print character to screen
 *
 *  Emulate basic terminal commands
 */
static void fb_putchar(int item,char ch)
{
	
	switch (ch) {
		case '\n':
			invert_cursor(item);
			FB(item,position) += FB(item,columns);
			FB(item,position) -= FB(item,position) % FB(item,columns);
			break;
		case '\r':
			invert_cursor(item);
			FB(item,position) -= FB(item,position) % FB(item,columns);
			break;
		case '\b':
			invert_cursor(item);
			if (FB(item,position) % FB(item,columns))
				FB(item,position)--;
			break;
		case '\t':
			invert_cursor(item);
			do {
				draw_char(item,' ');
				FB(item,position)++;
			} while (FB(item,position) % 8);
			break;
		default:
			draw_char(item,ch);
			FB(item,position)++;
	}
	
	if (FB(item,position) >= FB(item,columns) * FB(item,rows)) {
		FB(item,position) -= FB(item,columns);
		scroll_screen(item);
	}
	
	invert_cursor(item);
	
}


/** Initialize framebuffer as a chardev output device
 *
 * @param addr Address of theframebuffer
 * @param x    Screen width in pixels
 * @param y    Screen height in pixels
 * @param bpp  Bits per pixel (8, 16, 24, 32)
 * @param scan Bytes per one scanline
 *
 */
void fb_init(int item,__address addr, unsigned int x, unsigned int y, unsigned int bpp, unsigned int scan,
	unsigned int BGCOLOR,unsigned int FGCOLOR,unsigned int LOGOCOLOR)
{
	
	if( (graphics_items[item]=malloc(sizeof(framebuffer_descriptor_t))) ==NULL) 
	{
		return;
	}

	switch (bpp) {
		case 8:
			FB(item,putpixel) = putpixel_1byte;
			FB(item,getpixel) = getpixel_1byte;
			FB(item,pixelbytes) = 1;
			break;
		case 16:
			FB(item,putpixel) = putpixel_2byte;
			FB(item,getpixel) = getpixel_2byte;
			FB(item,pixelbytes) = 2;
			break;
		case 24:
			FB(item,putpixel) = putpixel_3byte;
			FB(item,getpixel) = getpixel_3byte;
			FB(item,pixelbytes) = 3;
			break;
		case 32:
			FB(item,putpixel) = putpixel_4byte;
			FB(item,getpixel) = getpixel_4byte;
			FB(item,pixelbytes) = 4;
			break;
	}

		
	FB(item,fbaddress) = (unsigned char *) addr;
	FB(item,xres) = x;
	FB(item,yres) = y;
	FB(item,scanline) = scan;
	
	
	FB(item,rows) = y / FONT_SCANLINES;
	FB(item,columns) = x / COL_WIDTH;

	FB(item,BGCOLOR)=BGCOLOR;
	FB(item,FGCOLOR)=FGCOLOR;
	FB(item,LOGOCOLOR)=LOGOCOLOR;


	clear_screen(item);
	draw_logo(item,FB(item,xres) - helenos_width, 0);
	invert_cursor(item);

}


static int get_free_item()
{
	int item;
	for(item=0;graphics_items[item]!=NULL;item++);
	return (item==GRAPHICS_ITEMS)?EFB:item;
}

unsigned int mod_col(unsigned int col,int mod)
{
	if(mod & 1) col^=0xff;
	if(mod & 2) col^=0xff00;
	if(mod & 4) col^=0xff0000;
	return col;
}


int create_window(int item,unsigned int x, unsigned int y,unsigned int x_size, unsigned int y_size,
	unsigned int BGCOLOR,unsigned int FGCOLOR,unsigned int LOGOCOLOR)
{
	int item_new;
	
	if(EFB==(item_new=get_free_item()))return EFB;
	
	
	if( (graphics_items[item_new]=malloc(sizeof(framebuffer_descriptor_t))) ==NULL) 
	{
		return EFB;
	}
	
	
	
	FB(item_new,putpixel) = FB(item,putpixel);
	FB(item_new,getpixel) = FB(item,getpixel);
	FB(item_new,pixelbytes) = FB(item,pixelbytes);

	FB(item_new,fbaddress) = FB(item,fbaddress) + POINTPOS(x, y) ; 
	FB(item_new,xres) = x_size; 
	FB(item_new,yres) = y_size; 
	FB(item_new,scanline) =  FB(item,scanline);
	

	FB(item_new,rows) = y_size / FONT_SCANLINES; 
	FB(item_new,columns) = x_size / COL_WIDTH; 

	FB(item_new,BGCOLOR)=BGCOLOR;
	FB(item_new,FGCOLOR)=FGCOLOR;
	FB(item_new,LOGOCOLOR)=LOGOCOLOR;


	clear_screen(item_new); 
	draw_logo(item_new,FB(item_new,xres) - helenos_width, 0); 
	invert_cursor(item_new); 

	return item_new;
}



