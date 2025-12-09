/*
 * sdcard.c
 *
 *  Created on: Nov 16, 2025
 *      Author: simon
 */
#include <string.h>

#include "sdcard.h"
#include "ff.h"
#include "diskio.h"   /* For disk_initialize() */
#include "sd_diskio.h" /* sd driver */
#include "stm32n6570_discovery_sd.h"

static int dis=0;

/* Global variables --------------------------------------------------------*/
FATFS SDFatFS;                /* File system object for the SD card */
FIL file;                     /* File object */
extern void Error_Handler();

int sdcard_hw_init() {

	extern int32_t BSP_SD_Init(uint32_t Instance);
	if (BSP_SD_Init(0) != 0) {
		printf("error initializing NOR flash\n");
		Error_Handler();
	}
	BSP_SD_CardInfo card_info;
	BSP_SD_GetCardInfo(0, &card_info);
	printf(
			"SD card info : \nblock Nbr : %d\nblock size : %d\ncard speed : %d\n",
			card_info.BlockNbr, card_info.BlockSize, card_info.CardSpeed);

}

int sdcard_init() {
	char disk_path[8];

	sdcard_hw_init();

	FATFS_LinkDriver(&SD_Driver, disk_path);

    /* ---------------------------------------------------------------------*/
    /* 1. Initialize the SD disk (SDIO driver)                               */
    /* ---------------------------------------------------------------------*/
    if (disk_initialize(0) != 0) {
        Error_Handler();     /* SD init failed */
    }

    /* ---------------------------------------------------------------------*/
    /* 2. Mount the file system                                              */
    /* ---------------------------------------------------------------------*/
    FRESULT res = f_mount(&SDFatFS, "", 1); /* 1 = mount immediately */
    if (res != FR_OK) {
        Error_Handler();     /* Mount failed */
    }

    /* ---------------------------------------------------------------------*/
    /* 3. Create a file, write a string, close the file                     */
    /* ---------------------------------------------------------------------*/
    const char *msg = "Hello FatFs!\n";
    res = f_open(&file, "test.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
    	printf("Could not open test file!\n");
//        Error_Handler();
    }
    UINT written;
    res = f_write(&file, msg, strlen(msg), &written);
    if (res != FR_OK || written != strlen(msg)) {
    	printf("Could not write to test file!\n");
//        Error_Handler();
    }
    f_close(&file);


	return 0;
}



void SDMMC2_IRQHandler(void)
{
  BSP_SD_IRQHandler(0);
}
