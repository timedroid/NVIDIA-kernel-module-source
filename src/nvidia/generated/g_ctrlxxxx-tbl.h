// This file is automatically generated by rmconfig - DO NOT EDIT!
//
//
// Typedefs and #defines used to associate an rmcontrol command with its handler.
//
// This file is used in 2 ways:
//
//     #if (RMCTRL_DRIVER_FORM)
//         by rmcontrol framework, as in client.c, syscon.c, etc.
//
//     #if (RMCTRL_TEST_FORM)
//         By our rmcontrol table consistency checker..
//
// Profile:  devel-soc-disp-dce-client
// Template: templates/gt_ctrlxxxx-tbl.h
//


#ifndef _CTRLXXXX_TBL_H_
#define _CTRLXXXX_TBL_H_

#include "resserv/rs_access_rights.h"

#if !defined(RMCTRL_DRIVER_FORM)
#  define RMCTRL_DRIVER_FORM 0
#endif

#if !defined(RMCTRL_TEST_FORM)
#  define RMCTRL_TEST_FORM 0
#endif

// test form is incompatible with driver form
#if RMCTRL_DRIVER_FORM && RMCTRL_TEST_FORM
#  error "Both RMCTRL_DRIVER_FORM and RMCTRL_TEST_FORM specified!"
#endif

// rmctrl handler prototype
struct RS_RES_CONTROL_PARAMS_INTERNAL;
typedef NV_STATUS RmCtrlFunc(struct RS_RES_CONTROL_PARAMS_INTERNAL*);

struct RS_LEGACY_CONTROL_TABLE
{
    NvU32        cmd;
    NvU32        paramsSize;
    NvU32        flags;
    RS_ACCESS_MASK    rightsRequired;

#if RMCTRL_DRIVER_FORM
    RmCtrlFunc  *func;
#endif

    // This section is only defined for test program resman/config/tools/rmctrl-tbls.c
#if RMCTRL_TEST_FORM
    // when testing, we just want char names of these
    const char  *cmdName;
    const char  *paramType;
    const char  *func;
    const char  *fileName;           // __FILE__
    int          lineNumber;         // __LINE__
#endif

};
typedef struct RS_LEGACY_CONTROL_TABLE RmCtrlTableEntry;


// per-rmcontrol flags values
// NOTE: new bits added here must also be added to:
//     RMCONTROL_VALID_FLAGS list in chip-config/config/Rmcontrols.pm
//     rmctrl_flags[]             in resman/config/tools/rmctrl-tbls.c
#define RMCTRL_FLAGS_NONE                                     0x000000000
#define RMCTRL_FLAGS_NO_STATIC                                0x000000000 // internal to chip-config. TODO -- delete
#define RMCTRL_FLAGS_ONLY_IF_CMD_DEFINED                      0x000000000 // internal to chip-config. TODO -- delete
#define RMCTRL_FLAGS_KERNEL_PRIVILEGED                        0x000000000
#define RMCTRL_FLAGS_NO_GPUS_LOCK                             0x000000001
#define RMCTRL_FLAGS_NO_GPUS_ACCESS                           0x000000002
#define RMCTRL_FLAGS_PRIVILEGED                               0x000000004
#define RMCTRL_FLAGS_HACK_USED_ON_MULTIPLE_CLASSES            0x000000008
#define RMCTRL_FLAGS_NON_PRIVILEGED                           0x000000010
#define RMCTRL_FLAGS_BIG_PAYLOAD                              0x000000020
#define RMCTRL_FLAGS_GPU_LOCK_DEVICE_ONLY                     0x000000040
#define RMCTRL_FLAGS_PRIVILEGED_IF_RS_ACCESS_DISABLED         0x000000100 // for Resserv Access Rights migration
#define RMCTRL_FLAGS_ROUTE_TO_PHYSICAL                        0x000000200
#define RMCTRL_FLAGS_INTERNAL                                 0x000000400
#define RMCTRL_FLAGS_API_LOCK_READONLY                        0x000000800
#define RMCTRL_FLAGS_GPU_LOCK_READONLY                        0x000001000
#define RMCTRL_FLAGS_ROUTE_TO_VGPU_HOST                       0x000002000
#define RMCTRL_FLAGS_CACHEABLE                                0x000004000
#define RMCTRL_FLAGS_COPYOUT_ON_ERROR                         0x000008000
#define RMCTRL_FLAGS_ALLOW_WITHOUT_SYSMEM_ACCESS              0x000010000

