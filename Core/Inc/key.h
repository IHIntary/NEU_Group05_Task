/* USER CODE BEGIN Header */
#ifndef __KEY_H
#define __KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define KEY0_PRES  1U
#define KEY1_PRES  2U
#define KEY2_PRES  3U
#define WKUP_PRES  4U
/* USER CODE END Private defines */

/* USER CODE BEGIN Prototypes */
uint8_t key_scan(uint8_t mode);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __KEY_H */
/* USER CODE END Header */
