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
 *    Daniel Vetter <daniel.vetter@ffwll.ch>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "drm.h"
#include "i915_drm.h"
#include "drmtest.h"
#include "intel_bufmgr.h"
#include "intel_batchbuffer.h"
#include "intel_gpu_tools.h"

/*
 * Testcase: pwrite/pread consistency when touching partial cachelines
 *
 * Some fancy new pwrite/pread optimizations clflush in-line while
 * reading/writing. Check whether all required clflushes happen.
 *
 */

static drm_intel_bufmgr *bufmgr;
struct intel_batchbuffer *batch;

drm_intel_bo *scratch_bo;
drm_intel_bo *staging_bo;
#define BO_SIZE (4*4096)
uint32_t devid;
uint64_t mappable_gtt_limit;
int fd;

static void
copy_bo(drm_intel_bo *src, drm_intel_bo *dst)
{
	BEGIN_BATCH(8);
	OUT_BATCH(XY_SRC_COPY_BLT_CMD |
		  XY_SRC_COPY_BLT_WRITE_ALPHA |
		  XY_SRC_COPY_BLT_WRITE_RGB);
	OUT_BATCH((3 << 24) | /* 32 bits */
		  (0xcc << 16) | /* copy ROP */
		  4096);
	OUT_BATCH(0 << 16 | 0);
	OUT_BATCH((BO_SIZE/4096) << 16 | 1024);
	OUT_RELOC_FENCED(dst, I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER, 0);
	OUT_BATCH(0 << 16 | 0);
	OUT_BATCH(4096);
	OUT_RELOC_FENCED(src, I915_GEM_DOMAIN_RENDER, 0, 0);
	ADVANCE_BATCH();

	intel_batchbuffer_flush(batch);
}

static void
blt_bo_fill(drm_intel_bo *tmp_bo, drm_intel_bo *bo, int val)
{
	uint8_t *gtt_ptr;
	int i;

	drm_intel_gem_bo_map_gtt(tmp_bo);
	gtt_ptr = tmp_bo->virtual;

	for (i = 0; i < BO_SIZE; i++)
		gtt_ptr[i] = val;

	drm_intel_gem_bo_unmap_gtt(tmp_bo);

	if (bo->offset < mappable_gtt_limit &&
	    (IS_G33(devid) || intel_gen(devid) >= 4))
		drmtest_trash_aperture();

	copy_bo(tmp_bo, bo);
}

#define MAX_BLT_SIZE 128
#define ROUNDS 1000
uint8_t tmp[BO_SIZE];

static void test_partial_reads(void)
{
	int i, j;

	printf("checking partial reads\n");
	for (i = 0; i < ROUNDS; i++) {
		int start, len;
		int val = i % 256;

		blt_bo_fill(staging_bo, scratch_bo, i);

		start = random() % BO_SIZE;
		len = random() % (BO_SIZE-start) + 1;

		drm_intel_bo_get_subdata(scratch_bo, start, len, tmp);
		for (j = 0; j < len; j++) {
			if (tmp[j] != val) {
				printf("mismatch at %i, got: %i, expected: %i\n",
				       j, tmp[j], val);
				exit(1);
			}
		}

		drmtest_progress("partial reads test: ", i, ROUNDS);
	}

}

static void test_partial_writes(void)
{
	int i, j;
	uint8_t *gtt_ptr;

	printf("checking partial writes\n");
	for (i = 0; i < ROUNDS; i++) {
		int start, len;
		int val = i % 256;

		blt_bo_fill(staging_bo, scratch_bo, i);

		start = random() % BO_SIZE;
		len = random() % (BO_SIZE-start) + 1;

		memset(tmp, i + 63, BO_SIZE);

		drm_intel_bo_subdata(scratch_bo, start, len, tmp);

		copy_bo(scratch_bo, staging_bo);
		drm_intel_gem_bo_map_gtt(staging_bo);
		gtt_ptr = staging_bo->virtual;

		for (j = 0; j < start; j++) {
			if (gtt_ptr[j] != val) {
				printf("mismatch at %i, got: %i, expected: %i\n",
				       j, tmp[j], val);
				exit(1);
			}
		}
		for (; j < start + len; j++) {
			if (gtt_ptr[j] != tmp[0]) {
				printf("mismatch at %i, got: %i, expected: %i\n",
				       j, tmp[j], i);
				exit(1);
			}
		}
		for (; j < BO_SIZE; j++) {
			if (gtt_ptr[j] != val) {
				printf("mismatch at %i, got: %i, expected: %i\n",
				       j, tmp[j], val);
				exit(1);
			}
		}
		drm_intel_gem_bo_unmap_gtt(staging_bo);

		drmtest_progress("partial writes test: ", i, ROUNDS);
	}

}