//
// RMCTRL_HACK_* defines are used for RMCTRL_ONLY_IF_ENTRY 'cond' parameter.
// The reason is MS preprocessor doesn't expand the text in a macro, so error
// will occur if our entry like below:
// RMCTRL_ONLY_IF_ENTRY(#if defined(NV_MODS),
//                      ...............
//                      ),
// Make the RMCTRL_HACK_* defines to avoid this issue and they will be added
// into RMCONFIG module, that is, RMCFG_FEATURE_NV_MODS, so they
// will be removed soon.
//
#define RMCTRL_HACK_NV_VERIF_FEATURES 0

#define RMCTRL_HACK_NV_MODS 0

#define RMCTRL_HACK_NV_WINDOWS 0

#define RMCTRL_HACK_NV_UNIX 0
#ifdef NV_UNIX
#  undef  RMCTRL_HACK_NV_UNIX
#  define RMCTRL_HACK_NV_UNIX 1
#endif

#define RMCTRL_HACK_NV_PROFILER 0

#define RMCTRL_HACK_DEBUG_PROFILER 0
#ifdef DEBUG_PROFILER
#  undef  RMCTRL_HACK_DEBUG_PROFILER
#  define RMCTRL_HACK_DEBUG_PROFILER 1
#endif

#define RMCTRL_HACK_DEBUG 0
#ifdef DEBUG
#  undef  RMCTRL_HACK_DEBUG
#  define RMCTRL_HACK_DEBUG 1
#endif

#define RMCTRL_HACK_DEVELOP 0
#ifdef DEVELOP
#  undef  RMCTRL_HACK_DEVELOP
#  define RMCTRL_HACK_DEVELOP 1
#endif

// Define macros to help initialize the table

// only the in-kernel resman form needs handler function
#if RMCTRL_DRIVER_FORM
#  define RMCTRL_FUNC(func)      func,
#else
#  define RMCTRL_FUNC(func)
#endif

#if RMCTRL_TEST_FORM
#  define RMCTRL_TEST(rmctrlCmd,rmctrlParamType,rmctrlFunc)             \
      rmctrlCmd,                                                        \
      #rmctrlParamType,                                                 \
      #rmctrlFunc,                                                      \
      __FILE__,                                                         \
      __LINE__,
#else
#  define RMCTRL_TEST(rmctrlCmd,rmctrlParamType,rmctrlFunc)
#endif

extern RmCtrlFunc rmControlErrorCmdNotFound;           // defined in rmctrl.c

