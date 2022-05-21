#include <ULib.h>

/*
   z64ram = 0x8011FD40 # 0x8011FD3C, aligned
   z64rom = 0xB96EDC
 */

typedef struct {
	DmaEntry* file;
	DmaEntry* palette;
} NewSkyboxFiles;

const NewSkyboxFiles gNewSkyboxFiles[] = {
	{
		&gExtDmaTable[941],
		&gExtDmaTable[941 + 1],
	},
	{
		&gExtDmaTable[943],
		&gExtDmaTable[943 + 1],
	},
	{
		&gExtDmaTable[945],
		&gExtDmaTable[945 + 1],
	},
	{
		&gExtDmaTable[947],
		&gExtDmaTable[947 + 1],
	},
	{
		&gExtDmaTable[949],
		&gExtDmaTable[949 + 1],
	},
	{
		&gExtDmaTable[951],
		&gExtDmaTable[951 + 1],
	},
	{
		&gExtDmaTable[953],
		&gExtDmaTable[953 + 1],
	},
	{
		&gExtDmaTable[955],
		&gExtDmaTable[955 + 1],
	},
	{
		&gExtDmaTable[957],
		&gExtDmaTable[957 + 1],
	},
};