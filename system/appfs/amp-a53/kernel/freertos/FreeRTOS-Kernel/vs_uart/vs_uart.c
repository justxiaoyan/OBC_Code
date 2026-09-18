#if 0
#include <vs_regops.h>
#include <vs_errno.h>
#include <vs_type.h>
#include <vs_mem_map.h>
#include <vs_plat_freq.h>
#include <vs_yaml_parser.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
//#include <hw_types.h>

#define PL011_DEFAULT_BAUDRATE	115200

#define PL011_REG_DR_OFFSET		(0x00) /* Data register */
#define PL011_REG_ECR_OFFSET		(0x04) /* Error clear register (Write) */
#define PL011_REG_FR_OFFSET		(0x18) /* Flag register (Read only) */
#define PL011_REG_RLCR_OFFSET		(0x1c) /* Receive line control register */
#define PL011_REG_IBRD_OFFSET		(0x24) /* Integer baud rate register */
#define PL011_REG_FBRD_OFFSET		(0x28) /* Fractional baud rate register */
#define PL011_REG_LCRH_OFFSET		(0x2C) /* Line control register */
#define PL011_REG_CR_OFFSET		(0x30) /* Control register */
#define PL011_REG_IFLS_OFFSET		(0x34) /* Interrupt FIFO Level Select Register */
#define PL011_REG_IMSC_OFFSET		(0x38) /* Interrupt Mask Set/Clear Register */
#define PL011_REG_ICR_OFFSET		(0x44) /* Interrupt Clear Register */

#define PL011_DR_DATA_MSK	0xFF

#define PL011_FR_TXFE		0x80
#define PL011_FR_RXFF		0x40
#define PL011_FR_TXFF		0x20
#define PL011_FR_RXFE		0x10
#define PL011_FR_BUSY		0x08
#define PL011_FR_TMSK		(PL011_FR_TXFF + PL011_FR_BUSY)

#define PL011_CR_RXE		(1 << 9)
#define PL011_CR_TXE		(1 << 8)
#define PL011_CR_UARTEN		(1 << 0)

#define PL011_LCRH_SPS		(1 << 7)
#define PL011_LCRH_WLEN_8	(3 << 5)
#define PL011_LCRH_FEN		(1 << 4)
#define PL011_LCRH_STP2		(1 << 3)
#define PL011_LCRH_EPS		(1 << 2)
#define PL011_LCRH_PEN		(1 << 1)
#define PL011_LCRH_BRK		(1 << 0)

#define PL011_IMSC_OEIM		(1 << 10)
#define PL011_IMSC_BEIM		(1 << 9)
#define PL011_IMSC_PEIM		(1 << 8)
#define PL011_IMSC_FEIM		(1 << 7)
#define PL011_IMSC_RTIM		(1 << 6)
#define PL011_IMSC_TXIM		(1 << 5)
#define PL011_IMSC_RXIM		(1 << 4)
#define PL011_IMSC_DSRMIM	(1 << 3)
#define PL011_IMSC_DCDMIM	(1 << 2)
#define PL011_IMSC_CTSMIM	(1 << 1)
#define PL011_IMSC_RIMIM	(1 << 0)
#define PL011_IMSC_ALL		0x7FF

#define UART9_BASE_ADDR     (0xC09A000)
#define IOMUX_BASE_ADDR     (0x210e000)
#define IOMUX_UART9_TX_OFFSET   (0x414)
#define IOMUX_UART9_RX_OFFSET   (0X418)

#define  PL011_CLK (100000000)

static uint32_t uartBaseAddr = 0U;

/* 如果还是找不到，添加备用实现 */
#ifndef HAVE_WRITEL
#define HAVE_WRITEL
static inline void writel(uint32_t val, volatile void *addr)
{
    *(volatile uint32_t *)addr = val;
    __asm__ volatile("dsb sy" : : : "memory");
}
#endif

#ifndef HAVE_READL
#define HAVE_READL
static inline uint32_t readl(volatile void *addr)
{
    uint32_t val = *(volatile uint32_t *)addr;
    __asm__ volatile("dsb sy" : : : "memory");
    return val;
}
#endif

#ifndef HAVE_CLRBITS
#define HAVE_CLRBITS
static inline void clrbits(uint32_t mask, volatile void *addr)
{
    uint32_t reg = readl(addr);
    reg &= ~mask;
    writel(reg, addr);
}
#endif

static void pl011_setbrt(unsigned int baudrate)
{
	unsigned int temp;
	unsigned int div;
	unsigned int remainder;
	unsigned int frac;

	/*
	 * Set baud rate
	 *
	 * IBRD = UART_CLK / (16 * BAUD_RATE)
	 * FBRD = RND((64 * MOD(UART_CLK,(16 * BAUD_RATE))) / (16 * BAUD_RATE))
	 */
	temp = 16 * baudrate;
	div = PL011_CLK / temp;
	remainder = PL011_CLK % temp;
	temp = (8 * remainder) / baudrate;
	frac = (temp >> 1) + (temp & 1);

	writel(div, (void *)(uartBaseAddr + PL011_REG_IBRD_OFFSET));
	writel(frac, (void *)(uartBaseAddr + PL011_REG_FBRD_OFFSET));
}

static void pl011_enable(void)
{
	writel(PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE, (void *)(uartBaseAddr + PL011_REG_CR_OFFSET));
}

static void pl011_disable(void)
{
	writel(0, (void *)(uartBaseAddr + PL011_REG_CR_OFFSET));
}

static void pl011_int_all_disable(void)
{
	writel(PL011_IMSC_ALL, (void *)(uartBaseAddr + PL011_REG_IMSC_OFFSET));
}

