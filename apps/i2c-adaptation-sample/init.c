/*
 * Copyright (c) 2019 Vijay Kumar Banerjee. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <assert.h>
#include <stdlib.h>
#include <sysexits.h>

#include <rtems.h>
#include <rtems/bsd/bsd.h>
#include <rtems/dhcpcd.h>
#include <bsp/i2c.h>
#include <libcpu/am335x.h>
#include <rtems/irq-extension.h>
#include <rtems/counter.h>
#include <bsp/bbb-gpio.h>
#include <rtems/console.h>
#include <rtems/shell.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dev/iicbus/iic.h>

#include <machine/rtems-bsd-commands.h>

#include <bsp.h>

#define PRIO_SHELL		150
#define PRIO_WPA		(RTEMS_MAXIMUM_PRIORITY - 1)
#define PRIO_INIT_TASK		(RTEMS_MAXIMUM_PRIORITY - 1)
#define PRIO_MEDIA_SERVER	200
#define STACK_SIZE_SHELL	(64 * 1024)
#define I2C_BUS "/dev/iic0"
#define EEPROM_ADDRESS 0x50

int 
read_eeprom(int address)
{
	int		fd;
	int		ret;
	uint8_t		wbuf[2];
	uint8_t		rbuf[2];
	struct iic_msg 	msg[2];
	struct iic_rdwr_data rdwr;

	if ((fd = open(I2C_BUS, O_RDWR)) < 0) {
		perror("open");
		return (-1);
	}

	msg[0].slave = EEPROM_ADDRESS << 1;
	msg[0].flags = 0;
	msg[0].len = 1;
	wbuf[0] = 0xff & address;
	msg[0].buf = wbuf;

	rbuf[0] = 0;
	rbuf[1] = 0;

	msg[1].slave = (EEPROM_ADDRESS << 1) | IIC_M_RD;
	msg[1].flags = IIC_M_RD;
	msg[1].len = 2;
	msg[1].buf = rbuf;

	rdwr.msgs = msg;
	rdwr.nmsgs = 2;

	ret = ioctl(fd, I2CRDWR, &rdwr);

	if (ret < 0) {
		perror("ioctl(I2CRDWR)");
		ret = (-1);
	} else 
		ret = (rbuf[0] << 8) + rbuf[1];

	close(fd);

	return (ret);
}

void
libbsdhelper_start_shell(rtems_task_priority prio)
{
	rtems_status_code sc = rtems_shell_init(
		"SHLL",
		STACK_SIZE_SHELL,
		prio,
		CONSOLE_DEVICE_NAME,
		false,
		true,
		NULL
	);
	assert(sc == RTEMS_SUCCESSFUL);
}

static void
Init(rtems_task_argument arg)
{
	rtems_status_code sc;
	int exit_code;
    int data;

	(void)arg;

	puts("\nRTEMS I2C TEST\n");
    exit_code = bbb_register_i2c_0();
    assert(exit_code == 0);
	sc = rtems_bsd_initialize();
	assert(sc == RTEMS_SUCCESSFUL);
    data = read_eeprom(0);
    printf("%d", data);

	/* Some time for USB device to be detected. */
//	rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(4000));
	libbsdhelper_start_shell(PRIO_SHELL);

	exit(0);
}

/*
 * Configure LibBSD.
 */
#define RTEMS_BSD_CONFIG_BSP_CONFIG
#define RTEMS_BSD_CONFIG_TERMIOS_KQUEUE_AND_POLL
#define RTEMS_BSD_CONFIG_INIT

#include <machine/rtems-bsd-config.h>

/*
 * Configure RTEMS.
 */
#define CONFIGURE_MICROSECONDS_PER_TICK 1000

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_STUB_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_ZERO_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK

#define CONFIGURE_FILESYSTEM_DOSFS
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 32

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 1

#define CONFIGURE_INIT_TASK_STACK_SIZE (64*1024)
#define CONFIGURE_INIT_TASK_INITIAL_MODES RTEMS_DEFAULT_MODES
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_BDBUF_BUFFER_MAX_SIZE (32 * 1024)
#define CONFIGURE_BDBUF_MAX_READ_AHEAD_BLOCKS 4
#define CONFIGURE_BDBUF_CACHE_MEMORY_SIZE (1 * 1024 * 1024)
#define CONFIGURE_BDBUF_READ_AHEAD_TASK_PRIORITY 97
#define CONFIGURE_SWAPOUT_TASK_PRIORITY 97

//#define CONFIGURE_STACK_CHECKER_ENABLED

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT

#include <rtems/confdefs.h>

/*
 * Configure Shell.
 */
#include <rtems/netcmds-config.h>
#include <bsp/irq-info.h>
#define CONFIGURE_SHELL_COMMANDS_INIT

#define CONFIGURE_SHELL_USER_COMMANDS \
  &bsp_interrupt_shell_command, \
  &rtems_shell_ARP_Command, \
  &rtems_shell_I2C_Command

#define CONFIGURE_SHELL_COMMANDS_ALL

#include <rtems/shellconfig.h>
