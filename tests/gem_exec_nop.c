/*
 * Copyright © 2011 Intel Corporation
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
 *    Chris Wilson <chris@chris-wilson.co.uk>
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "drm.h"
#include "i915_drm.h"
#include "drmtest.h"
#include "intel_gpu_tools.h"

bool skipped_all = true;

static double elapsed(const struct timeval *start,
		      const struct timeval *end,
		      int loop)
{
	return (1e6*(end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec))/loop;
}

static int exec(int fd, uint32_t handle, int loops, unsigned ring_id)
{
	struct drm_i915_gem_execbuffer2 execbuf;
	struct drm_i915_gem_exec_object2 gem_exec[1];
	int ret = 0;

	gem_exec[0].handle = handle;
	gem_exec[0].relocation_count = 0;
	gem_exec[0].relocs_ptr = 0;
	gem_exec[0].alignment = 0;
	gem_exec[0].offset = 0;
	gem_exec[0].flags = 0;
	gem_exec[0].rsvd1 = 0;
	gem_exec[0].rsvd2 = 0;

	execbuf.buffers_ptr = (uintptr_t)gem_exec;
	execbuf.buffer_count = 1;
	execbuf.batch_start_offset = 0;
	execbuf.batch_len = 8;
	execbuf.cliprects_ptr = 0;
	execbuf.num_cliprects = 0;
	execbuf.DR1 = 0;
	execbuf.DR4 = 0;
	execbuf.flags = ring_id;
	i915_execbuffer2_set_context_id(execbuf, 0);
	execbuf.rsvd2 = 0;

	while (loops-- && ret == 0) {
		ret = drmIoctl(fd,
			       DRM_IOCTL_I915_GEM_EXECBUFFER2,
			       &execbuf);
	}
	gem_sync(fd, handle);

	return ret;
}

static void loop(int fd, uint32_t handle, unsigned ring_id, const char *ring_name)
{
	int count;

	skipped_all = false;

	for (count = 1; count <= 1<<17; count <<= 1) {
		struct timeval start, end;

		gettimeofday(&start, NULL);
		if (exec(fd, handle, count, ring_id))
			exit(1);
		gettimeofday(&end, NULL);
		printf("Time to exec x %d:		%7.3fµs (ring=%s)\n",
		       count, elapsed(&start, &end, count), ring_name);
		fflush(stdout);
	}

}
int main(int argc, char **argv)
{
	uint32_t batch[2] = {MI_BATCH_BUFFER_END};
	uint32_t handle;
	uint32_t devid;
	int fd;

	drmtest_subtest_init(argc, argv);

	fd = drm_open_any();
	devid = intel_get_drm_devid(fd);

	handle = gem_create(fd, 4096);
	gem_write(fd, handle, 0, batch, sizeof(batch));

	if (drmtest_run_subtest("render"))
		loop(fd, handle, I915_EXEC_RENDER, "render");

	if (drmtest_run_subtest("bsd"))
		if (HAS_BSD_RING(devid))
			loop(fd, handle, I915_EXEC_BSD, "bsd");

	if (drmtest_run_subtest("blt"))
		if (HAS_BLT_RING(devid))
			loop(fd, handle, I915_EXEC_BLT, "blt");


	gem_close(fd, handle);

	close(fd);

	return skipped_all ? 77 : 0;
}
