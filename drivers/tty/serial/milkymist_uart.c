/*
 * Copyright (C) 2011 Michael Walle <michael@walle.cc>
 * Copyright (C) 2009 Sebastien Bourdeauducq, Takeshi Matsuya
 * Copyright (C) 2006 Peter Korsgaard <jacmet@sunsite.dk>
 * Copyright (C) 2007 Secret Lab Technologies Ltd.
 *
 * based on uartlite.c
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial.h>
#include <linux/serialP.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/serial_core.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#define MILKYMIST_UART_MAJOR TTY_MAJOR
#define MILKYMIST_UART_MINOR 64

#define MILKYMIST_NR_UARTS CONFIG_SERIAL_MILKYMIST_NR_UARTS

#define UART_RXTX  0x00
#define UART_DIV   0x04
#define UART_STAT  0x08
#define UART_CTRL  0x0c
#define UART_DEBUG 0x10

#define UART_STAT_THRE    0x01
#define UART_STAT_RX_EVT  0x02
#define UART_STAT_TX_EVT  0x04

#define UART_CTRL_RX_INT  0x01
#define UART_CTRL_TX_INT  0x02
#define UART_CTRL_THRU    0x04

#define UART_DEBUG_BREAK  0x01

static struct uart_port milkymist_uart_ports[MILKYMIST_NR_UARTS];

static void milkymist_uart_tx_char(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;

	if (port->x_char) {
		iowrite32be(port->x_char, port->membase + UART_RXTX);
		port->x_char = 0;
		port->icount.tx++;
		return;
	}

	if (uart_circ_empty(xmit) || uart_tx_stopped(port))
		return;

	iowrite32be(xmit->buf[xmit->tail], port->membase + UART_RXTX);
	xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE-1);
	port->icount.tx++;

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);
}

static void milkymist_uart_rx_char(struct uart_port *port)
{
	struct tty_struct *tty = port->state->port.tty;
	unsigned char ch;

	ch = ioread32be(port->membase + UART_RXTX) & 0xff;
	port->icount.rx++;

	if (uart_handle_sysrq_char(port, ch))
		goto ignore_char;

	uart_insert_char(port, 0, 0, ch, TTY_NORMAL);

ignore_char:
	tty_flip_buffer_push(tty);
}

static irqreturn_t milkymist_uart_isr(int irq, void *data)
{
	struct uart_port *port = data;
	u8 stat;

	spin_lock(&port->lock);

	/* read and ack events */
	stat = ioread32be(port->membase + UART_STAT) & 0xff;
	iowrite32be(stat, port->membase + UART_STAT);

	if (stat & UART_STAT_RX_EVT)
		milkymist_uart_rx_char(port);
	if (stat & UART_STAT_TX_EVT)
		milkymist_uart_tx_char(port);

	spin_unlock(&port->lock);

	return IRQ_HANDLED;
}

static void milkymist_uart_start_tx(struct uart_port *port)
{
	milkymist_uart_tx_char(port);
}

static unsigned int milkymist_uart_tx_empty(struct uart_port *port)
{
	/* XXX return tx_pending */
	return TIOCSER_TEMT;
}

static void milkymist_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
}

static unsigned int milkymist_uart_get_mctrl(struct uart_port *port)
{
	return 0;
}

static void milkymist_uart_stop_tx(struct uart_port *port)
{
}

static void milkymist_uart_stop_rx(struct uart_port *port)
{
}

static void milkymist_uart_enable_ms(struct uart_port *port)
{
}

static void milkymist_uart_break_ctl(struct uart_port *port, int break_state)
{
}

static int milkymist_uart_startup(struct uart_port *port)
{
	int ret;

	ret = request_irq(port->irq, milkymist_uart_isr,
			IRQF_DISABLED, "milkymist_uart", port);

	/* ack events */
	iowrite32be(UART_STAT_TX_EVT | UART_STAT_RX_EVT,
			port->membase + UART_STAT);

	iowrite32be(UART_CTRL_RX_INT | UART_CTRL_TX_INT,
			port->membase + UART_CTRL);

	if (ret) {
		pr_err("milkymist_uart: unable to attach interrupt\n");
		return ret;
	}

	return 0;
}

static void milkymist_uart_shutdown(struct uart_port *port)
{
	iowrite32be(0, port->membase + UART_CTRL);
	free_irq(port->irq, port);
}

static const char *milkymist_uart_type(struct uart_port *port)
{
	return (port->type == PORT_MILKYMIST_UART) ? "Milkymist UART" : NULL;
}

