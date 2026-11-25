#ifndef __DEBUG_PRINTK_H
#define __DEBUG_PRINTK_H

extern int uart_debug_port;

void serial_init (void);
void serial_printf(const char* Format, ...);

#define debug(x...)	serial_printf(x)

#endif
