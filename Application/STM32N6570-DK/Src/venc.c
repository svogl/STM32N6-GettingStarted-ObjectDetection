/*
 * venc.c
 *
 *  Created on: Nov 13, 2025
 *      Author: simon
 */

// TODO (sv): extract SD functionality into separate file

/* Includes ------------------------------------------------------------------*/

#include "stdio.h"

#include "main.h"
#include "venc.h"

#include "ewl.h"
#include "h264encapi.h"
#include "stm32n6xx_hal.h"
#include "stm32n6xx_ll_venc.h"
#include "stm32n6570_discovery.h"
#include "stm32n6570_discovery_sd.h"

/** @addtogroup Templates
* @{
*/

/** @addtogroup HAL
* @{
*/

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define FRAMERATE 30
/* number of frames to film and encode */
#define VIDEO_FRAME_NB 10
#define USE_SD_AS_OUTPUT 0

/* Private macro -------------------------------------------------------------*/

/* enable trace if possible */
#if USE_COM_LOG || defined(TERMINAL_IO)
#define TRACE_MAIN(...) printf(__VA_ARGS__)
#else
#define TRACE_MAIN(...)
#endif /* USE_COM_LOG || defined(TERMINAL_IO) */

/* Private variables ---------------------------------------------------------*/

uint16_t * pipe_buffer[2];
volatile uint8_t buf_index_changed = 0;
H264EncIn encIn= {0};
H264EncOut encOut= {0};
H264EncInst encoder= {0};
H264EncConfig cfg= {0};
uint32_t output_size = 0;
uint32_t img_addr = 0;

EWLLinearMem_t outbuf;
static int frame_nb = 0;
uint32_t output_buffer[800*480/8] __NON_CACHEABLE __attribute__((aligned(8)));

#if USE_SD_AS_OUTPUT
uint32_t sd_buf1[0x10000] __NON_CACHEABLE;
uint32_t sd_buf2[0x10000] __NON_CACHEABLE;
uint32_t * curr_buf = sd_buf1;
size_t buf_index = 0;
size_t SD_index = 0;
#endif /* USE_SD_AS_OUTPUT */


/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/

/**
* @brief  Save an encoded buffer fragment at the given offset.
* @param  offset
* @param  buf  pointer to the buffer to save
* @param  size  size (in bytes) of the buffer to save
* @retval err error code. 0 On success.
*/
int save_stream(uint32_t offset, uint32_t * buf, size_t size){
  int err = 0;
#if USE_SD_AS_OUTPUT
  for(int i = 0; i<size/4; i++){
    curr_buf[buf_index] = buf[i];
    buf_index++;
    /* upload to sd every 512 blocks to limit the impact of access latency */
    if(buf_index >= 0x10000){
      if(BSP_SD_WriteBlocks_DMA(0, curr_buf, SD_index, 512)!= BSP_ERROR_NONE){
        err = -1;
      }
      SD_index+=512;
      /* swap buffers */
      buf_index = 0;
      curr_buf = (curr_buf == sd_buf1)?sd_buf2:sd_buf1;
    }
  }
#endif
  return err;
}

int flush_out_buffer(void){
#if USE_SD_AS_OUTPUT
  if(BSP_SD_WriteBlocks(0, (uint32_t *) curr_buf, SD_index, 512)!= BSP_ERROR_NONE){
        return -1;
      }
#endif
  return 0;
}
/**
* @brief  erases data in output medium
* @retval err error code. 0 On success.
*/
int erase_enc_output(void){
  /* flash must be erased by blocks of 64 bytes */
#if USE_SD_AS_OUTPUT
  /* only 100 blocks are erased because erasing the 64M would take forever */
  if (BSP_SD_Erase(0, 0, 1943552) != BSP_ERROR_NONE)
  {
    TRACE_MAIN("failed to erase external flash block nb \n");
    return -1;
  }
#endif
  return 0;
}


/**
* @brief  Main program
* @param  None
* @retval None
*/
int venc_init(void)
{
  /* erase output*/
  TRACE_MAIN("erasing flash output blocks\n");
  erase_enc_output();
  TRACE_MAIN("Done erasing output flash blocks\n");

#if USE_SD_AS_OUTPUT
  /* wait for erase operation to be done */
  while(BSP_SD_GetCardState(0) != SD_TRANSFER_OK);
#endif
  /* initialize VENC */
  LL_VENC_Init();

  /* initialize encoder software for camera feed encoding */
  encoder_prepare(800,480,output_buffer);

  return 0;
}


