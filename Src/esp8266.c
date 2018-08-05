#include <string.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

#define UART_SIZE 1024
static uint8_t uart_buf[UART_SIZE];
static uint16_t uart_len = 0;
static uint16_t uart_read = 0;
static uint16_t uart_write = 0;

extern UART_HandleTypeDef huart2;
extern void send_debug(const char *str);
extern char debug[256];

void UART_IRQHandler(UART_HandleTypeDef *huart)
{
  uint32_t isrflags = huart->Instance->SR;

  if (isrflags & USART_SR_RXNE) {
    *(uart_buf + uart_write) = huart->Instance->DR;
    uart_write = (uart_write + 1) & (UART_SIZE - 1);
    ++uart_len;
    return;
  }

  /* noise error */
  if (isrflags & USART_SR_NE) {
    huart->ErrorCode |= HAL_UART_ERROR_NE;
  }

  /* frame error */
  if (isrflags & USART_SR_FE) {
    huart->ErrorCode |= HAL_UART_ERROR_FE;
  }

  /* over-run */
  if (isrflags & USART_SR_ORE) { 
    huart->ErrorCode |= HAL_UART_ERROR_ORE;
  }
}

uint16_t uart_get(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  uint16_t count;
  uint16_t got = 0;
	
  if (uart_len) {
    count = MIN(uart_len, Size);
    memcpy(pData, uart_buf + uart_read, count);
    uart_read = (uart_read + count) & (UART_SIZE - 1);
    __HAL_UART_DISABLE_IT(huart, UART_IT_RXNE);
    uart_len -= count;
    __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
    Size -= count;
    got += count;
  }
	
  if (Size == 0 || Timeout == 0)
    return got;

  uint32_t tickstart = HAL_GetTick();
  while ((HAL_GetTick() - tickstart) < Timeout) {
    if (uart_len) {
      count = MIN(uart_len, Size);
      memcpy(pData + got, uart_buf + uart_read, count);
      uart_read = (uart_read + count) & (UART_SIZE - 1);
      __HAL_UART_DISABLE_IT(huart, UART_IT_RXNE);
      uart_len -= count;
      __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
      Size -= count;
      got += count;
      if (Size == 0) break;
    }
  }
	
  return got;
}

uint16_t UART_Receive_Until(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout,
			    const char* until)
{
  uint32_t tickstart = 0;
  uint16_t count = 0;
  uint16_t left = Size;
  uint8_t data;
  uint8_t until_len = 0;
  if (until)
    until_len = strlen(until);
  uint8_t* cmp = pData - until_len;
	
  tickstart = HAL_GetTick();
  while (left && (HAL_GetTick() - tickstart) < Timeout) {
    if (uart_get(huart, &data, 1, 0) == 1) {
      --left;
      ++count;
      *pData++ = data;
      ++cmp;
      if (until_len && count >= until_len && *cmp == *until &&
	  memcmp(until, cmp, until_len) == 0) {
	break;
      }
    }
  }
  return count;
}

int UART_Receive_ParseInt(UART_HandleTypeDef *huart, uint32_t Timeout)
{
  uint32_t tickstart = 0;
  uint8_t data;
  int out = 0;
  int state = 0;
	
  tickstart = HAL_GetTick();
  while ((HAL_GetTick() - tickstart) < Timeout) {
    if (uart_get(huart, &data, 1, 0) == 1) {
      if (state == 0 && data >= '0' && data <= '9')
	state = 1;
      if (state == 1) {
	if (data < '0' || data > '9')
	  break;
	out *= 10;
	out += data - '0';
      }
    }
  }
  return out;
}

uint16_t UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
  return UART_Receive_Until(huart, pData, Size, Timeout, NULL);
}

void send_wifi(const char *str)
{
  HAL_UART_Transmit(&huart2, (unsigned char*)str, strlen(str), 1000);
}

void wifi_command(const char *str, const char *until)
{
  char response[256];
  send_wifi(str);
  sprintf(debug, "send wifi %s\r\n", str);
  send_debug(debug);
	
  int l = UART_Receive_Until(&huart2, (unsigned char*)response, 256, 5000, until);
  response[l] = 0;
  sprintf(debug, "got response %d %s\r\n", l, response);
  send_debug(debug);
}

void wifi_command_ok(const char *str)
{
  wifi_command(str, "OK\r\n");
}

void check_wifi_connection() {
  char data[512];
  if (uart_len) {
    uint16_t l = UART_Receive_Until(&huart2, (unsigned char*)data, 256, 10, "+IPD");
    data[l] = 0;
    if (strncmp(data + l - 4, "+IPD", 4) == 0) {
      int connection = UART_Receive_ParseInt(&huart2, 1000);
      int len = UART_Receive_ParseInt(&huart2, 1000);

      sprintf(data, "got connection %d\r\n", connection);
      send_debug(data);

      l = uart_get(&huart2, (unsigned char*)data, len, 10000);
			
      char message[32];
      static int xx = 0;
      sprintf(message, "<h1>HELLO %d<h1>", xx++);
			
      sprintf(data, "AT+CIPSEND=%d,%d\r\n", connection, strlen(message));
      wifi_command_ok(data);
						
      UART_Receive_Until(&huart2, (unsigned char*)data, 100, 5000, ">");
      send_wifi(message);

      UART_Receive_Until(&huart2, (unsigned char*)data, 100, 5000, "SEND OK");

      sprintf(data, "AT+CIPCLOSE=%d\r\n", connection);
      wifi_command_ok(data);
    }
  }
}

void wifi_setup() {
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_ERR);
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
	
  wifi_command("AT+RST\r\n", "ready\r\n"); 
  send_wifi("AT+UART_CUR=115200,8,1,0,0\r\n");
  huart2.Init.BaudRate = 115200;
  HAL_UART_Init(&huart2);

  wifi_command_ok("AT+GMR\r\n");
  wifi_command_ok("AT+CWMODE=3\r\n");
  wifi_command_ok("AT+CWJAP=\"hallonsaft\",\"apakattmus\"\r\n");
  wifi_command_ok("AT+CIFSR\r\n");
  wifi_command_ok("AT+CIPMUX=1\r\n");
  wifi_command_ok("AT+CIPSERVER=1,80\r\n");
}
