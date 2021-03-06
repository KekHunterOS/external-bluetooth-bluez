/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2013-2014  Intel Corporation. All rights reserved.
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "hal.h"

struct hal_sock_connect_signal {
	short   size;
	uint8_t bdaddr[6];
	int     channel;
	int     status;
#if ANDROID_VERSION >= PLATFORM_VER(6, 0, 0)
	unsigned short max_tx_packet_size;
	unsigned short max_rx_packet_size;
#endif
} __attribute__((packed));

void bt_socket_register(struct ipc *ipc, const bdaddr_t *addr, uint8_t mode);
void bt_socket_unregister(void);