int encoder_prepare(uint32_t width, uint32_t height, uint32_t * output_buffer)
{
  H264EncRet ret;

  H264EncPreProcessingCfg preproc_cfg = {0};

  /* software workaround for Linemult triggering VENC interrupt. Make it happen as little as possible */
  MODIFY_REG(DCMIPP->P1PPCR, DCMIPP_P1PPCR_LINEMULT_Msk,DCMIPP_MULTILINE_128_LINES);

  frame_nb = 0;
  /* Step 1: Initialize an encoder instance */
  /* set config to 1 ref frame */
  cfg.refFrameAmount = 1;
  /* 30 fps frame rate */
  cfg.frameRateDenom = 1;
  cfg.frameRateNum = FRAMERATE;
  /* Image resolution */
  cfg.width = width;
  cfg.height = height;
  /* Stream type */
  cfg.streamType = H264ENC_BYTE_STREAM;

  /* encoding level*/
  /*See API guide for level depending on resolution and framerate*/
  cfg.level = H264ENC_LEVEL_2_2;
  cfg.svctLevel = 0;

  /* Output buffer size */
  outbuf.size = cfg.width * cfg.height;

  ret = H264EncInit(&cfg, &encoder);
  if (ret != H264ENC_OK)
  {
    TRACE_MAIN("error initializing encoder %d\n", ret);
    return -1;
  }

  /* set format conversion for preprocessing */
  ret = H264EncGetPreProcessing(encoder, &preproc_cfg);
  if(ret != H264ENC_OK){
    TRACE_MAIN("error getting preproc data\n");
    return -1;
  }
  preproc_cfg.inputType = H264ENC_RGB565;
  ret = H264EncSetPreProcessing(encoder, &preproc_cfg);
  if(ret != H264ENC_OK){
    TRACE_MAIN("error setting preproc data\n");
    return -1;
  }

  /*assign buffers to input structure */
  encIn.pOutBuf = output_buffer;
  encIn.busOutBuf = (uint32_t) output_buffer;
  encIn.outBufSize = width * height/2;

  /* create stream */
  ret = H264EncStrmStart(encoder, &encIn, &encOut);
  if (ret != H264ENC_OK)
  {
    TRACE_MAIN("error starting stream\n");
    return -1;
  }

  /* save the stream header */
  if (save_stream(output_size, encIn.pOutBuf,  encOut.streamSize))
  {
    TRACE_MAIN("error saving stream\n");
    return -1;
  }
  TRACE_MAIN("stream started. saved %d bytes\n", encOut.streamSize);
  output_size+= encOut.streamSize;
  return 0;
}


int encode_frame(){
  int ret = H264ENC_FRAME_READY;
  if(!img_addr){
    TRACE_MAIN("Error : NULL image address");
    return -1;
  }
  if (!frame_nb)
  {
    /* if frame is the first : set as intra coded */
    encIn.timeIncrement = 0;
    encIn.codingType = H264ENC_INTRA_FRAME;
  }
  else
  {
    /* if there was a frame previously, set as predicted */
    encIn.timeIncrement = 1;
    encIn.codingType = H264ENC_PREDICTED_FRAME;
  }
  encIn.ipf = H264ENC_REFERENCE_AND_REFRESH;
  encIn.ltrf = H264ENC_REFERENCE;
  /* set input buffers to structures */
  encIn.busLuma = img_addr;
  ret = H264EncStrmEncode(encoder, &encIn, &encOut, NULL, NULL, NULL);
  switch (ret)
  {
  case H264ENC_FRAME_READY:
    /*save stream */
    if (save_stream(output_size, encIn.pOutBuf,  encOut.streamSize))
    {
      TRACE_MAIN("error saving stream frame %d\n", frame_nb);
      return -1;
    }
    output_size += encOut.streamSize;
    break;
  case H264ENC_SYSTEM_ERROR:
    TRACE_MAIN("fatal error while encoding\n");
    break;
  default:
    TRACE_MAIN("error encoding frame %d : %d\n", frame_nb, ret);
    break;
  }
  frame_nb++;
  return 0;
}


int encoder_end(void){
  int ret = H264EncStrmEnd(encoder, &encIn, &encOut);
  TRACE_MAIN("done encoding %d frames. size : %d\n",frame_nb ,output_size);
  if (ret != H264ENC_OK)
  {
    return -1;
  }
  else
  {
    /* save stream tail */
    if (save_stream(output_size, encIn.pOutBuf,  encOut.streamSize))
    {
      TRACE_MAIN("error saving stream\n");
      return -1;
    }
    output_size+=encOut.streamSize;
  }

  return 0;
}


