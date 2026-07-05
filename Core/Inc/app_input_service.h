#ifndef APP_INPUT_SERVICE_H
#define APP_INPUT_SERVICE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void AppInputService_Init(void);
uint8_t AppInputService_ScanRemoteKey(void);
uint8_t AppInputService_PollKey2Pressed(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_INPUT_SERVICE_H */
