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

/** @file gem_linear_render_blits.c
 *
 * This is a test of doing many blits, with a working set
 * larger than the aperture size.
 *
 * The goal is to simply ensure the basics work.
 */

#include "rendercopy.h"

#define WIDTH 512
#define STRIDE (WIDTH*4)
#define HEIGHT 512
#define SIZE (HEIGHT*STRIDE)

static render_copyfunc_t render_copy;

static void
check_bo(drm_intel_bo *bo, uint32_t val)
{
	uint32_t *ptr;
	int i;

	do_or_die(drm_intel_gem_bo_map_gtt(bo));
	ptr = bo->virtual;
	for (i = 0; i < WIDTH*HEIGHT; i++) {
		if (ptr[i] != val) {
			fprintf(stderr, "Expected 0x%08x, found 0x%08x "
				"at offset 0x%08x\n",
				val, ptr[i], i * 4);
			abort();
		}
		val++;
	}
	drm_intel_gem_bo_unmap_gtt(bo);
}

int main(int argc, char **argv)
{
	drm_intel_bufmgr *bufmgr;
	struct intel_batchbuffer *batch;
	uint32_t *start_val;
	struct scratch_buf *buf;
	uint32_t start = 0;
	int i, j, fd, count;

	fd = drm_open_any();

	render_copy = get_render_copyfunc(intel_get_drm_devid(fd));
	if (render_copy == NULL) {
		printf("no render-copy function, doing nothing\n");
		return 77;
	}

	bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
	drm_intel_bufmgr_gem_set_vma_cache_size(bufmgr, 32);
	batch = intel_batchbuffer_alloc(bufmgr, intel_get_drm_devid(fd));

	count = 0;
	if (argc > 1)
		count = atoi(argv[1]);
	if (count == 0)
		count = 3 * gem_aperture_size(fd) / SIZE / 2;
	printf("Using %d 1MiB buffers\n", count);

	buf = malloc(sizeof(*buf)*count);
	start_val = malloc(sizeof(*start_val)*count);

	for (i = 0; i < count; i++) {
		uint32_t tiling = I915_TILING_X + (random() & 1);
		unsigned long pitch = STRIDE;
		uint32_t *ptr;

		buf[i].bo = drm_intel_bo_alloc_tiled(bufmgr, "",
						     WIDTH, HEIGHT, 4,
						     &tiling, &pitch, 0);
		buf[i].stride = pitch;
		buf[i].tiling = tiling;
		buf[i].size = SIZE;

		start_val[i] = start;

		do_or_die(drm_intel_gem_bo_map_gtt(buf[i].bo));
		ptr = buf[i].bo->virtual;
		for (j = 0; j < WIDTH*HEIGHT; j++)
			ptr[j] = start++;
		drm_intel_gem_bo_unmap_gtt(buf[i].bo);
	}

	printf("Verifying initialisation...\n");
	for (i = 0; i < count; i++)
		check_bo(buf[i].bo, start_val[i]);

	printf("Cyclic blits, forward...\n");
	for (i = 0; i < count * 4; i++) {
		int src = i % count;
		int dst = (i + 1) % count;

		render_copy(batch, buf+src, 0, 0, WIDTH, HEIGHT, buf+dst, 0, 0);
		start_val[dst] = start_val[src];
	}
	for (i = 0; i < count; i++)
		check_bo(buf[i].bo, start_val[i]);

	printf("Cyclic blits, backward...\n");
	for (i = 0; i < count * 4; i++) {
		int src = (i + 1) % count;
		int dst = i % count;

		render_copy(batch, buf+src, 0, 0, WIDTH, HEIGHT, buf+dst, 0, 0);
		start_val[dst] = start_val[src];
	}
	for (i = 0; i < count; i++)
		check_bo(buf[i].bo, start_val[i]);

	printf("Random blits...\n");
	for (i = 0; i < count * 4; i++) {
		int src = random() % count;
		int dst = random() % count;

		if (src == dst)
			continue;

		render_copy(batch, buf+src, 0, 0, WIDTH, HEIGHT, buf+dst, 0, 0);
		start_val[dst] = start_val[src];
	}
	for (i = 0; i < count; i++)
		check_bo(buf[i].bo, start_val[i]);

	return 0;
}
