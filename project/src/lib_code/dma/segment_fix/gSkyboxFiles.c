#include <oot_mq_debug/z64hdr.h>

typedef struct {
	DmaEntry* file;
	DmaEntry* palette;
} NewSkyboxFiles;

const NewSkyboxFiles gSkyboxFiles[] = {
	{
		&gDmaDataTable[941],
		&gDmaDataTable[941 + 1],
	},
	{
		&gDmaDataTable[943],
		&gDmaDataTable[943 + 1],
	},
	{
		&gDmaDataTable[945],
		&gDmaDataTable[945 + 1],
	},
	{
		&gDmaDataTable[947],
		&gDmaDataTable[947 + 1],
	},
	{
		&gDmaDataTable[949],
		&gDmaDataTable[949 + 1],
	},
	{
		&gDmaDataTable[951],
		&gDmaDataTable[951 + 1],
	},
	{
		&gDmaDataTable[953],
		&gDmaDataTable[953 + 1],
	},
	{
		&gDmaDataTable[955],
		&gDmaDataTable[955 + 1],
	},
	{
		&gDmaDataTable[957],
		&gDmaDataTable[957 + 1],
	},
};