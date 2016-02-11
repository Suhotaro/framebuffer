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
#include <time.h>
#include <pthread.h>

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

#define CONCAT(str) #str

enum
{
    NONE = -1,
    OK = 0,
    ERROR = 1,
};

enum
{
    DEFAULT = -1,
    DRAW = 0,
    RANDOM_COLOR,
    DPMS_ON,
    DPMS_OFF,
    DPMS_STANDBY,
    DPMS_SUSPEND,
    REINIT,
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

/*--------------------------- Globals ---------------------------*/
static fb_device fb_dev;
static int cmd = DRAW;
unsigned char r = 0xFF, g = 0x00, b = 0x00;
int flag = 0;

void *fb_routine(void *ptr);
void *console_routine(void *ptr);

void page_flip(unsigned char r, unsigned char g, unsigned char b);
void fb_init(void);

void DPMS_set(int dpms_value);

void draw_with_offset(unsigned char a,
                      unsigned char r,
                      unsigned char g,
                      unsigned char b,
                      unsigned int offset);

inline size_t round_up_to_page_size(size_t x)
{
    return (x + (sysconf(_SC_PAGE_SIZE) - 1)) & ~(sysconf(_SC_PAGE_SIZE)-1);
}

int main()
{
    pthread_t thr1, thr2;

    int ret;

    ret = pthread_create(&thr1, NULL, fb_routine, NULL);
    if (ret)
    {
        perror ("Failed create first thread\n");
        exit(1);
    }

    ret = pthread_create(&thr2, NULL, console_routine, NULL);
    if (ret)
    {
        perror ("Failed create first thread\n");
        exit(1);
    }

    pthread_join(thr1, NULL);
    pthread_join(thr2, NULL);

    return 0;
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
     * DPMS ON
     */
    /*
    res = ioctl(fb_dev.fb_fd, FBIOBLANK, FB_BLANK_UNBLANK);
    if ( res < 0)
        ERROR("6: FBIOGET_FSCREENINFO failed\n  errno=%d", errno);
    */

    free(fb_dev.vinfo);
    free(fb_dev.finfo);
}

void draw_with_offset(unsigned char a,
                      unsigned char r,
                      unsigned char g,
                      unsigned char b,
                      unsigned int offset)
{
    unsigned int *pixel = (unsigned int *)fb_dev.vaddr;
    int i = 0;

    for (i = 0; i < fb_dev.width * fb_dev.height; i++)
        pixel[i + offset] = (a << 24) | (b << 16) | (g << 8) | r;
}

void page_flip(unsigned char r, unsigned char g, unsigned char b)
{
    int crtc = 0;
    int res = 0;

    flag ^= 1;
    if (flag)
    {
        draw_with_offset(0, r, g, b, fb_dev.width * fb_dev.height);

        fb_dev.vinfo->activate = FB_ACTIVATE_VBL;
        fb_dev.vinfo->xoffset = 0;
        fb_dev.vinfo->yoffset = 0;

        res = ioctl(fb_dev.fb_fd, FBIOPAN_DISPLAY, fb_dev.vinfo);
        if(res < 0)
            INFO("FBIOPAN_DISPLAY failed %d", errno);

        res = ioctl(fb_dev.fb_fd, FBIO_WAITFORVSYNC, &crtc);
        if(res < 0)
            INFO("FBIO_WAITFORVSYNC failed %d", errno);
    }
    else
    {
        draw_with_offset(0, r, g, b, 0);

        fb_dev.vinfo->activate = FB_ACTIVATE_VBL;
        fb_dev.vinfo->xoffset = 0;
        fb_dev.vinfo->yoffset = fb_dev.vinfo->yres;

        res = ioctl(fb_dev.fb_fd, FBIOPAN_DISPLAY, fb_dev.vinfo);
        if(res < 0)
            INFO("FBIOPAN_DISPLAY failed %d", errno);

        res = ioctl(fb_dev.fb_fd, FBIO_WAITFORVSYNC, &crtc);
        if(res < 0)
            INFO("FBIO_WAITFORVSYNC failed %d", errno);
    }
}

void *fb_routine(void *ptr)
{
    fb_init();
    INFO("Feamebuffer Init - OK");

    unsigned char rr, gg, bb;

    srand(time(NULL));

    INFO("  width=%d\n  height=%d", fb_dev.width, fb_dev.height);

    while(1)
    {
        switch (cmd)
        {
            case DRAW:
                page_flip(r, g, b);

                break;
            case RANDOM_COLOR:

                rr = rand() % 0xFF;
                gg = rand() % 0xFF;
                bb = rand() % 0xFF;

                page_flip(rr, gg, bb);

                break;

            case DPMS_ON:
            case DPMS_OFF:
            case DPMS_SUSPEND:
            case DPMS_STANDBY:

                DPMS_set(cmd);

                cmd = DEFAULT;

                break;

            case REINIT:

                fb_init();
                cmd = 0;

                break;

            default:

                sleep(1);
                printf("default");

                break;
        }
    }

    close(fb_dev.fb_fd);

    return NULL;
}

void *console_routine(void *ptr)
{
    int user_cmd = -1;

    while(1)
    {
        sleep(1);

        printf ("\n------------------------------------------\n");

        printf ("info:\n"
                "  DRAW:         0\n"
                "  RANDOM COLOR: 1\n"
                "  DPMS_ON:      2\n"
                "  DPMS_OFF:     3\n"
                "  DPMS_STANDBY: 4\n"
                "  DPMS_SUSPEND: 5\n"
                "  DPMS_SUSPEND: 6\n"
        );
        printf("input command: ");
        int ret = scanf("%d", &user_cmd);
        (void) ret;

        switch (user_cmd)
        {

            case 0: /* RANDOM COLOR */

                printf("  cmd:" CONCAT(DRAW) "\n");
                cmd = DRAW;

                break;

            case 1: /* RANDOM COLOR */

                printf("  cmd:" CONCAT(RANDOM_COLOR) "\n");
                cmd = RANDOM_COLOR;

                break;

            case 2: /* DPMS_ON */
            case 3: /* DPMS_OFF */
            case 4: /* DPMS_STANDBY */
            case 5: /* DPMS_SUSPEND */

                printf("  cmd:" CONCAT(DPMS) "\n");
                cmd = user_cmd;

                break;

            case 6: /* REINIT */

                printf("  cmd:" CONCAT(REINIT) "\n");
                cmd = user_cmd;

                break;


            default:
                break;
        }

        printf ("------------------------------------------\n\n");

        user_cmd = -1;
    }

    return NULL;
}

void DPMS_set(int dpms_value)
{
    int fbmode = -1;
    int ret = 0;

    switch (dpms_value)
    {
        case DPMS_ON: fbmode = FB_BLANK_UNBLANK;
            break;

        case DPMS_OFF: fbmode = FB_BLANK_POWERDOWN;
            break;

        case DPMS_STANDBY: fbmode = FB_BLANK_VSYNC_SUSPEND;
            break;

        case DPMS_SUSPEND: fbmode = FB_BLANK_HSYNC_SUSPEND;
            break;

        default: perror("Never get here\n");
            break;
    }

    ret = ioctl(fb_dev.fb_fd, FBIOBLANK, (void *)fbmode);
    if (ret < 0)
    {
        perror("FBIOBLANK ioctl failed");

    }

    cmd = -1;
}