static void test_partial_read_writes(void)
{
	int i, j;
	uint8_t *gtt_ptr;

	printf("checking partial writes after partial reads\n");
	for (i = 0; i < ROUNDS; i++) {
		int start, len;
		int val = i % 256;

		blt_bo_fill(staging_bo, scratch_bo, i);

		/* partial read */
		start = random() % BO_SIZE;
		len = random() % (BO_SIZE-start) + 1;

		drm_intel_bo_get_subdata(scratch_bo, start, len, tmp);
		for (j = 0; j < len; j++) {
			if (tmp[j] != val) {
				printf("mismatch in read at %i, got: %i, expected: %i\n",
				       j, tmp[j], val);
				exit(1);
			}
		}

		/* Change contents through gtt to make the pread cachelines
		 * stale. */
		val = (i + 17) % 256;
		blt_bo_fill(staging_bo, scratch_bo, val);

		/* partial write */
		start = random() % BO_SIZE;
		len = random() % (BO_SIZE-start) + 1;

		memset(tmp, i + 63, BO_SIZE);

		drm_intel_bo_subdata(scratch_bo, start, len, tmp);

		copy_bo(scratch_bo, staging_bo);
		drm_intel_gem_bo_map_gtt(staging_bo);
		gtt_ptr = staging_bo->virtual;

		for (j = 0; j < start; j++) {
			if (gtt_ptr[j] != val) {
				printf("mismatch at %i, got: %i, expected: %i\n",
				       j, tmp[j], val);
				exit(1);
			}
		}
		for (; j < start + len; j++) {
			if (gtt_ptr[j] != tmp[0]) {
				printf("mismatch at %i, got: %i, expected: %i\n",
				       j, tmp[j], tmp[0]);
				exit(1);
			}
		}
		for (; j < BO_SIZE; j++) {
			if (gtt_ptr[j] != val) {
				printf("mismatch at %i, got: %i, expected: %i\n",
				       j, tmp[j], val);
				exit(1);
			}
		}
		drm_intel_gem_bo_unmap_gtt(staging_bo);

		drmtest_progress("partial read/writes test: ", i, ROUNDS);
	}
}

int main(int argc, char **argv)
{
	srandom(0xdeadbeef);

	drmtest_subtest_init(argc, argv);

	fd = drm_open_any();

	bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
	//drm_intel_bufmgr_gem_enable_reuse(bufmgr);
	devid = intel_get_drm_devid(fd);
	batch = intel_batchbuffer_alloc(bufmgr, devid);

	/* overallocate the buffers we're actually using because */
	scratch_bo = drm_intel_bo_alloc(bufmgr, "scratch bo", BO_SIZE, 4096);
	staging_bo = drm_intel_bo_alloc(bufmgr, "staging bo", BO_SIZE, 4096);

	drmtest_init_aperture_trashers(bufmgr);
	mappable_gtt_limit = gem_mappable_aperture_size();

	if (drmtest_run_subtest("reads"))
		test_partial_reads();

	if (drmtest_run_subtest("writes"))
		test_partial_writes();

	if (drmtest_run_subtest("writes-after-reads"))
		test_partial_read_writes();

	drmtest_cleanup_aperture_trashers();
	drm_intel_bufmgr_destroy(bufmgr);

	close(fd);

	return 0;
}