static int milkymist_uart_request_port(struct uart_port *port)
{
	if (!request_mem_region(port->mapbase, 16, "milkymist_uart")) {
		dev_err(port->dev, "memory region busy\n");
		return -EBUSY;
	}

	port->membase = ioremap(port->mapbase, 16);
	if (!port->membase) {
		dev_err(port->dev, "unable to map registers\n");
		release_mem_region(port->mapbase, 16);
		return -EBUSY;
	}

	return 0;
}

static void milkymist_uart_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, 16);
	iounmap(port->membase);
	port->membase = NULL;
}

static void milkymist_uart_config_port(struct uart_port *port, int flags)
{
	if (!milkymist_uart_request_port(port))
		port->type = PORT_MILKYMIST_UART;
}

static int milkymist_uart_verify_port(struct uart_port *port,
		struct serial_struct *ser)
{
	if ((ser->type != PORT_UNKNOWN) && (ser->type != PORT_MILKYMIST_UART))
		return -EINVAL;
	return 0;
}

static void milkymist_uart_set_termios(struct uart_port *port,
		struct ktermios *termios, struct ktermios *old)
{
	unsigned long flags;
	unsigned int baud;
	unsigned int quot;

	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/16);
	quot = uart_get_divisor(port, baud);

	spin_lock_irqsave(&port->lock, flags);
	uart_update_timeout(port, termios->c_cflag, baud);
	iowrite32be(quot, port->membase + UART_DIV);
	spin_unlock_irqrestore(&port->lock, flags);

	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);
}

static struct uart_ops milkymist_uart_ops = {
	.tx_empty       = milkymist_uart_tx_empty,
	.set_mctrl      = milkymist_uart_set_mctrl,
	.get_mctrl      = milkymist_uart_get_mctrl,
	.stop_tx        = milkymist_uart_stop_tx,
	.start_tx       = milkymist_uart_start_tx,
	.stop_rx        = milkymist_uart_stop_rx,
	.enable_ms      = milkymist_uart_enable_ms,
	.break_ctl      = milkymist_uart_break_ctl,
	.startup        = milkymist_uart_startup,
	.shutdown       = milkymist_uart_shutdown,
	.set_termios    = milkymist_uart_set_termios,
	.type           = milkymist_uart_type,
	.release_port   = milkymist_uart_release_port,
	.request_port   = milkymist_uart_request_port,
	.config_port    = milkymist_uart_config_port,
	.verify_port    = milkymist_uart_verify_port,
#if 0
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char  = milkymist_uart_get_poll_char,
	.poll_put_char  = milkymist_uart_put_poll_char,
#endif
#endif
};

#ifdef CONFIG_SERIAL_MILKYMIST_CONSOLE
static void milkymist_uart_console_wait_tx(struct uart_port *port)
{
	int i;
	u8 stat;

	for (i = 0; i < 100000; i++) {
		stat = ioread32be(port->membase + UART_STAT) & 0xff;
		if (stat & UART_STAT_THRE)
			break;
		cpu_relax();
	}
}

static void milkymist_uart_console_putchar(struct uart_port *port, int ch)
{
	milkymist_uart_console_wait_tx(port);
	iowrite32be(ch, port->membase + UART_RXTX);
}

static void milkymist_uart_console_write(struct console *co, const char *s,
		unsigned int count)
{
	struct uart_port *port = &milkymist_uart_ports[co->index];
	u32 ctrl;
	unsigned long flags;
	int locked = 1;

	if (oops_in_progress)
		locked = spin_trylock_irqsave(&port->lock, flags);
	else
		spin_lock_irqsave(&port->lock, flags);

	/* wait until current transmission is finished */
	milkymist_uart_console_wait_tx(port);

	/* save ctrl and stat */
	ctrl = ioread32be(port->membase + UART_CTRL);

	/* disable irqs */
	iowrite32be(ctrl & ~(UART_CTRL_RX_INT | UART_CTRL_TX_INT),
			port->membase + UART_CTRL);

	uart_console_write(port, s, count, milkymist_uart_console_putchar);
	milkymist_uart_console_wait_tx(port);

	/* ack write event */
	iowrite32be(UART_STAT_TX_EVT, port->membase + UART_STAT);

	/* restore control register */
	iowrite32be(ctrl, port->membase + UART_CTRL);

	if (locked)
		spin_unlock_irqrestore(&port->lock, flags);
}

