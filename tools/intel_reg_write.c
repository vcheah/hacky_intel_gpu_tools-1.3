/*
 * Copyright © 2007 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Ben Gamari <bgamari.foss@gmail.com>
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include "intel_gpu_tools.h"
#include "intel_vlv.h"

int main(int argc, char** argv)
{
	uint32_t reg, value;
	volatile uint32_t *ptr,offset;
	struct pci_device *pci_dev;

	if (argc < 3) {
		printf("Usage: %s addr value\n", argv[0]);
		printf("  WARNING: This is dangerous to you and your system's health.\n");
		printf("           Only for use in debugging.\n");
		exit(1);
	}

	intel_register_access_init(intel_get_pci_device(), 0);
	pci_dev = intel_get_pci_device();
	sscanf(argv[1], "0x%x", &reg);
	sscanf(argv[2], "0x%x", &value);

	if (IS_VALLEYVIEW(pci_dev->device_id)) {
                if (IS_DISPLAYREG(reg))
                        offset = 0x180000;
		else
			offset = 0x0; 
        }

	ptr = (volatile uint32_t *)((volatile char *)mmio + reg + offset);

	printf("Value before: 0x%X\n", *ptr);
	*ptr = value;
	printf("Value after: 0x%X\n", *ptr);

	intel_register_access_fini();
	return 0;
}

