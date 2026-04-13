#include "crsf.h"

// Define the static member variable
DMA_BUFFER uint8_t CRSF::rxData[CRSF_MAX_FRAME_SIZE];

void CRSF::init(UART_HandleTypeDef* huart) {
  huart_ = huart;

  // Initialize the rxData buffer to zero
  for (uint16_t i = 0; i < sizeof(rxData); i++) {
	  rxData[i] = 0;
  }

  // Start DMA reception
  HAL_UARTEx_ReceiveToIdle_DMA(huart_, rxData, sizeof(rxData));
}


void CRSF::processReceivedData(UART_HandleTypeDef *huart, uint16_t Size) {
  // Check if the callback is for our UART
  if (huart == huart_) {
    // Indicate that data has been received (for debugging)
    printf("Received %d bytes: \n", Size);

    // Dump the received data into the buffer for processing, incrementing frame index and wrapping it
    for (uint16_t i = 0; i < Size && i < CRSF_MAX_FRAME_SIZE; i++) {
      buffer[frameIndex][i] = rxData[i];
    }
    frameIndex = (frameIndex + 1) % CRSF_MAX_FRAME_COUNT; // Move to the next frame index, wrap around if needed

    // Print the frame index for debugging
    printf("Frame Index: %d\n", frameIndex);
  }

  // Restart the DMA reception for the next data
  HAL_UARTEx_ReceiveToIdle_DMA(huart, rxData, sizeof(rxData));
}

void CRSF::overrunErrorHandler(UART_HandleTypeDef *huart) {
    if (huart == huart_) {
        if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
            // Clear ORE flag by reading SR/ISR and DR/RDR
            __HAL_UART_CLEAR_OREFLAG(huart);
            // Restart DMA reception
            HAL_UARTEx_ReceiveToIdle_DMA(huart, rxData, sizeof(rxData));
        }
    }
}
