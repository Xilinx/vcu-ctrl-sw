/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#define YES               1
#define NO                0

#define MIPS              1
#define X86               2
#define OR1K              3
#define VSPARC            4
#define RISCV             5
#define CPU               0 // Deprecated build option - will be removed in future release - DEFAULT

#define NO_AL             0
#define CNXT_KAL          1
#define NXP_OSAL          2
#define OSAL              NO_AL // Deprecated build option - will be removed in future release - DEFAULT

#define ARM926            0
#define ARMR5             1
#define ARMA53            2
#define ARM_CPU_TYPE      ARM926 // Deprecated build option - will be removed in future release - DEFAULT

#define ADS               0
#define RVDS              1
#define GNU_MIPS          2
#define GNU_MIPS_LNX      3
#define GNU_ARM           4
#define GNU_ARM_SOURCERY  5
#define GNU_X86           6
#define WIN_X86           7
#define DS5               8
#define GNU_OR32          9
#define GNU_ARM_LINARO   10
#define GNU_OR1K         11
#define GCC_SPARC        12
#define GCC_RTEMS_SPARC  13
#define GCC_RISCV        14
#define TOOLSET           RVDS // Deprecated build option - will be removed in future release - DEFAULT

#define NO_DEBUG          0
#define BUILD_DEBUG       1
#define ARRAY_DEBUG       2
#define FULL_DEBUG        3
#define DEBUG_CAPS        BUILD_DEBUG // Build control - DEBUG mode                    - DEFAULT

#define GENTB_PLATFORM    0
#define WIN_LIB           1
#define GEN_TB_ENC        2
#define TARGET_PLATFORM   GENTB_PLATFORM // Build control                              - DEFAULT

#define USE_CACHES        1           // Build control                                 - DEFAULT
#define DIS_PIC           0           // Build control                                 - DEFAULT

#define VIDEO_TRANS       0
#define GTB_TRANS         1
#define GTB_DEC           2
#define WINDSOR_LIB       3
#define GTB_ENC           4
#define MEDIA_DEC         5
#define MEDIA_LIB         6
#define TARGET_APP        MEDIA_LIB    // Build control - Application - MediaLib       - DEFAULT

#define NONE              0
#define NUP               1
#define UCOS              2
#define UCOS3             3
#define RTOS              NONE         // Build control - use CtrlSW LibRTOS           - DEFAULT

// Target build controls - supported modes are HAPS or SIMULATION. SIMULATION is optimised for startup
//                         time so no zero init of memory and minimal trace
#define CHIP              0
#define EMULATION         1
#define HAPS              2
#define SIMULATION        3
#define CMODEL            4
#define TARGET_LEVEL      HAPS         // Build control for other formats             - DEFAULT

#define SVC_DISABLED      0
#define SVC_ENABLED       1
#define SVC_SUPPORT       SVC_DISABLED // Build control for other formats             - DEFAULT

#define MVC_DISABLED      0
#define MVC_ENABLED       1
#define MVC_SUPPORT       MVC_DISABLED // Build control for other formats             - DEFAULT

#define SFA_DISABLED      0
#define SFA_ENABLED       1
#define SFA_SUPPORT       SFA_ENABLED  // Build lib_mvd in decoupled mode             - DEFAULT

#define CNXT_HW           0
#define NXP_HW            1
#define HWLIB             NXP_HW

#define DTV               0
#define STB               1
#define PLAYMODE          DTV          // Build lib_mvd in DTV API mode               - DEFAULT

#define STANDARD          0
#define REBOOT            1
#define BOOT_ARCH         REBOOT       // Build lib_mvd in reboot mode                - DEFAULT

