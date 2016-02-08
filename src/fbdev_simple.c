/*
 * Roman Peresipkyn
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/time.h>

#define MAX_BUF 3

#define ERROR(fmt, arg...)              \
do {                                    \
    printf("ERROR: %s: %d: "fmt"\n",    \
		__func__,                       \
		__LINE__,                       \
		##arg);                         \
	exit(1);                            \
}  while(0);

#define INFO(fmt, arg...)      \
do {                           \
    printf("INFO: "fmt"\n",    \
		##arg);                \
}  while(0);

enum
{
	NONE = -1,
	OK = 0,
	ERROR = 1,
};

typedef struct
{
	int fb_fd;
	void *vaddr;

	int width;
	int height;

	int size;

	struct fb_fix_screeninfo *finfo;
	struct fb_var_screeninfo *vinfo;
} fb_device;

fb_device fb_dev;

void fb_init(void);
void draw(unsigned char a, unsigned char r, unsigned char g, unsigned char b);
void fill_draw(void);

inline size_t round_up_to_page_size(size_t x)
{
    return (x + (sysconf(_SC_PAGE_SIZE) - 1)) & ~(sysconf(_SC_PAGE_SIZE)-1);
}

int main (int argc, char **argv)
{
	struct timeval start;
	struct timeval current;

	int res = 0;
	int flag = 0;

	unsigned char a = 0x00;
	unsigned char r = 0xFF;
	unsigned char g = 0x00;
	unsigned char b = 0x00;

	if (argc > 1)
	{
		printf("color %d %d %d %d\n",
				(unsigned char)atoi(argv[1]),
				(unsigned char)atoi(argv[2]),
				(unsigned char)atoi(argv[3]),
				(unsigned char)atoi(argv[4]));

		r = (unsigned char)atoi(argv[1]);
		g = (unsigned char)atoi(argv[2]);
		b = (unsigned char)atoi(argv[3]);
		a = (unsigned char)atoi(argv[4]);
	}

	(void) r;
	(void) g;
	(void) b;
	(void) a;

	fb_init();
	INFO("Feamebuffer Init - OK");

	gettimeofday(&start, NULL);

	INFO("  width=%d\n  height=%d", fb_dev.width, fb_dev.height);

	fill_draw();

	while(current.tv_sec <= start.tv_sec + 5)
	{
		//sleep(1);
		int crtc = 0;

		printf("time:%ld\n", (long)current.tv_sec);

		res = ioctl(fb_dev.fb_fd, FBIO_WAITFORVSYNC, &crtc);
		if(res < 0)
			INFO("FBIO_WAITFORVSYNC failed %d", errno);

		flag ^= 1;
		if (!flag)
		{
			fb_dev.vinfo->activate = FB_ACTIVATE_VBL;
			fb_dev.vinfo->xoffset = 0;
			fb_dev.vinfo->yoffset = 0;

			res = ioctl(fb_dev.fb_fd, FBIOPAN_DISPLAY, fb_dev.vinfo);
			if(res < 0)
				INFO("FBIOPAN_DISPLAY failed %d", errno);
		}
		else
		{
			fb_dev.vinfo->activate = FB_ACTIVATE_VBL;
			fb_dev.vinfo->xoffset = 0;
			fb_dev.vinfo->yoffset = fb_dev.vinfo->yres;

			res = ioctl(fb_dev.fb_fd, FBIOPAN_DISPLAY, fb_dev.vinfo);
			if(res < 0)
				INFO("FBIOPAN_DISPLAY failed %d", errno);
		}

		gettimeofday(&current, NULL);
	}

	close(fb_dev.fb_fd);
	INFO("Evrefing - OK");

	return OK;
}

void fb_init(void)
{
	const char *name = "/dev/fb0";
	fb_dev.finfo = calloc(1, sizeof(struct fb_fix_screeninfo));
	fb_dev.vinfo = calloc(1, sizeof(struct fb_var_screeninfo));
	size_t fb_size;
	size_t fb_size_round_up;
	int res = NONE;


	fb_dev.fb_fd = open(name, O_RDWR);
	if (NONE == fb_dev.fb_fd)
		ERROR("Failed open FB device\n");
	INFO("Open FB - OK");

	res = ioctl(fb_dev.fb_fd, FBIOGET_VSCREENINFO, fb_dev.vinfo);
	if ( res < 0)
		ERROR("2: FBIOGET_VSCREENINFO failed\n  errno=%d", errno);

/*
	fb_dev.vinfo->yres     = 16;
	fb_dev.vinfo->xres     = 8;
*/

	fb_dev.vinfo->reserved[0] = 0;
	fb_dev.vinfo->reserved[1] = 0;
	fb_dev.vinfo->reserved[2] = 0;
	fb_dev.vinfo->xoffset = 0;
	fb_dev.vinfo->yoffset = 0;
	fb_dev.vinfo->activate = FB_ACTIVATE_VBL;;
	fb_dev.vinfo->bits_per_pixel = 32;
	fb_dev.vinfo->red.offset     = 16;
	fb_dev.vinfo->red.length     = 8;
	fb_dev.vinfo->green.offset   = 8;
	fb_dev.vinfo->green.length   = 8;
	fb_dev.vinfo->blue.offset    = 0;
	fb_dev.vinfo->blue.length    = 8;
	fb_dev.vinfo->transp.offset  = 0;
	fb_dev.vinfo->transp.length  = 0;

	fb_dev.vinfo->yres_virtual = fb_dev.vinfo->yres * MAX_BUF;
