#ifndef CRSF_H
#define CRSF_H

#include "main.h"

#if defined( __ICCARM__ )
  #define DMA_BUFFER \
      _Pragma("location=\".dma_buffer\"")
#else
  #define DMA_BUFFER \
      __attribute__((section(".dma_buffer")))
#endif

#define CRSF_MAX_FRAME_SIZE 64
#define CRSF_MAX_FRAME_COUNT 16

class CRSF {
  public:
    void init(
      UART_HandleTypeDef* huart
    );
    
    void processReceivedData(UART_HandleTypeDef *huart, uint16_t Size);
    void overrunErrorHandler(UART_HandleTypeDef *huart);
    
    void printData(void) {
      printf("Received data printing: ");
      for (uint16_t i = 0; i < sizeof(rxData); i++) {
        printf("%02X ", rxData[i]);
      }
      printf("\r\n");
    }

    DMA_BUFFER static uint8_t rxData[CRSF_MAX_FRAME_SIZE];
  
  private:
    UART_HandleTypeDef* huart_;

    uint8_t buffer[CRSF_MAX_FRAME_COUNT][CRSF_MAX_FRAME_SIZE]; // Buffer for 16 frames of 64 bytes each
    uint16_t frameIndex = 0; // Index to track the current frame being processed
};

#endif /* CRSF_H */
