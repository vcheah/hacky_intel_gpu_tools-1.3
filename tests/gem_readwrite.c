/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "drm.h"
#include "i915_drm.h"
#include "drmtest.h"

#define OBJECT_SIZE 16384

static int
do_read(int fd, int handle, void *buf, int offset, int size)
{
	struct drm_i915_gem_pread gem_pread;

	/* Ensure that we don't have any convenient data in buf in case
	 * we fail.
	 */
	memset(buf, 0xd0, size);

	memset(&gem_pread, 0, sizeof(gem_pread));
	gem_pread.handle = handle;
	gem_pread.data_ptr = (uintptr_t)buf;
	gem_pread.size = size;
	gem_pread.offset = offset;

	return ioctl(fd, DRM_IOCTL_I915_GEM_PREAD, &gem_pread);
}

static int
do_write(int fd, int handle, void *buf, int offset, int size)
{
	struct drm_i915_gem_pwrite gem_pwrite;

	memset(&gem_pwrite, 0, sizeof(gem_pwrite));
	gem_pwrite.handle = handle;
	gem_pwrite.data_ptr = (uintptr_t)buf;
	gem_pwrite.size = size;
	gem_pwrite.offset = offset;

	return ioctl(fd, DRM_IOCTL_I915_GEM_PWRITE, &gem_pwrite);
}

int main(int argc, char **argv)
{
	int fd;
	uint8_t expected[OBJECT_SIZE];
	uint8_t buf[OBJECT_SIZE];
	int ret;
	int handle;

	fd = drm_open_any();

	handle = gem_create(fd, OBJECT_SIZE);

	printf("Testing contents of newly created object.\n");
	ret = do_read(fd, handle, buf, 0, OBJECT_SIZE);
	assert(ret == 0);
	memset(&expected, 0, sizeof(expected));
	assert(memcmp(expected, buf, sizeof(expected)) == 0);

	printf("Testing read beyond end of buffer.\n");
	ret = do_read(fd, handle, buf, OBJECT_SIZE / 2, OBJECT_SIZE);
	assert(ret == -1 && errno == EINVAL);

	printf("Testing full write of buffer\n");
	memset(buf, 0, sizeof(buf));
	memset(buf + 1024, 0x01, 1024);
	memset(expected + 1024, 0x01, 1024);
	ret = do_write(fd, handle, buf, 0, OBJECT_SIZE);
	assert(ret == 0);
	ret = do_read(fd, handle, buf, 0, OBJECT_SIZE);
	assert(ret == 0);
	assert(memcmp(buf, expected, sizeof(buf)) == 0);

	printf("Testing partial write of buffer\n");
	memset(buf + 4096, 0x02, 1024);
	memset(expected + 4096, 0x02, 1024);
	ret = do_write(fd, handle, buf + 4096, 4096, 1024);
	assert(ret == 0);
	ret = do_read(fd, handle, buf, 0, OBJECT_SIZE);
	assert(ret == 0);
	assert(memcmp(buf, expected, sizeof(buf)) == 0);

	printf("Testing partial read of buffer\n");
	ret = do_read(fd, handle, buf, 512, 1024);
	assert(ret == 0);
	assert(memcmp(buf, expected + 512, 1024) == 0);

	printf("Testing read of bad buffer handle\n");
	ret = do_read(fd, 1234, buf, 0, 1024);
	assert(ret == -1 && errno == ENOENT);

	printf("Testing write of bad buffer handle\n");
	ret = do_write(fd, 1234, buf, 0, 1024);
	assert(ret == -1 && errno == ENOENT);

	close(fd);

	return 0;
}
