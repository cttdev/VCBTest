#include "crsf.h"

// Define the static member variable
DMA_BUFFER uint8_t CRSF::rxData[CRSF_MAX_FRAME_SIZE];

void CRSF::init(UART_HandleTypeDef* huart) {
  huart_ = huart;

  // Initialize the rxData buffer to zero
  for (uint16_t i = 0; i < sizeof(rxData); i++) {
	  rxData[i] = 0;
  }

  // Initialize the frame buffer to zero
  for (uint16_t i = 0; i < CRSF_MAX_FRAME_COUNT; i++) {
    for (uint16_t j = 0; j < CRSF_MAX_FRAME_SIZE; j++) {
      buffer[i][j] = 0;
    }
  }

  // Clear the buffer lengths
  for (uint16_t i = 0; i < CRSF_MAX_FRAME_COUNT; i++) {
    bufferLengths[i] = 0;
  }

  // Start DMA reception
  HAL_UARTEx_ReceiveToIdle_DMA(huart_, rxData, sizeof(rxData));
}

void CRSF::printLatestPacket(void) {
  printf("Latest packet data: ");
  // latest packet is at frameIndex - 1 (wrap around if needed)
  uint16_t latestIndex = (frameIndex == 0) ? (CRSF_MAX_FRAME_COUNT - 1) : (frameIndex - 1);
  for (uint16_t i = 0; i < CRSF_MAX_FRAME_SIZE; i++) {
    printf("%02X ", buffer[latestIndex][i]);
  }
  printf("\r\n");
}

void CRSF::parseRCChannel(uint8_t* data, uint16_t length) {
  // Tacket packet from RC data channel and put into a struct for use in the rest of the program
  // Check sync code (frist byte should be 0xC8)
  if (!(length > 0 && data[0] == 0xC8)) {
    printf("Invalid RC packet or sync code not found.\r\n");
    return;
  }
  
  // Check packet length
  if(data[1] != 0x18 || data[1] != length-2) {
    printf("Unexpected RC packet length: %d\r\n", length);
    return;
  }

  // Check frame type
  if(data[2] != 0x16) {
    printf("Unexpected RC packet type: %02X\r\n", data[2]);
    return;
  }

  // Reset channel data before parsing new values
  rcData = {0}; // Reset all channel values to zero

  // Parse channel data (the data is 16 11-bit channels that are packed into 22 bytes starting from data[3] LSB first)
  // Iterate through the 16 channels and extract the 11-bit values, we need to reverse the order of the bits as they are packed LSB first
  for (int i = 0; i < 16; i++) {
    // Bit 0 is the first bit of the channel data
    for (int bit = 0; bit < 11; bit++) {
      // Calculate the byte index and bit index in the data array
      int byteIndex = 3 + (i * 11 + bit) / 8; // Start from data[3]
      int bitIndex = (i * 11 + bit) % 8;

      // Extract the bit value and set it in the corresponding channel field
      uint8_t bitValue = (data[byteIndex] >> bitIndex) & 0x01; // Get the specific bit
      switch (i) {
        case 0: rcData.channel_01 |= (bitValue << bit); break;
        case 1: rcData.channel_02 |= (bitValue << bit); break;
        case 2: rcData.channel_03 |= (bitValue << bit); break;
        case 3: rcData.channel_04 |= (bitValue << bit); break;
        case 4: rcData.channel_05 |= (bitValue << bit); break;
        case 5: rcData.channel_06 |= (bitValue << bit); break;
        case 6: rcData.channel_07 |= (bitValue << bit); break;
        case 7: rcData.channel_08 |= (bitValue << bit); break;
        case 8: rcData.channel_09 |= (bitValue << bit); break;
        case 9: rcData.channel_10 |= (bitValue << bit); break;
        case 10: rcData.channel_11 |= (bitValue << bit); break;
        case 11: rcData.channel_12 |= (bitValue << bit); break;
        case 12: rcData.channel_13 |= (bitValue << bit); break;
        case 13: rcData.channel_14 |= (bitValue << bit); break;
        case 14: rcData.channel_15 |= (bitValue << bit); break;
        case 15: rcData.channel_16 |= (bitValue << bit); break;
      }
    }
  }

  // Print parsed channel data as unsigned integers for debugging
  printf("Parsed RC Channels:\r\n");
  printf("Channel 1: %u\r\n", rcData.channel_01);
  printf("Channel 2: %u\r\n", rcData.channel_02);
  printf("Channel 3: %u\r\n", rcData.channel_03);
  printf("Channel 4: %u\r\n", rcData.channel_04);
  printf("Channel 5: %u\r\n", rcData.channel_05);
  printf("Channel 6: %u\r\n", rcData.channel_06);
  printf("Channel 7: %u\r\n", rcData.channel_07);
  printf("Channel 8: %u\r\n", rcData.channel_08);
  printf("Channel 9: %u\r\n", rcData.channel_09);
  printf("Channel 10: %u\r\n", rcData.channel_10);
  printf("Channel 11: %u\r\n", rcData.channel_11);
  printf("Channel 12: %u\r\n", rcData.channel_12);
  printf("Channel 13: %u\r\n", rcData.channel_13);
  printf("Channel 14: %u\r\n", rcData.channel_14);
  printf("Channel 15: %u\r\n", rcData.channel_15);
  printf("Channel 16: %u\r\n", rcData.channel_16);
}

void CRSF::parseLatestPacket(void) {
  // Parse a packet that you already know is an RC data packet
  uint16_t latestIndex = (frameIndex == 0) ? (CRSF_MAX_FRAME_COUNT - 1) : (frameIndex - 1);

  // Check if the latest packet has data (first byte should be 0xC8 for RC data)
  if (buffer[latestIndex][0] == 0xC8) {
    parseRCChannel(buffer[latestIndex], bufferLengths[latestIndex]);
  } else {
    printf("Latest packet is not an RC data packet.\r\n");
  }
}

void CRSF::processReceivedData(UART_HandleTypeDef *huart, uint16_t Size) {
  // Check if the callback is for our UART
  if (huart == huart_) {
    // Indicate that data has been received (for debugging)
    // printf("Received %d bytes: \n", Size);
    
    // Clear the buffer entry for the current frame index before copying new data
    memset(buffer[frameIndex], 0, CRSF_MAX_FRAME_SIZE);

    // Dump the received data into the buffer for processing, incrementing frame index and wrapping it
    memcpy(buffer[frameIndex], rxData, (Size < CRSF_MAX_FRAME_SIZE) ? Size : CRSF_MAX_FRAME_SIZE);
    bufferLengths[frameIndex] = Size; // Store the actual length of the received frame
    frameIndex = (frameIndex + 1) % CRSF_MAX_FRAME_COUNT; // Move to the next frame index, wrap around if needed

    // Print the frame index for debugging
    // printf("Frame Index: %d\n", frameIndex);
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
