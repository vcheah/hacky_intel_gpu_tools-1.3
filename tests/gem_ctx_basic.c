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
 *    Ben Widawsky <ben@bwidawsk.net>
 *
 */

/*
 * This test is useful for finding memory and refcount leaks.
 */

#include <pthread.h>
#include "rendercopy.h"

/* options */
int num_contexts = 10;
int uncontexted = 0; /* test only context create/destroy */
int multiple_fds = 1;
int iter = 10000;

/* globals */
pthread_t *threads;
int *returns;
int devid;
int fd;

static void init_buffer(drm_intel_bufmgr *bufmgr,
			struct scratch_buf *buf,
			uint32_t size)
{
	buf->bo = drm_intel_bo_alloc(bufmgr, "", size, 4096);
	buf->size = size;
	assert(buf->bo);
	buf->tiling = I915_TILING_NONE;
	buf->stride = 4096;
}

static void *work(void *arg)
{
	struct intel_batchbuffer *batch;
	render_copyfunc_t rendercopy = get_render_copyfunc(devid);
	drm_intel_context *context;
	drm_intel_bufmgr *bufmgr;
	int thread_id = *(int *)arg;
	int td_fd;
	int i;

	if (multiple_fds)
		td_fd = fd = drm_open_any();
	else
		td_fd = fd;

	assert(td_fd >= 0);

	bufmgr = drm_intel_bufmgr_gem_init(td_fd, 4096);
	batch = intel_batchbuffer_alloc(bufmgr, devid);
	context = drm_intel_gem_context_create(bufmgr);

	if (!context) {
		returns[thread_id] = 77;
		goto out;
	}

	for (i = 0; i < iter; i++) {
		struct scratch_buf src, dst;

		init_buffer(bufmgr, &src, 4096);
		init_buffer(bufmgr, &dst, 4096);


		if (uncontexted) {
			assert(rendercopy);
			rendercopy(batch, &src, 0, 0, 0, 0, &dst, 0, 0);
		} else {
			int ret;
			ret = drm_intel_bo_subdata(batch->bo, 0, 4096, batch->buffer);
			assert(ret == 0);
			intel_batchbuffer_flush_with_context(batch, context);
		}
	}

out:
	drm_intel_gem_context_destroy(context);
	intel_batchbuffer_free(batch);
	drm_intel_bufmgr_destroy(bufmgr);

	if (multiple_fds)
		close(td_fd);

	pthread_exit(&returns[thread_id]);
}

static void parse(int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "i:c:n:muh?")) != -1) {
		switch (opt) {
		case 'i':
			iter = atoi(optarg);
			break;
		case 'c':
			num_contexts = atoi(optarg);
			break;
		case 'm':
			multiple_fds = 1;
			break;
		case 'u':
			uncontexted = 1;
			break;
		case 'h':
		case '?':
		default:
			exit(EXIT_SUCCESS);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int i;

	fd = drm_open_any();
	devid = intel_get_drm_devid(fd);

	parse(argc, argv);

	threads = calloc(num_contexts, sizeof(*threads));
	returns = calloc(num_contexts, sizeof(*returns));

	for (i = 0; i < num_contexts; i++)
		pthread_create(&threads[i], NULL, work, &i);

	for (i = 0; i < num_contexts; i++) {
		int thread_status, ret;
		void *retval;
		ret = pthread_join(threads[i], &retval);
		thread_status = *(int *)retval;
		if (!ret && thread_status)
			exit(thread_status);
	}

	free(returns);
	free(threads);
	close(fd);

	exit(EXIT_SUCCESS);
}
