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

static void
test_bad_close(int fd)
{
	struct drm_gem_close close_bo;
	int ret;

	printf("Testing error return on bad close ioctl.\n");

	close_bo.handle = 0x10101010;
	ret = ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_bo);

	assert(ret == -1 && errno == EINVAL);
}

static void
test_create_close(int fd)
{
	uint32_t handle;

	printf("Testing creating and closing an object.\n");

	handle = gem_create(fd, 16*1024);

	gem_close(fd, handle);
}

static void
test_create_fd_close(int fd)
{
	printf("Testing closing with an object allocated.\n");

	gem_create(fd, 16*1024);
	/* leak it */

	close(fd);
}

int main(int argc, char **argv)
{
	int fd;

	drmtest_subtest_init(argc, argv);

	fd = drm_open_any();

	if (drmtest_run_subtest("bad-close"))
		test_bad_close(fd);
	if (drmtest_run_subtest("create-close"))
		test_create_close(fd);
	if (drmtest_run_subtest("create-fd-close"))
		test_create_fd_close(fd);

	return 0;
}