static void pl011_int_all_clear(void)
{
	writel(PL011_IMSC_ALL, (void *)(uartBaseAddr + PL011_REG_ICR_OFFSET));
}

static void pl011_int_disable(unsigned int mask)
{
	writel(mask,  (void *)(uartBaseAddr + PL011_REG_IMSC_OFFSET));
}

static void pl011_int_enable(unsigned int mask)
{
	clrbits(mask,  (void *)(uartBaseAddr + PL011_REG_IMSC_OFFSET));
}

static void pl011_clear_err(void)
{
	writel(0xFFFFFFFF, (void *)(uartBaseAddr + PL011_REG_ECR_OFFSET));
}

static int pl011_putc(char c)
{
	unsigned int us = 50000;

	/* Wait until there is space in the FIFO */
	while (readl((void *)(uartBaseAddr + PL011_REG_FR_OFFSET)) & PL011_FR_TXFF) {
		us--;
		if (us == 0)
			break;
	}

	/* Send the character */
	writel(c, (void *)(uartBaseAddr + PL011_REG_DR_OFFSET));

	return 0;
}

static int pl011_tstc(void)
{
	return !(readl((void *)(uartBaseAddr + PL011_REG_FR_OFFSET)) & PL011_FR_RXFE);
}

static int pl011_getc(void)
{
	unsigned int data;

	/* Wait until there is data in the FIFO */
	while (!pl011_tstc()) {
		;
	}

	data = readl((void *)(uartBaseAddr + PL011_REG_DR_OFFSET));
	/* Check for an error flag */
	if (data & 0xFFFFFF00) {
		pl011_clear_err();
		data = 0x8000000 | (data & PL011_DR_DATA_MSK);
	}

	return (int)data;
}

static void (*pl011_handler)() = NULL;

void serial_intr_handler(void)
{
	if (pl011_handler != NULL)
		pl011_handler();
}

static void pl011_line_ctrl(void)
{
	/*
	 * Internal update of baud rate register require line
	 * control register write
	 */
	writel(PL011_LCRH_WLEN_8 | PL011_LCRH_FEN, (void *)(uartBaseAddr + PL011_REG_LCRH_OFFSET));
}

static void pl011_set_fifo(void)
{
	/*
	 * bit5:3
	 * b000 = Receive FIFO becomes 1/8 full
	 * b001 = Receive FIFO becomes 1/4 full
	 * b010 = Receive FIFO becomes 1/2 full
	 * b011 = Receive FIFO becomes 3/4 full
	 * b100 = Receive FIFO becomes 7/8 full
	 * bit2:0
	 * b000 = Transmit FIFO becomes 1/8 full
	 * b001 = Transmit FIFO becomes 1/4 full
	 * b010 = Transmit FIFO becomes 1/2 full
	 * b011 = Transmit FIFO becomes 3/4 full
	 * b100 = Transmit FIFO becomes 7/8 full
	 */
	writel(0x10, (void *)(uartBaseAddr + PL011_REG_IFLS_OFFSET));
}

static void uart9_iomux_config()
{
	uint32_t val = readl((void *)(IOMUX_BASE_ADDR + IOMUX_UART9_TX_OFFSET));
	val &= (~0x7);
	val |= 3; // UART9_TXD
	writel(val, (void *)(IOMUX_BASE_ADDR + IOMUX_UART9_TX_OFFSET));

	val = readl((void *)(IOMUX_BASE_ADDR + IOMUX_UART9_RX_OFFSET));
	val &= (~0x7);
	val |= 3;
	writel(val, (void *)(IOMUX_BASE_ADDR + IOMUX_UART9_RX_OFFSET));
}

static void vs_uart_get_base_addr(uint32_t *addr)
{
	*addr = UART9_BASE_ADDR;		// set uart addr
}

void vs_uart_init(void)
{
	vs_uart_get_base_addr(&uartBaseAddr);
	uart9_iomux_config();
	pl011_disable();
	pl011_clear_err();
	pl011_setbrt(PL011_DEFAULT_BAUDRATE);
	pl011_line_ctrl();
	pl011_int_all_disable();
	pl011_int_all_clear();
	pl011_set_fifo();
	pl011_enable();
}

void vs_uart_setbrt(unsigned int bdr)
{
	pl011_disable();
	pl011_setbrt(bdr);
	pl011_enable();
}

int vs_uart_putc(char c)
{
	pl011_putc(c);

	return 0;
}

int vs_uart_puts(const char *str, int num)
{
	int i;
	int ret;

	for (i = 0; i < num; i++) {
		ret = pl011_putc(str[i]);
		if (ret != 0)
			return ret;
	}

	return 0;
}

int vs_uart_getc(void)
{
	return pl011_getc();
}

void vs_uart_disable(void)
{
	pl011_disable();
}

int vs_uart_tstc(void)
{
	return pl011_tstc();
}

void vs_uart_rx_register(void (*handler)())
{
	pl011_handler = handler;
}

void vs_uart_rx_int_disable(void)
{
	pl011_int_disable(PL011_IMSC_RXIM);
}

void vs_uart_rx_int_enable(void)
{
	pl011_int_enable(PL011_IMSC_RXIM);
}




extern int vsnprintf(char *str, size_t size, const char *format, va_list ap);

int vs_printf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    int len;
    int ret;
    
    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len < 0) {
        return -1;
    }
    
    if (len >= (int)sizeof(buffer)) {
        return -2;
    }
    
    ret = vs_uart_puts(buffer, len);
    if (ret != 0) {
        return ret;
    }
    
    return len;
}
