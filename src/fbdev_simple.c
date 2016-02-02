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
} fb_device;

fb_device fb_dev;

void fb_init(void);
void draw(unsigned char a, unsigned char r, unsigned char g, unsigned char b);

inline size_t round_up_to_page_size(size_t x)
{
    return (x + (sysconf(_SC_PAGE_SIZE) - 1)) & ~(sysconf(_SC_PAGE_SIZE)-1);
}

int main (int argc, char **argv)
{
	struct timeval start;
	struct timeval current;

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

	fb_init();
	close(fb_dev.fb_fd);
	INFO("Feamebuffer Init - OK");

	gettimeofday(&start, NULL);

	INFO("  width=%d\n  height=%d", fb_dev.width, fb_dev.height);

	while(current.tv_sec <= start.tv_sec + 5)
	{
		printf("time:%ld\n", (long)current.tv_sec);

		draw(a, r, g, b);

		gettimeofday(&current, NULL);
	}


	INFO("Evrefing - OK");

	return OK;
}

void fb_init(void)
{
	const char *name = "/dev/fb0";
	int res = NONE;
	struct fb_fix_screeninfo *finfo = calloc(1, sizeof(struct fb_fix_screeninfo));
	struct fb_var_screeninfo *vinfo = calloc(1, sizeof(struct fb_var_screeninfo));
	size_t fb_size;
	size_t fb_size_round_up;


	fb_dev.fb_fd = open(name, O_RDWR);
	if (NONE == fb_dev.fb_fd)
		ERROR("Failed open FB device\n");
	INFO("Open FB - OK");

	res = ioctl(fb_dev.fb_fd, FBIOGET_VSCREENINFO, vinfo);
	if ( res < 0)
		ERROR("2: FBIOGET_VSCREENINFO failed\n  errno=%d", errno);

/*
	vinfo->yres     = 16;
	vinfo->xres     = 8;
*/

	vinfo->reserved[0] = 0;
	vinfo->reserved[1] = 0;
	vinfo->reserved[2] = 0;
	vinfo->xoffset = 0;
	vinfo->yoffset = 0;
	vinfo->activate = FB_ACTIVATE_NOW;
	vinfo->bits_per_pixel = 32;
	vinfo->red.offset     = 16;
	vinfo->red.length     = 8;
	vinfo->green.offset   = 8;
	vinfo->green.length   = 8;
	vinfo->blue.offset    = 0;
	vinfo->blue.length    = 8;
	vinfo->transp.offset  = 0;
	vinfo->transp.length  = 0;

	vinfo->yres_virtual = vinfo->yres * MAX_BUF;
/*
	res = ioctl(fb_dev.fb_fd, FBIOPAN_DISPLAY, vinfo);
	if(res < 0)
	{
		INFO("2.5: FBIOPUT_VSCREENINFO failed\n  errno=%d", errno);
	}
*/

	res = ioctl(fb_dev.fb_fd, FBIOPAN_DISPLAY, vinfo);
	if(res < 0)
	{
		INFO("3: FBIOPUT_VSCREENINFO failed\n  errno=%d", errno);
		vinfo->yres_virtual = vinfo->yres;
		INFO("Page flip not supported\n");
	}

	if(vinfo->yres_virtual < (vinfo->yres *MAX_BUF))
	{
		vinfo->yres_virtual = vinfo->yres;
		INFO("Page flip not supported\n");
	}

	fb_dev.width = vinfo->xres;
	fb_dev.height = vinfo->yres;


	INFO("\n"
		 " VInfo\n"
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
		  finfo->id,
		  vinfo->xres,
		  vinfo->yres,
		  vinfo->xres_virtual,
		  vinfo->yres_virtual,
		  vinfo->bits_per_pixel,
		  vinfo->red.offset, vinfo->red.length,
		  vinfo->green.offset, vinfo->green.length,
		  vinfo->blue.offset, vinfo->blue.length,
		  vinfo->transp.offset, vinfo->transp.length,
		  vinfo->activate,
		  vinfo->width,
		  vinfo->height);

	res = ioctl(fb_dev.fb_fd, FBIOGET_FSCREENINFO, finfo);
	if (res < 0)
		ERROR("5: FBIOGET_FSCREENINFO failed\n  errno=%d", errno);

	INFO("\n"
		 " FInfo\n"
		 "   finfo->smem_len     = %d\n"
		 "   finfo->line_length  = %d\n",
		 finfo->smem_len,
		 finfo->line_length);

	if (finfo->smem_len <= 0)
		INFO("SMEM less then 0");


	fb_size = vinfo->xres * vinfo->yres_virtual;
	fb_size_round_up = round_up_to_page_size(vinfo->xres * vinfo->yres * vinfo->bits_per_pixel / 8);
	size_t size = vinfo->xres * vinfo->yres * vinfo->bits_per_pixel / 8 * 3;

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


	free(vinfo);
	free(finfo);
}

void draw(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
{
	unsigned int *pixel = (unsigned int *)fb_dev.vaddr;
	int i = 0;

	for (i = 0; i < fb_dev.width * fb_dev.height; i++)
		pixel[i] = (a << 24) | (b << 16) | (g << 8) | r;
}