/*
	res = ioctl(fb_dev.fb_fd, FBIOPAN_DISPLAY, fb_dev.vinfo);
	if(res < 0)
	{
		INFO("2.5: FBIOPUT_VSCREENINFO failed\n  errno=%d", errno);
	}
*/

	res = ioctl(fb_dev.fb_fd, FBIOPAN_DISPLAY, fb_dev.vinfo);
	if(res < 0)
	{
		INFO("3: FBIOPUT_VSCREENINFO failed\n  errno=%d", errno);
		fb_dev.vinfo->yres_virtual = fb_dev.vinfo->yres;
		INFO("Page flip not supported\n");
	}

	if(fb_dev.vinfo->yres_virtual < (fb_dev.vinfo->yres *MAX_BUF))
	{
		fb_dev.vinfo->yres_virtual = fb_dev.vinfo->yres;
		INFO("Page flip not supported\n");
	}

	fb_dev.width = fb_dev.vinfo->xres;
	fb_dev.height = fb_dev.vinfo->yres;


	INFO("\n"
		 " fb_dev.vinfo\n"
		 "   fb           = %d\n"
		 "   id           = %s\n"
		 "   xres         = %d px \n"
		 "   yres         = %d px \n"
		 "   xres_virtual = %d px \n"
		 "   yres_virtual = %d px \n"
		 "   bpp          = %d    \n"
		 "   r            = %2u:%u\n"
		 "   g            = %2u:%u\n"
		 "   b            = %2u:%u\n"
		 "   t            = %2u:%u\n"
		 "   active       = %d    \n"
		 "   width        = %d mm \n"
		 "   height       = %d mm \n",
		  fb_dev.fb_fd,
		  fb_dev.finfo->id,
		  fb_dev.vinfo->xres,
		  fb_dev.vinfo->yres,
		  fb_dev.vinfo->xres_virtual,
		  fb_dev.vinfo->yres_virtual,
		  fb_dev.vinfo->bits_per_pixel,
		  fb_dev.vinfo->red.offset, fb_dev.vinfo->red.length,
		  fb_dev.vinfo->green.offset, fb_dev.vinfo->green.length,
		  fb_dev.vinfo->blue.offset, fb_dev.vinfo->blue.length,
		  fb_dev.vinfo->transp.offset, fb_dev.vinfo->transp.length,
		  fb_dev.vinfo->activate,
		  fb_dev.vinfo->width,
		  fb_dev.vinfo->height);

	res = ioctl(fb_dev.fb_fd, FBIOGET_FSCREENINFO, fb_dev.finfo);
	if (res < 0)
		ERROR("5: FBIOGET_FSCREENINFO failed\n  errno=%d", errno);

	INFO("\n"
		 " fb_dev.finfo\n"
		 "   fb_dev.finfo->smem_len     = %d\n"
		 "   fb_dev.finfo->line_length  = %d\n",
		 fb_dev.finfo->smem_len,
		 fb_dev.finfo->line_length);

	if (fb_dev.finfo->smem_len <= 0)
		INFO("SMEM less then 0");


	fb_size = fb_dev.vinfo->xres * fb_dev.vinfo->yres_virtual;
	fb_size_round_up = round_up_to_page_size(fb_dev.vinfo->xres * fb_dev.vinfo->yres * fb_dev.vinfo->bits_per_pixel / 8);
	size_t size = fb_dev.vinfo->xres * fb_dev.vinfo->yres * fb_dev.vinfo->bits_per_pixel / 8 * 3;

	INFO("\n"
		 " FB\n"
		 "   fb_size     = %zu\n"
		 "   fb_size_rup = %zu\n"
		 "   size        = %zu\n"
		 "   pagesize    = %ld\n"
		 "   page_size   = %ld\n",
			fb_size,
			fb_size_round_up,
			size,
			sysconf(_SC_PAGESIZE),
			sysconf(_SC_PAGE_SIZE));

	int refreshRate = 1000000000000000LLU /
	    (
	        (uint64_t)( fb_dev.vinfo->upper_margin + fb_dev.vinfo->lower_margin + fb_dev.vinfo->yres + fb_dev.vinfo->vsync_len)
	        * ( fb_dev.vinfo->left_margin  + fb_dev.vinfo->right_margin + fb_dev.vinfo->xres + fb_dev.vinfo->hsync_len)
	        * fb_dev.vinfo->pixclock
	    );

	INFO("\n"
		 " My\n"
		 "   refresh    =%d"
		 "   vsync_len  =%d"
		 "   hvsync_len =%d",
		 refreshRate,
		 fb_dev.vinfo->vsync_len,
		 fb_dev.vinfo->hsync_len
		 );


	fb_dev.vaddr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fb_dev.fb_fd, 0);
	if (fb_dev.vaddr == MAP_FAILED)
		ERROR("MMap failed, errno=%d", errno);
	INFO("MMap - OK\n  vaddr=%p", fb_dev.vaddr);

	fb_dev.size = size;

	memset(fb_dev.vaddr, 0, size);


	/*
	 * This ioctl is used in Android sources, don't know what is it for?
	 */
    /*
	res = ioctl(fb_dev.fb_fd, FBIOBLANK, FB_BLANK_UNBLANK);
	if ( res < 0)
		ERROR("6: FBIOGET_FSCREENINFO failed\n  errno=%d", errno);
	*/


	free(fb_dev.vinfo);
	free(fb_dev.finfo);
}

void fill_draw(void)
{
	unsigned char r = 255;
	unsigned char g = 0;
	unsigned char b = 0;
	unsigned char a = 0;

	unsigned int *pixel = (unsigned int *)fb_dev.vaddr;
	int i = 0;

	for (i = 0; i < fb_dev.width * fb_dev.height; i++)
		pixel[i] = (a << 24) | (b << 16) | (g << 8) | r;

	r = 0;
	g = 0;
	b = 255;
	a = 0;

	for (i = 0; i < fb_dev.width * fb_dev.height; i++)
		pixel[i + (fb_dev.width * fb_dev.height)] = (a << 24) | (b << 16) | (g << 8) | r;
}


void draw(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned int *pixel = (unsigned int *)fb_dev.vaddr;
	int i = 0;

	for (i = 0; i < fb_dev.width * fb_dev.height; i++)
		pixel[i] = (a << 24) | (b << 16) | (g << 8) | r;
}


