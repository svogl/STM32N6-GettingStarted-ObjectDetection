/**
  ******************************************************************************
  * @file    default.h
  * @author  STEdgeAI
  * @date    2026-01-08 20:02:54
  * @brief   Minimal description of the generated c-implemention of the network
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */
#ifndef LL_ATON_DEFAULT_H
#define LL_ATON_DEFAULT_H

/******************************************************************************/
#define LL_ATON_DEFAULT_C_MODEL_NAME        "default"
#define LL_ATON_DEFAULT_ORIGIN_MODEL_NAME   "st_yolo_x_nano_480_1_0_0_25_3_st_int8"

/************************** USER ALLOCATED IOs ********************************/
// No user allocated inputs
// No user allocated outputs

/************************** INPUTS ********************************************/
#define LL_ATON_DEFAULT_IN_NUM        (1)    // Total number of input buffers
// Input buffer 1 -- Input_0_out_0
#define LL_ATON_DEFAULT_IN_1_ALIGNMENT   (32)
#define LL_ATON_DEFAULT_IN_1_SIZE_BYTES  (691200)

/************************** OUTPUTS *******************************************/
#define LL_ATON_DEFAULT_OUT_NUM        (3)    // Total number of output buffers
// Output buffer 1 -- Transpose_782_out_0
#define LL_ATON_DEFAULT_OUT_1_ALIGNMENT   (32)
#define LL_ATON_DEFAULT_OUT_1_SIZE_BYTES  (4050)
// Output buffer 2 -- Transpose_886_out_0
#define LL_ATON_DEFAULT_OUT_2_ALIGNMENT   (32)
#define LL_ATON_DEFAULT_OUT_2_SIZE_BYTES  (64800)
// Output buffer 3 -- Transpose_834_out_0
#define LL_ATON_DEFAULT_OUT_3_ALIGNMENT   (32)
#define LL_ATON_DEFAULT_OUT_3_SIZE_BYTES  (16200)

#endif /* LL_ATON_DEFAULT_H */
