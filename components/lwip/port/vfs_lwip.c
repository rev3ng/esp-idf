// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/errno.h>
#include <sys/lock.h>
#include <sys/fcntl.h>
#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "esp_attr.h"
#include "soc/uart_struct.h"
#include "lwip/sockets.h"
#include "sdkconfig.h"

/* Non-LWIP file descriptors are from 0 to (LWIP_SOCKET_OFFSET-1).
 * LWIP file descriptors are from LWIP_SOCKET_OFFSET to FD_SETSIZE-1.
*/

int lwip_socket_offset;

static int lwip_fcntl_r_wrapper(int fd, int cmd, va_list args);
static int lwip_ioctl_r_wrapper(int fd, int cmd, va_list args);

void esp_vfs_lwip_sockets_register()
{
    esp_vfs_t vfs = {
        .flags = ESP_VFS_FLAG_DEFAULT,
        .write = &lwip_write_r,
        .open = NULL,
        .fstat = NULL,
        .close = &lwip_close_r,
        .read = &lwip_read_r,
        .fcntl = &lwip_fcntl_r_wrapper,
        .ioctl = &lwip_ioctl_r_wrapper,
        .socket_select = &lwip_select
    };
    int max_fd;

    ESP_ERROR_CHECK(esp_vfs_register_socket_space(&vfs, NULL, &lwip_socket_offset, &max_fd));

    /* LWIP can't be allowed to create more sockets than fit in the per-VFS fd space. Currently this isn't configurable
     * but it's set much larger than CONFIG_LWIP_MAX_SOCKETS should ever be (max 2^12 FDs).
     */
    assert(max_fd >= lwip_socket_offset && CONFIG_LWIP_MAX_SOCKETS <= max_fd - lwip_socket_offset);
}

static int lwip_fcntl_r_wrapper(int fd, int cmd, va_list args)
{
    return lwip_fcntl_r(fd, cmd, va_arg(args, int));
}

static int lwip_ioctl_r_wrapper(int fd, int cmd, va_list args)
{
    return lwip_ioctl_r(fd, cmd, va_arg(args, void *));
}