#define MVD_GATHER_PERF_METRICS        // Gather lib_mvd performance metrics          - DEFAULT
#define MVD_DTV_USERDATA               // Report any userdata payloads                - DEFAULT
#define MVD_WAIT_BOB_INACTIVE          // Wait for HW Bit buffer to go empty          - DEFAULT
#define MVD_NO_BSDMA_SAFETY_MARGIN     // Disable safety margin in stream feed        - DEFAULT
#define MVD_CQ_ENABLE_REFILL           // Enable CQ refill on underflow               - DEFAULT
#define MVD_SPP_HW_GOULOMB             // Enabled HW Goulomb decode in SPP            - DEFAULT
#define MVD_CQ_CQSR                    // Enable CQ Status register                   - DEFAULT
#define MVD_PERF_MEASURE               // Enable performance measurement              - DEFAULT
#define MVD_PERF_MEASURE_DCP           // Enable decoupled performance measurement    - DEFAULT
#define MVD_DCP_DYNAMIC_CONFIG         // Dynamically adjust DFE memory requirements  - DEFAULT
#define MVD_HW_SCHEDULE_LEGACY         // DecLib internal scheduler mode of operation - DEFAULT
#define MVD_HW_SCHEDULE_SEQUENTIAL     // DecLib internal scheduler mode of operation - DEFAULT
//#define MVD_HW_SCHEDULE_STAT_UPDATE_FIX
#define MVD_EARLY_DCP_ALLOC            // Allocate the decoupled memory area at stream start time - DEFAULT
#define MVD_TASK_THREAD                // Mvd task are handled in a thread

#define DECLIB_FORCE_HW_STOP           // Force stop HW on an API stop command - DEFAULT
#define DECLIB_CTX_FLUSH_AFTER_SAVE    // Flush internal DecLib ctx on save/restore - DEFAULT
#define DECLIB_SERVICE_EOS
#define DECLIB_ISR_IN_THREAD_CTX       // Handle DecLib interrupts in internal queue - DEFAULT
#define DECLIB_THREAD_CALL_DIRECT      // Handle DecLib interrupts in internal queue - DEFAULT
#define DECLIB_USER_SPACE              // Userspace build          - DEFAULT
#define DECLIB_ENABLE_DFE              // Used decoupled operation - DEFAULT
#define DECLIB_ENABLE_DBE              // Used decoupled operation - DEFAULT
#define DECLIB_ENABLE_DCP              // Used decoupled operation - DEFAULT
#define DECLIB_USE_DMA_MEM_REGION      // Specify a sperate physical memory region that is accessed by both FW and HW - DEFAULT
//#define DECLIB_DCP_SCHED_NEW

#define AXI_ENABLED                // Build lib_mvd for AXI bus HW coniguration - DEFAULT
#define PAL_CLOCK_API              // Call PAL clock controls (if implemented) - CONFIGURABLE
#define DIAG_SUPPORT_ENABLED       // Include initial diagnostics code                          - DEFAULT
#define GLOBAL_USE_RUN_TIME_CFG    // Configure lib)mvd from AL_Mvd layer                       - DEFAULT
#define GTB_SET_NUM_STREAMS        // Set number of streams at build time using parameter below - DEFAULT
#define GTB_NUM_STREAMS          2 // Number of streams the firmware is built to handle - CONFIGURABLE

#define ENABLE_PERF_TIMER          // Enable calls to performance timers, if implemented - CONFIGURABLE
#define ENABLE_TRACE_IN_RELEASE  0 // Enable trace                  - CONFIGURABLE

#define DISABLE_TRACE              // Disable verbose trace         - CONFIGURABLE
#define DISABLE_TRACE_CRITICAL     // Disable critical trace        - CONFIGURABLE

#define USE_DECODER                // Always required               - DEFAULT
#define USE_DEC_DBG                // Enable internal lib_mvd debug - CONFIGURABLE

#define FW_API_VERSION    19       // Used for tracking changes in lib_mvd API - currently stable
#define FW_VERSION         1       // Define a firmware API version, this is used in debug callbacks for tracking purposes

#define ENABLE_CRIT_SECTIONS       // Locks to prevent from concurrent accesses around shared structures

#define SPLIT_INPUT_STACK_SIZE 4   // Number of stream buffers that can be stacked in split-input mode
#define SEPARATED_BUF_REQ          // If set, we distinguish main YUV buffers from secondary YUV buffers and MBI buffers
