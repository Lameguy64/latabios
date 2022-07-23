/* Very simple OpROM fixup utility by John "Lameguy" Wilbert Villamor
 * 2022 Meido-Tek Productions
 * 
 * This is a very simple utility that pads a Option ROM and calculates the
 * appropriate checksum byte to prepare it as a valid ROM image.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char *argv[])
{
    FILE *fp;
    const char *infile;
    unsigned char *rombuff;
    unsigned char csum;
    long i;
    long fsize,romsize;

    /* usage text */	
    if( argc <= 1 )
    {
	printf("Usage:\n");
	printf("    romfixup <file>\n");
	return 0;
    }
	
    infile = argv[1];

    /* open file */
    fp = fopen(infile, "rb");
    if( !fp )
    {
	printf("I cannot open %s!\n", infile);
	return 1;
    }
    
    /* get file size */
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
	
    if( fsize > 32768 )
    {
	fclose(fp);
	printf("ERROR: ROM image exceeds 32K!\n");
	return 1;
    }

    /* get size of ROM from the generic BIOS header */
    i = 0;
    fseek(fp, 2, SEEK_SET);
    fread(&i, 1, 1, fp);
	
    romsize = i<<9;
	
    printf("ROM size defined by header: %d bytes\n", romsize);
	
    if( fsize > romsize )
    {
	fclose(fp);
	printf("ERROR: ROM image is larger than size defined in header!\n");
	return 1;
    }
    
    /* allocate and fill buffer with FFs */
    rombuff = (unsigned char*)malloc(romsize);
    memset(rombuff, 0xff, romsize);
    
    /* load the ROM image */
    fseek(fp, 0, SEEK_SET);
    fread(rombuff, 1, fsize, fp);
	
    fclose(fp);
    
    /* checksum the ROM image minus 1 byte */
    csum = 0;
    for(i=0; i<romsize-1; i++)
	csum += rombuff[i];

    /* apply two's complement to the result */
    rombuff[i] = 256-csum;
	
    printf("Computed checksum: %x\n", rombuff[i]);
    printf("Test result: %x\n", ((unsigned char)csum+rombuff[i])%0x100);
    
    /* Write the fixed ROM */
    fp = fopen(infile, "wb");
    fwrite(rombuff, 1, romsize, fp);
    fclose(fp);
	
    free(rombuff);
	
    return 0;
    
} /* main */
