/**
 * File              : ss_blast.c
 * License           : GPL-3.0-or-later
 * Author            : Peter Gu <github.com/regymm>
 * Date              : 2024.02.11
 * Last Modified Date: 2024.02.12
 */

// code from https://github.com/WangXuan95/Zynq-Tutorial/blob/main/zedboard_app/led_gpio/led_gpio.c
// implemented according to XAPP583
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>

// copied from ustcfg
static const unsigned char BitReverseTable256[] =
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

unsigned int bitswap_uint32(unsigned int data) {
	return (BitReverseTable256[data & 0xff] << 24) |
	(BitReverseTable256[(data >> 8) & 0xff] << 16) |
	(BitReverseTable256[(data >> 16) & 0xff] << 8) |
	(BitReverseTable256[(data >> 24) & 0xff]);
}

#define GPIO_BASEADDR 0x41200000
#define GPIO0_OFFSET 0x0
#define GPIO1_OFFSET 0x8
#define GPIO_MAP_SIZE 0x100

void *gpio_map_addr = NULL;

unsigned int ins = 0;
unsigned int outs = 0;

void update_ins()
{
	// GPIO1 [1:0] for input only
	ins = *((volatile unsigned int*) (gpio_map_addr + GPIO1_OFFSET)) & 0x3;
	/*printf("Update inputs : %01x\n", ins);*/
}

void update_outs()
{
	// GPIO1 [5:2] for output only
	*((volatile unsigned int*) (gpio_map_addr + GPIO1_OFFSET)) = outs << 2;
	/*printf("Update outputs: %03x\n", outs);*/
}

int done()
{
	update_ins();
	return ins & 0x1;
}
int init_b()
{
	update_ins();
	return ins & 0x2;
}
void cfg_write(int shift, int x)
{
	unsigned int mask = 0x1 << shift;
	outs = x ? (outs | mask) : (outs & ~mask);
	update_outs();
	/*printf("Mask with shift %d: %03x, x=%d\n", shift, mask, x);*/
}
void cclk(int x) { cfg_write(3, x); }
void prog_b(int x) { cfg_write(2, x); }
void cs_b(int x) { cfg_write(1, x); }
void rdwr_b(int x) { cfg_write(0, x); }
void d(unsigned int x) {
	x = bitswap_uint32(x);
	*((volatile unsigned int*) (gpio_map_addr + GPIO0_OFFSET)) = x;
	/*printf("%08x\n", x);*/
}

/* Assert and Deassert CCLK */
 void shift_cclk(int count)
{
	cclk(0);
	for (int i = 0; i < count; i++) {
		cclk(1);
		cclk(0);
	}
}

/* Serialize word and clock each bit on target's DIN and CCLK pins */
void shift_word_out(unsigned int data32)
{
	d(data32);
	shift_cclk(1);
	return;
}

int main(int argc, char* argv[])
{
	int i = 0;

	if (argc != 2) {
		printf("Usage ./ss_blast [bitstream_name]\n");
		exit(-1);
	}

	int memfd;
	if ((memfd = open("/dev/mem", O_RDWR | O_DSYNC)) == -1) {
		printf("Can't open /dev/mem!\n");
		exit(-2);
	}

	gpio_map_addr = mmap(0, GPIO_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, GPIO_BASEADDR);
	if (gpio_map_addr == (void*) -1) {
		printf("Can't map the CDMA-Control-Port to user space!\n");
		exit(-3);
	}

	FILE* bit = fopen(argv[1], "rb");
	if (!bit) {
		printf("Error reading bitstream file %s!\n", argv[1]);
		exit(-4);
	}
	fseek(bit, 0, SEEK_END);
	unsigned int bsize = ftell(bit);
	fseek(bit, 0, SEEK_SET);
	// openxc7 and vivado bitstream have different header offset<
	// making the 000000bb11220044 sync sequence off the 32-bit word
	int skip = (4 - bsize % 4) % 4;
	unsigned int *buf = (unsigned int *) malloc(bsize * 4);
	unsigned int rsize = fread((void*)buf + skip, 1, bsize - skip, bit);
	fclose(bit);
	if (bsize - skip != rsize) {
		printf("Reading entire bitstream encountered error!\n");
		free(buf);
		return -5;
	}

	printf("Begin blasting...\n");

	/* Bring csi_b, rdwr_b Low and program_b High */
	prog_b(1);
	rdwr_b(0);
	cs_b(0);
	/* Configuration Reset */
	prog_b(0);
	/* PROGRAM_B pulse width. Check the target FPGA data sheet for the
	* TPROGRAM pulse width. One microsecond is safe for any 7 series FPGA
	*/
	usleep(1);
	prog_b(1);

	/* Wait for Device Initialization */
	i = 0;
	while(init_b() == 0) {
		usleep(1000);
		if (i++ > 5) {
			printf("Waited a long time but init_b pin not high yet.\n");
			exit(-6);
		}
	}

	/* Configuration (Bitstream) Load */
	for (int i = 0; i < bsize / 4; i++) {
		if (i%100000 == 0)
			printf("%d/%d blasted. \n", i, bsize/4);
		shift_word_out(buf[i]);
	}
	free(buf);

	/* Check INIT_B */
	if (init_b() == 0) {
		printf("init_b invalid after programming!\n");
		exit(-7);
	}
	/* Wait for DONE to assert */
	i = 0;
	while(done() == 0) {
		shift_cclk(100);
		usleep(1000);
		if (i++ > 5) {
			printf("Waited a long time but done pin not high yet.\n");
			exit(-8);
		}
	}
	/* Compensate for Special Startup Conditions */
	shift_cclk(8);
	printf("Blast finished.\n");
	return 0;
}