// rmctrl table entry wrapper
#if RMCTRL_TEST_FORM
#define RMCTRL_ENTRY(cond,rmctrlCmd,rmctrlFunc,rmctrlParamType,rmctrlParamSize,rmctrlFlags,rmctrlAccessRights) \
{                                                                       \
    rmctrlCmd,                                                          \
    rmctrlParamSize,                                                    \
    rmctrlFlags,                                                        \
    rmctrlAccessRights,                                                 \
    RMCTRL_FUNC(rmctrlFunc)                                             \
    RMCTRL_TEST(#rmctrlCmd, rmctrlParamType, rmctrlFunc)                \
}
#else
#define RMCTRL_ENTRY(cond,rmctrlCmd,rmctrlFunc,rmctrlParamType,rmctrlParamSize,rmctrlFlags,rmctrlAccessRights) \
{                                                                       \
    (cond) ? rmctrlCmd : 0,                                             \
    (cond) ? rmctrlParamSize : 0,                                       \
    (cond) ? rmctrlFlags : 0,                                           \
    rmctrlAccessRights,                                                 \
    RMCTRL_FUNC((cond) ? rmctrlFunc : rmControlErrorCmdNotFound)        \
    RMCTRL_TEST(#rmctrlCmd, rmctrlParamType, rmctrlFunc)                \
}
#endif

// end of table indicated by NULL rmControl function ptr.
#define RMCTRL_NULL_ENTRY      { 0, ~0, 0, RS_ACCESS_MASK_EMPTY }

// by default, add a null entry after each
#if ! defined(RMCTRL_ADD_NULL_ENTRY)
#  define RMCTRL_ADD_NULL_ENTRY 1
#endif


//
// A full table of all known rmcontrols for use by resman/config/tools/rmctrl-tbls test
//
#if RMCTRL_TEST_FORM

// hack to indicate a new "table" for a given class and category
// Uses the type name (string compare)
typedef int RMCTRL_ENTRY_NEW_TABLE;
#define TEST_ENTRY_SET_TABLE(cls,which) \
    RMCTRL_ENTRY(NV_TRUE, cls, which, RMCTRL_ENTRY_NEW_TABLE, 0, 0, RS_ACCESS_MASK_EMPTY)
#define TEST_CLASS(p)                        ((unsigned int) (p)->cmd)
#define TEST_FLAGS(p)                        ((unsigned int) (p)->rmcfgFlag)
#define TEST_IS_SET_TABLE(p)                 (0 == strcmp((p)->paramType, "RMCTRL_ENTRY_NEW_TABLE"))


// force an error for testing
#if !defined(RMCTRL_TEST_FORCE_ERROR)
#  define RMCTRL_TEST_FORCE_ERROR 0
#endif

RmCtrlTableEntry rmctrlTableEntries[] = {

#if RMCTRL_TEST_FORCE_ERROR

    TEST_ENTRY_SET_TABLE(0x0000, 0, SYSTEM),

    // force an error - duplicate entry
    RMCTRL_ENTRY(NV_TRUE,
                 NV0000_CTRL_CMD_SYSTEM_NOTIFY_EVENT,
                 NV0000_CTRL_SYSTEM_NOTIFY_EVENT_PARAMS,
                 nv0000CtrlCmdSystemNotifyEvent),

    RMCTRL_ENTRY(NV_TRUE,
                 NV0000_CTRL_CMD_SYSTEM_NOTIFY_EVENT,
                 NV0000_CTRL_SYSTEM_NOTIFY_EVENT_PARAMS,
                 nv0000CtrlCmdSystemNotifyEvent),

    // force an error - bad class
    RMCTRL_ENTRY(NV_TRUE,
                 NV0002_CTRL_CMD_UPDATE_CONTEXTDMA,
                 NV0002_CTRL_UPDATE_CONTEXTDMA_PARAMS,
                 nv0002CtrlCmdUpdateContextdma),

    TEST_ENTRY_SET_TABLE(0x0002, 0, SYSTEM),

#define NV0002_CTRL_CMD_BAD_BAD_BAD_CMD  NV0002_CTRL_CMD_UPDATE_CONTEXTDMA

    // forcing an error - same value, different name
    RMCTRL_ENTRY(NV_TRUE,
                 NV0002_CTRL_CMD_BAD_BAD_BAD_CMD,
                 NV0002_CTRL_UPDATE_CONTEXTDMA_PARAMS,
                 nv0002CtrlCmdUpdateContextdma),

#endif // RMCTRL_TEST_FORCE_ERROR




    // empty table entry at end to help w/ some of the checks
    TEST_ENTRY_SET_TABLE(0xFFFF, 0xFFFF),

};

#endif // RMCTRL_TEST_FORM


#endif // _CTRLXXXX_TBL_H_
