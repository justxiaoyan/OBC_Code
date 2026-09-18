#ifndef __VS_UART_H__
#define __VS_UART_H__

int vs_uart_init(void);
void vs_uart_setbrt(unsigned int baudrate);
unsigned int vs_uart_getbrg(void);
void vs_uart_putc(const char ch);
int vs_uart_puts(const char *str, int num);
int vs_uart_getc(void);
int vs_uart_tstc(void);
int vs_printf(const char *format, ...);
#endif /* __VS_UART_H__ */