static int __devinit milkymist_uart_console_setup(struct console *co,
		char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (co->index < 0 || co->index >= MILKYMIST_NR_UARTS)
		return -EINVAL;

	port = &milkymist_uart_ports[co->index];

	if (!port->mapbase) {
		pr_debug("console on ttyS%i not present\n", co->index);
		return -ENODEV;
	}

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver milkymist_uart_driver;

static struct console milkymist_uart_console = {
	.name	=	"ttyS",
	.write	=	milkymist_uart_console_write,
	.device	=	uart_console_device,
	.setup	=	milkymist_uart_console_setup,
	.flags	=	CON_PRINTBUFFER,
	.index	=	-1,
	.data	=	&milkymist_uart_driver,
};

static int __init milkymist_uart_console_init(void)
{
	register_console(&milkymist_uart_console);
	return 0;
}
console_initcall(milkymist_uart_console_init);
#endif

static struct uart_driver milkymist_uart_driver = {
	.owner = THIS_MODULE,
	.driver_name = "milkymist_uart",
	.dev_name    = "ttyS",
	.major       = MILKYMIST_UART_MAJOR,
	.minor       = MILKYMIST_UART_MINOR,
	.nr          = MILKYMIST_NR_UARTS,
#ifdef CONFIG_SERIAL_MILKYMIST_CONSOLE
	.cons        = &milkymist_uart_console,
#endif
};

static int __devinit milkymist_uart_probe(struct platform_device *op)
{
	struct uart_port *port;
	struct device_node *np = op->dev.of_node;
	int ret;
	struct resource res;
	const unsigned int *pid, *clk;
	int id;
	int irq;

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		dev_err(&op->dev, "invalid address\n");
		return ret;
	}

	irq = irq_of_parse_and_map(np, 0);

	clk = of_get_property(np, "clock-frequency", NULL);
	if (!clk) {
		dev_warn(&op->dev, "no clock-frequency property set\n");
		return -ENODEV;
	}

	pid = of_get_property(np, "port-number", NULL);

	if (pid)
		id = *pid;
	else {
		/* find free id */
		for (id = 0; id < MILKYMIST_NR_UARTS; id++)
			if (milkymist_uart_ports[id].mapbase == 0)
				break;
	}

	if (id < 0 || id >= MILKYMIST_NR_UARTS) {
		dev_err(&op->dev, "milkymist_uart%i too large\n", id);
		return -EINVAL;
	}

	if (milkymist_uart_ports[id].mapbase
			&& milkymist_uart_ports[id].mapbase != res.start) {
		dev_err(&op->dev, "milkymist_uart%i already in use\n", id);
		return -EBUSY;
	}

	ret = uart_register_driver(&milkymist_uart_driver);
	if (ret) {
		dev_err(&op->dev, "uart_register_driver() failed; err=%i\n", ret);
		return ret;
	}

	port = &milkymist_uart_ports[id];

	spin_lock_init(&port->lock);
	port->line = id;
	port->regshift = 2;
	port->iotype = UPIO_MEM;
	port->mapbase = res.start;
	port->membase = NULL;
	port->flags = UPF_BOOT_AUTOCONF;
	port->irq = irq;

	port->ops = &milkymist_uart_ops;
	port->type = PORT_UNKNOWN;
	port->uartclk = *clk;
	
	port->dev = &op->dev;

	dev_set_drvdata(&op->dev, port);

	ret = uart_add_one_port(&milkymist_uart_driver, port);
	if (ret) {
		dev_err(&op->dev, "uart_add_one_port() failed; err=%i\n", ret);
		port->mapbase = 0;
		return ret;
	}

	//device_init_wakeup(&op->dev, 1);

	return 0;
}

static int __devexit milkymist_uart_remove(struct platform_device *dev)
{
	struct uart_port *port = dev_get_drvdata(&dev->dev);

	uart_remove_one_port(&milkymist_uart_driver, port);
	dev_set_drvdata(&dev->dev, NULL);
	port->mapbase = 0;

	return 0;
}

static const struct of_device_id milkymist_uart_match[] = {
	{ .compatible = "milkymist,uart", },
	{},
};
MODULE_DEVICE_TABLE(of, milkymist_uart_match);

static struct platform_driver milkymist_uart_of_driver = {
	.driver = {
		.name = "milkymist_uart",
		.owner = THIS_MODULE,
		.of_match_table = milkymist_uart_match,
	},
	.probe		= milkymist_uart_probe,
	.remove		= __devexit_p(milkymist_uart_remove),
};

static int __init milkymist_uart_init(void)
{
	return platform_driver_register(&milkymist_uart_of_driver);
}

static void __exit milkymist_uart_exit(void)
{
	platform_driver_unregister(&milkymist_uart_of_driver);
}

module_init(milkymist_uart_init);
module_exit(milkymist_uart_exit);

MODULE_AUTHOR("Milkymist Project");
MODULE_DESCRIPTION("Milkymist UART driver");
MODULE_LICENSE("GPL");
