

#ifndef _INTEL_VLV_H_
#define _INTEL_VLV_H_

#define false 0
#define true 1

#define VLV_DISPLAY_BASE                0x180000
#define RENDER_RING_BASE                0x02000
#define BLT_RING_BASE                   0x22000
#define GFX_MODE_GEN7                   0x0229c
#define RENDER_HWS_PGA_GEN7            (0x04080)
#define BSD_HWS_PGA_GEN7               (0x04180)
#define BLT_HWS_PGA_GEN7               (0x04280)
#define GEN6_BSD_SLEEP_PSMI_CONTROL     0x12050
#define GEN6_BSD_RNCID                  0x12198
#define GEN6_BLITTER_ECOSKPD            0x221d0
#define VLV_MASTER_IER                  0x4400c /* Gunit master IER */
#define GEN6_PMIER                      0x4402C
#define VLV_IIR_RW                      0x182084
#define VLV_ISR                         0x1820ac
#define FORCEWAKE_VLV                   0x1300b0
#define FORCEWAKE_ACK_VLV               0x1300b4
#define GEN6_GDRST                      0x941c
#define _3D_CHICKEN3                    0x02090
#define IVB_CHICKEN3                    0x4200c
#define GEN7_HALF_SLICE_CHICKEN1        0xe100 /* IVB GT1  VLV */
#define GEN7_L3CNTLREG1                 0xB01C
#define GEN7_L3_CHICKEN_MODE_REGISTER   0xB030
#define GEN7_ROW_CHICKEN2               0xe4f4
#define GEN7_L3SQCREG4                  0xb034
#define GEN7_SQ_CHICKEN_MBCUNIT_CONFIG  0x9030
#define GEN6_MBCTL                      0x0907c
#define GEN6_UCGCTL2                    0x9404
#define GEN7_UCGCTL4                    0x940c
#define FENCE_REG_SANDYBRIDGE_0         0x100000
#define GEN6_BSD_RING_BASE              0x12000
#define GEN7_COMMON_SLICE_CHICKEN1      0x7010



static int IS_DISPLAYREG(uint32_t reg)
{

	if (reg >= VLV_DISPLAY_BASE)
		return false;

	if (reg >= RENDER_RING_BASE &&
                        reg < RENDER_RING_BASE + 0xff)
		return false;


	if (reg >= GEN6_BSD_RING_BASE &&
                        reg < GEN6_BSD_RING_BASE + 0xff)
		return false;

	if (reg >= BLT_RING_BASE &&
                        reg < BLT_RING_BASE + 0xff)
                return false;

	if (reg == PGTBL_ER)
                return false;

        if (reg >= IPEIR_I965 &&
                        reg < HWSTAM)
                return false;

	if (reg == MI_MODE)
                return false;

        if (reg == GFX_MODE_GEN7)
                return false;

        if (reg == RENDER_HWS_PGA_GEN7 ||
                        reg == BSD_HWS_PGA_GEN7 ||
                        reg == BLT_HWS_PGA_GEN7)
                return false;

        if (reg == GEN6_BSD_SLEEP_PSMI_CONTROL ||
                        reg == GEN6_BSD_RNCID)
                return false;

        if (reg == GEN6_BLITTER_ECOSKPD)
                return false;

        if (reg >= 0x4000c &&
                        reg <= 0x4002c)
                return false;

        if (reg >= 0x4f000 &&
                        reg <= 0x4f08f)
                return false;

        if (reg >= 0x4f100 &&
                        reg <= 0x4f11f)
                return false;

        if (reg >= VLV_MASTER_IER &&
                        reg <= GEN6_PMIER)
                return false;

	if (reg >= FENCE_REG_SANDYBRIDGE_0 &&
                        reg < (FENCE_REG_SANDYBRIDGE_0 + (16*8)))
                return false;

        if (reg >= VLV_IIR_RW &&
                        reg <= VLV_ISR)
                return false;

        if (reg == FORCEWAKE_VLV ||
                        reg == FORCEWAKE_ACK_VLV ||
                        reg == 0x130090)
                return false;

        if (reg == GEN6_GDRST)
                return false;

        if(reg > 0x9400 && reg <= 0x9418){
                return false;
        }

	  switch (reg) {
               case _3D_CHICKEN3:
               case IVB_CHICKEN3:
               case GEN7_HALF_SLICE_CHICKEN1:
               case GEN7_COMMON_SLICE_CHICKEN1:
               case GEN7_L3CNTLREG1:
               case GEN7_L3_CHICKEN_MODE_REGISTER:
               case GEN7_ROW_CHICKEN2:
               case GEN7_L3SQCREG4:
               case GEN7_SQ_CHICKEN_MBCUNIT_CONFIG:
               case GEN6_MBCTL:
               case GEN6_UCGCTL2:
               case GEN7_UCGCTL4:
                      return false;
               default:
                      break;
        }

        return true;
}

#endif
