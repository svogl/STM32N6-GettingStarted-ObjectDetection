/*
 * sdcard.c
 *
 *  Created on: Nov 16, 2025
 *      Author: Simon Vogl <simon@voxel.at>
 */
#include <string.h>

#include "sdcard.h"
#include "ff.h"
#include "diskio.h"   /* For disk_initialize() */
#include "sd_diskio.h" /* sd driver */
#include "stm32n6570_discovery_sd.h"

#define SPEED_TEST 0

#if SPEED_TEST

const int size=1024;
const int count=1024; //->1Mb

// put in psram; remove first attribute to have this in internal RAM.
__attribute__ ((section (".psram_bss")))
__attribute__ ((aligned (32)))
unsigned short speed_test_buffer[1024];

#endif

//static int dis=0;

/* Global variables --------------------------------------------------------*/
FATFS SDFatFS;                /* File system object for the SD card */
FIL file;                     /* File object */
extern void Error_Handler();

static int sdcard_hw_init() {

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
    	printf("SD SUBSYSTEM INIT FAILED!\n");
        Error_Handler();     /* SD init failed */
    }

    /* ---------------------------------------------------------------------*/
    /* 2. Mount the file system                                              */
    /* ---------------------------------------------------------------------*/
    FRESULT res = f_mount(&SDFatFS, "0:", 1); /* 1 = mount immediately */
    if (res != FR_OK) {
    	printf("MOUNT FAILED %d\n", res);
        Error_Handler();     /* Mount failed */
        return 1;
    }

    /* ---------------------------------------------------------------------*/
    /* 3. Create a file, write a string, close the file                     */
    /* ---------------------------------------------------------------------*/

    UINT written;
#if SPEED_TEST
    {
    	// header struct borrowed from cf. https://gist.github.com/Jon-Schneider/8b7c53d27a7a13346a643dac9c19d34f
    	typedef struct wav_header {
    	    // RIFF Header
    	    char riff_header[4]; // Contains "RIFF"
    	    int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    	    char wave_header[4]; // Contains "WAVE"

    	    // Format Header
    	    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    	    int fmt_chunk_size; // Should be 16 for PCM
    	    short audio_format; // Should be 1 for PCM. 3 for IEEE Float
    	    short num_channels;
    	    int sample_rate;
    	    int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    	    short sample_alignment; // num_channels * Bytes Per Sample
    	    short bit_depth; // Number of bits per sample

    	    // Data
    	    char data_header[4]; // Contains "data"
    	    char data_bytes[0]; // Number of bytes in data. Number of samples * num_channels * sample byte size
    	    // uint8_t bytes[]; // Remainder of wave file is bytes
    	} wav_header;

    	uint32_t ts[2] = { 0 };

    	wav_header wav = {
			.riff_header = "RIFF",
			.wav_size = sizeof(wav_header) - 8 +  size*count*2,
			.wave_header = "WAVE",
			.fmt_header = "fmt ",
			.fmt_chunk_size = 16,
			.audio_format = 1,
			.num_channels = 1,
			.sample_rate = 22000,
			.byte_rate = wav.sample_rate * 1 * 2,
			.bit_depth = 16,
			.data_header="data",
			.data_bytes = size*count*2,
    	};
        res = f_open(&file, "dummy.wav", FA_WRITE | FA_CREATE_ALWAYS);
        if (res != FR_OK) {
        	printf("FOPEN FAILED! %d\n", res);
        	return 0;
        }

		printf("WRITE WAV header: %d bytes\n", sizeof(wav_header));
        res = f_write(&file, (char*)&wav, sizeof(wav_header), &written);
        if (res != FR_OK || written != sizeof(wav_header)) {
			printf("FWRITE FAILED! %d %d\n", res, written);
			return 0;
		}
        // create some ugly saw-tooth pattern..:
		for (unsigned i=0;i<count;i++) {
			speed_test_buffer[i] = i;
		}

		ts[0] = HAL_GetTick();

		// write count times:
		for (unsigned i=0;i<count;i++) {
	        res = f_write(&file, speed_test_buffer, sizeof(speed_test_buffer), &written);
	        if (res != FR_OK || written != sizeof(speed_test_buffer)) {
				printf("WRITE FAILED! %d %d\n", res, written);
				return 0;
			}
		}
		ts[1] = HAL_GetTick();
		int ms = ts[1] - ts[0];
		printf("WRITE done: %ul ticks  %ld bytes/s \n", ms, (1000*count*sizeof(speed_test_buffer))/ms );
    }
#else
    const char *msg = "Hello FatFs!\n";
    res = f_open(&file, "test.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
    	printf("Could not open test file!\n");
//        Error_Handler();
    }
    res = f_write(&file, msg, strlen(msg), &written);
    if (res != FR_OK || written != strlen(msg)) {
    	printf("Could not write to test file!\n");
//        Error_Handler();
    }
#endif

    f_close(&file);

	return 0;
}



void SDMMC2_IRQHandler(void)
{
  BSP_SD_IRQHandler(0);
}
