#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "grabber.h"
#include "yuvrgb.h"

static int ErrorExit(int code)
{
  fprintf(stderr, "Fatal error code %d\n", code);
  return code;
}

static int saveBmp(const char* filename)
{
	FILE* fd2 = fopen(filename, "w");
	if (fd2 == NULL)
	{
		fprintf(stderr, "Failed to write %s\n", filename);
		return 1;
	}
	unsigned char* video = malloc(luma.width * luma.height * 3); 

	YUVtoRGB(video, &luma, &chroma);

                // write bmp
                unsigned char hdr[14 + 40];
                int i = 0;
#define PUT32(x) hdr[i++] = ((x)&0xFF); hdr[i++] = (((x)>>8)&0xFF); hdr[i++] = (((x)>>16)&0xFF); hdr[i++] = (((x)>>24)&0xFF);
#define PUT16(x) hdr[i++] = ((x)&0xFF); hdr[i++] = (((x)>>8)&0xFF);
#define PUT8(x) hdr[i++] = ((x)&0xFF);
                PUT8('B'); PUT8('M');
                PUT32((((luma.width * luma.height) * 3 + 3) &~ 3) + 14 + 40);
                PUT16(0); PUT16(0); PUT32(14 + 40);
                PUT32(40); PUT32(luma.width); PUT32(luma.height);
                PUT16(1);
                PUT16(3*8); // bits
                PUT32(0); PUT32(0); PUT32(0); PUT32(0); PUT32(0); PUT32(0);
#undef PUT32
#undef PUT16
#undef PUT8
                fwrite(hdr, 1, i, fd2);
                int y;
                for (y = luma.height-1; y != 0; y -= 1) {
                        fwrite(video+(y*luma.width*3), luma.width*3, 1, fd2);
                }

	fclose(fd2);
	free(video);
}

static unsigned int median(const unsigned int* hist, unsigned int count)
{
	count /= 2;
	const unsigned int* p = hist;
	unsigned int sum = *p;
	while (sum < count)
	{
		++p;
		sum += *p;
	}
	return (p - hist);
}

static void showhist(unsigned int* hist)
{
    int base, i;
    for (base = 0; base < 256; base += 16)
    {
	for (i = 0; i < 16; ++i)
	{
		printf("%4d", hist[base + i]);
	}
	printf("\n");
    }
}

void printRGB(int y, int u, int v)
{
   // Formulas from Wikipedia...
   int lum = 9535 * (y-16);
   v -= 128;
   u -= 128;
#if MACHINE==dm7025
   int b = (lum + (13074 * v)) >> 13;
   int g = (lum - (6660 * v) - (3202 * u)) >> 13;
   int r = (lum + (16531 * u)) >> 13;
#else
   int r = (lum + (13074 * v)) >> 13;
   int g = (lum - (6660 * v) - (3202 * u)) >> 13;
   int b = (lum + (16531 * u)) >> 13;
#endif

   printf("(y=%d u=%d v=%d) -> (r=%d, g=%d, b=%d)", y, u, v, r, g, b);
}


int main(int argc, char** argv)
{
    int r;

    r = grabber_initialize();
    if (r != 0) return ErrorExit(r);

    r = grabber_begin();
    if (r != 0) return ErrorExit(r);

    //printf("Luma size: %dx%d (stride: %d)\n", luma.width, luma.height, luma.stride);
    int avgY = avg(luma.data, 0, luma.width, luma.stride, luma.height);
    printf("Avg luma: %d\n", avgY);
    //printf("Chroma size: %dx%d (stride: %d)\n", chroma.width, chroma.height, chroma.stride);
    //printf("Avg ch-U: %d\n", avg2(chroma.data, 0, chroma.width, chroma.stride, chroma.height));
    //printf("Avg ch-V: %d\n", avg2(chroma.data + 1, 0, chroma.width, chroma.stride, chroma.height));

    unsigned int hist[256] = {0};
    histogram2(chroma.data, 0, chroma.width, chroma.stride, chroma.height, hist);
    int mU = median(hist, chroma.width*chroma.height/2);
    memset(hist, 0, sizeof(hist));
    histogram2(chroma.data+1, 0, chroma.width, chroma.stride, chroma.height, hist);
    int mV = median(hist, chroma.width*chroma.height/2);

    printf("Median U=%d V=%d\n", mU, mV);
    //showhist(hist);
    printRGB(avgY, mU, mV);
    printf("\nLeftTop pixel: ");
    printRGB(luma.data[0], chroma.data[0], chroma.data[1]);
    printf("\npixel at 100,100: ");
    printRGB(luma.data[100*luma.stride+100], chroma.data[50*chroma.stride+100], chroma.data[50*chroma.stride+101]);
    printf("\n");

    saveBmp("/tmp/tmp.bmp");

    r = grabber_end();
    if (r != 0) return ErrorExit(r);

    r = grabber_destroy();
    return r;
}
