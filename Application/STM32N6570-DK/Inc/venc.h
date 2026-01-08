#ifndef _VENC_H_
#define _VENC_H_

//// VENC API:::

// init once, set up HAL & hardware
extern int venc_init(void);

// prepare the encoder
extern int encoder_prepare(uint32_t width, uint32_t height, uint32_t * output_buffer);

// encode one frame
extern int encode_frame(void);

// finish outputing mp4 data.
extern int encoder_end(void);

//// file system API, called by ecnode* :::

/* change save stream and read frame for preferred in/output */
extern int save_stream(uint32_t offset, uint32_t * buf, size_t size);
extern int erase_enc_output(void);

extern uint32_t img_addr;
extern volatile uint8_t buf_index_changed;

#endif
