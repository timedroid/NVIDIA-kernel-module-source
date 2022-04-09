#ifndef _G_OBJTMR_NVOC_H_
#define _G_OBJTMR_NVOC_H_
#include "nvoc/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SPDX-FileCopyrightText: Copyright (c) 1993-2021 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "g_objtmr_nvoc.h"

#ifndef _OBJTMR_H_
#define _OBJTMR_H_

/*!
 * @file
 * @brief   Defines and structures used for the Tmr Engine Object.
 */

/* ------------------------ Includes --------------------------------------- */
#include "core/core.h"
#include "core/info_block.h"
#include "gpu/eng_state.h"
#include "gpu/gpu.h"
#include "tmr.h"
#include "lib/ref_count.h"
#include "os/os.h"
#include "nvoc/utility.h"

/* ------------------------ Macros ----------------------------------------- */
//
// Extent of the timer callback array
//
#define TMR_NUM_CALLBACKS_RM                96
#define TMR_NUM_CALLBACKS_OS                36

// Callback scheduled without any explicit flags set.
#define TMR_FLAGS_NONE              0x00000000
// Automatically reschedule the callback, so that it repeats.
// Otherwise, callback is scheduled for one-shot execution.
#define TMR_FLAG_RECUR              NVBIT(0)
// Indicate that the implementation of the callback function will/can release
// a GPU semaphore. This allows fifoIdleChannels to query this information,
// and hence not bail out early if channels are blocked on semaphores that
// will in fact be released.
 // !!NOTE: This is OBSOLETE, it should be moved directly to FIFO, where it's needed
#define TMR_FLAG_RELEASE_SEMAPHORE  NVBIT(1)
#define TMR_FLAG_OS_TIMER_QUEUED    NVBIT(2)

#define TMR_GET_GPU(p)   ENG_GET_GPU(p)

/* ------------------------ Function Redefinitions ------------------------- */
#define tmrEventScheduleRelSec(pTmr, pEvent, RelTimeSec) tmrEventScheduleRel(pTmr, pEvent, (NvU64)(RelTimeSec) * 1000000000 )

#define tmrGetInfoBlock(pTmr, pListHead, dataId)         getInfoPtr(pListHead, dataId)
#define tmrAddInfoBlock(pTmr, ppListHead, dataId, size)  addInfoPtr(ppListHead, dataId, size)
#define tmrDeleteInfoBlock(pTmr, ppListHead, dataId)     deleteInfoPtr(ppListHead, dataId)
#define tmrTestInfoBlock(pTmr, pListHead, dataId)        testInfoPtr(pListHead, dataId)

/* ------------------------ Datatypes -------------------------------------- */
TYPEDEF_BITVECTOR(MC_ENGINE_BITVECTOR);

//
// Forward references for timer related structures
//
typedef struct DAYMSECTIME      *PDAYMSECTIME;
typedef struct DAYMSECTIME      DAYMSECTIME;

//
// System time structure
//
struct DAYMSECTIME
{
    NvU32 days;
    NvU32 msecs;
    NvU32 valid;
};

/*!
 * Callback wrapper memory type, used with interfacing all scheduling functions
 * Reveals only partial representation of the event information.
 * User Use only, internal code will not change them.
 */
struct TMR_EVENT
{
    TIMEPROC        pTimeProc;    //<! The callback function for the event.
    void *          pUserData;    //<! Special object used to associate event with.
                                  //<! It will be given to the callback inside this struct.
    void *          pOSTmrCBdata; //<! This parameter holds the data of OS registered timer

    NvU32           flags;
    NvU32           chId;         //<! OBSOLETE, semaphore should be handled by FIFO directly
                                  //<! Used only with TMR_FLAG_RELEASE_SEMAPHORE, this is
                                  //<! soon to be obsoleted, it won't be necessary with a free
                                  //<! object for callbacks to manipulate.
};

/*!
 * Internal representation of the wrapper memory type.
 * Casted from public user datatype, so they have access to some fields only.
 * Internal Use only, will be hidden in time.c when obsolete tasks have migrated.
 */
typedef struct TMR_EVENT_PVT TMR_EVENT_PVT, *PTMR_EVENT_PVT;
struct TMR_EVENT_PVT
{
    // Public interface, must come first, don't declare anything above this field.
    TMR_EVENT           super;

    // Legacy Fields, soon to be obsoleted.
    NvBool              bLegacy;    //<! Used to mark legacy mode, controls which of the
                                    //<! two callbacks will called.
    TIMEPROC_OBSOLETE   pTimeProc_OBSOLETE;

    NvBool              bInUse;     //<! Marks this as currently used
    NvU64               timens;     //<! Absolute time to perform callback
    PTMR_EVENT_PVT      pNext;      //<! Next element in the list
};

/*!
 * Struct to pass to event creation and updates
 * as it holds all the relevant data.
 */
typedef struct TMR_EVENT_SET_PARAMS {
    NV_DECLARE_ALIGNED(NvP64 *ppEvent, 8);
    NV_DECLARE_ALIGNED(NvP64 pTimeProc, 8);
    NV_DECLARE_ALIGNED(NvP64 pCallbackData, 8);
    NvU32 flags;
} TMR_EVENT_SET_PARAMS;

/*!
 * Struct to pass to scheduling function.
 *
 * Takes a flag for schedule type based on the above
 * SCHEDULE_FLAGS.
 */
typedef struct TMR_EVENT_SCHEDULE_PARAMS {
    NV_DECLARE_ALIGNED(NvP64 pEvent, 8);
    NV_DECLARE_ALIGNED(NvU64 timeNs, 8);
    NvBool bUseTimeAbs;
} TMR_EVENT_SCHEDULE_PARAMS;

/*!
 * Struct to pass to cancel, destroy and
 * get-user-data commands.
 */
typedef struct TMR_EVENT_GENERAL_PARAMS {
    NV_DECLARE_ALIGNED(NvP64 pEvent, 8);
    NV_DECLARE_ALIGNED(NvP64 returnVal, 8);
} TMR_EVENT_GENERAL_PARAMS;

/*!
 * Timer object itself
 */
struct OBJTMR {
    const struct NVOC_RTTI *__nvoc_rtti;
    struct OBJENGSTATE __nvoc_base_OBJENGSTATE;
    struct Object *__nvoc_pbase_Object;
    struct OBJENGSTATE *__nvoc_pbase_OBJENGSTATE;
    struct OBJTMR *__nvoc_pbase_OBJTMR;
    NV_STATUS (*__tmrConstructEngine__)(struct OBJGPU *, POBJTMR, ENGDESCRIPTOR);
    NV_STATUS (*__tmrStateInitLocked__)(struct OBJGPU *, POBJTMR);
    NV_STATUS (*__tmrStateLoad__)(struct OBJGPU *, POBJTMR, NvU32);
    NV_STATUS (*__tmrStateUnload__)(struct OBJGPU *, POBJTMR, NvU32);
    void (*__tmrStateDestroy__)(struct OBJGPU *, POBJTMR);
    NV_STATUS (*__tmrReconcileTunableState__)(POBJGPU, struct OBJTMR *, void *);
    NV_STATUS (*__tmrStatePreLoad__)(POBJGPU, struct OBJTMR *, NvU32);
    NV_STATUS (*__tmrStatePostUnload__)(POBJGPU, struct OBJTMR *, NvU32);
    NV_STATUS (*__tmrStatePreUnload__)(POBJGPU, struct OBJTMR *, NvU32);
    NV_STATUS (*__tmrStateInitUnlocked__)(POBJGPU, struct OBJTMR *);
    void (*__tmrInitMissing__)(POBJGPU, struct OBJTMR *);
    NV_STATUS (*__tmrStatePreInitLocked__)(POBJGPU, struct OBJTMR *);
    NV_STATUS (*__tmrStatePreInitUnlocked__)(POBJGPU, struct OBJTMR *);
    NV_STATUS (*__tmrGetTunableState__)(POBJGPU, struct OBJTMR *, void *);
    NV_STATUS (*__tmrCompareTunableState__)(POBJGPU, struct OBJTMR *, void *, void *);
    void (*__tmrFreeTunableState__)(POBJGPU, struct OBJTMR *, void *);
    NV_STATUS (*__tmrStatePostLoad__)(POBJGPU, struct OBJTMR *, NvU32);
    NV_STATUS (*__tmrAllocTunableState__)(POBJGPU, struct OBJTMR *, void **);
    NV_STATUS (*__tmrSetTunableState__)(POBJGPU, struct OBJTMR *, void *);
    NvBool (*__tmrIsPresent__)(POBJGPU, struct OBJTMR *);
    NvBool PDB_PROP_TMR_USE_COUNTDOWN_TIMER_FOR_RM_CALLBACKS;
    NvBool PDB_PROP_TMR_ALARM_INTR_REMOVED_FROM_PMC_TREE;
    NvBool PDB_PROP_TMR_USE_OS_TIMER_FOR_CALLBACKS;
    NvBool PDB_PROP_TMR_USE_PTIMER_FOR_OSTIMER_CALLBACKS;
    NvBool PDB_PROP_TMR_USE_POLLING_FOR_CALLBACKS;
    NvBool PDB_PROP_TMR_USE_SECOND_COUNTDOWN_TIMER_FOR_SWRL;
    PTMR_EVENT_PVT pRmActiveEventList;
    PTMR_EVENT_PVT pRmCallbackFreeList_OBSOLETE;
    struct TMR_EVENT_PVT rmCallbackTable_OBSOLETE[96];
    POS1HZTIMERENTRY pOs1HzCallbackList;
    POS1HZTIMERENTRY pOs1HzCallbackFreeList;
    struct OS1HZTIMERENTRY os1HzCallbackTable[36];
    struct TMR_EVENT *pOs1HzEvent;
    PORT_SPINLOCK *pTmrSwrlLock;
    TIMEPROC_COUNTDOWN pSwrlCallback;
    NvU32 retryTimes;
    NvU32 errorCount;
    volatile NvS32 tmrChangePending;
    NvBool bInitialized;
    NvBool bAlarmIntrEnabled;
    PENG_INFO_LINK_NODE infoList;
    POBJREFCNT pGrTickFreqRefcnt;
};

#ifndef __NVOC_CLASS_OBJTMR_TYPEDEF__
#define __NVOC_CLASS_OBJTMR_TYPEDEF__
typedef struct OBJTMR OBJTMR;
#endif /* __NVOC_CLASS_OBJTMR_TYPEDEF__ */

#ifndef __nvoc_class_id_OBJTMR
#define __nvoc_class_id_OBJTMR 0x9ddede
#endif /* __nvoc_class_id_OBJTMR */

extern const struct NVOC_CLASS_DEF __nvoc_class_def_OBJTMR;

#define __staticCast_OBJTMR(pThis) \
    ((pThis)->__nvoc_pbase_OBJTMR)

#ifdef __nvoc_objtmr_h_disabled
#define __dynamicCast_OBJTMR(pThis) ((OBJTMR*)NULL)
#else //__nvoc_objtmr_h_disabled
#define __dynamicCast_OBJTMR(pThis) \
    ((OBJTMR*)__nvoc_dynamicCast(staticCast((pThis), Dynamic), classInfo(OBJTMR)))
#endif //__nvoc_objtmr_h_disabled

#define PDB_PROP_TMR_USE_COUNTDOWN_TIMER_FOR_RM_CALLBACKS_BASE_CAST
#define PDB_PROP_TMR_USE_COUNTDOWN_TIMER_FOR_RM_CALLBACKS_BASE_NAME PDB_PROP_TMR_USE_COUNTDOWN_TIMER_FOR_RM_CALLBACKS
#define PDB_PROP_TMR_USE_OS_TIMER_FOR_CALLBACKS_BASE_CAST
#define PDB_PROP_TMR_USE_OS_TIMER_FOR_CALLBACKS_BASE_NAME PDB_PROP_TMR_USE_OS_TIMER_FOR_CALLBACKS
#define PDB_PROP_TMR_USE_PTIMER_FOR_OSTIMER_CALLBACKS_BASE_CAST
#define PDB_PROP_TMR_USE_PTIMER_FOR_OSTIMER_CALLBACKS_BASE_NAME PDB_PROP_TMR_USE_PTIMER_FOR_OSTIMER_CALLBACKS
#define PDB_PROP_TMR_USE_SECOND_COUNTDOWN_TIMER_FOR_SWRL_BASE_CAST
#define PDB_PROP_TMR_USE_SECOND_COUNTDOWN_TIMER_FOR_SWRL_BASE_NAME PDB_PROP_TMR_USE_SECOND_COUNTDOWN_TIMER_FOR_SWRL
#define PDB_PROP_TMR_USE_POLLING_FOR_CALLBACKS_BASE_CAST
#define PDB_PROP_TMR_USE_POLLING_FOR_CALLBACKS_BASE_NAME PDB_PROP_TMR_USE_POLLING_FOR_CALLBACKS
#define PDB_PROP_TMR_IS_MISSING_BASE_CAST __nvoc_base_OBJENGSTATE.
#define PDB_PROP_TMR_IS_MISSING_BASE_NAME PDB_PROP_ENGSTATE_IS_MISSING
#define PDB_PROP_TMR_ALARM_INTR_REMOVED_FROM_PMC_TREE_BASE_CAST
#define PDB_PROP_TMR_ALARM_INTR_REMOVED_FROM_PMC_TREE_BASE_NAME PDB_PROP_TMR_ALARM_INTR_REMOVED_FROM_PMC_TREE

NV_STATUS __nvoc_objCreateDynamic_OBJTMR(OBJTMR**, Dynamic*, NvU32, va_list);

NV_STATUS __nvoc_objCreate_OBJTMR(OBJTMR**, Dynamic*, NvU32);
#define __objCreate_OBJTMR(ppNewObj, pParent, createFlags) \
    __nvoc_objCreate_OBJTMR((ppNewObj), staticCast((pParent), Dynamic), (createFlags))

#define tmrConstructEngine(pGpu, pTmr, arg0) tmrConstructEngine_DISPATCH(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#define tmrStateInitLocked(pGpu, pTmr) tmrStateInitLocked_DISPATCH(pGpu, __staticCast_OBJTMR(pTmr))
#define tmrStateLoad(pGpu, pTmr, arg0) tmrStateLoad_DISPATCH(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#define tmrStateUnload(pGpu, pTmr, arg0) tmrStateUnload_DISPATCH(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#define tmrStateDestroy(pGpu, pTmr) tmrStateDestroy_DISPATCH(pGpu, __staticCast_OBJTMR(pTmr))
#define tmrReconcileTunableState(pGpu, pEngstate, pTunableState) tmrReconcileTunableState_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), pTunableState)
#define tmrStatePreLoad(pGpu, pEngstate, arg0) tmrStatePreLoad_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), arg0)
#define tmrStatePostUnload(pGpu, pEngstate, arg0) tmrStatePostUnload_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), arg0)
#define tmrStatePreUnload(pGpu, pEngstate, arg0) tmrStatePreUnload_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), arg0)
#define tmrStateInitUnlocked(pGpu, pEngstate) tmrStateInitUnlocked_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate))
#define tmrInitMissing(pGpu, pEngstate) tmrInitMissing_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate))
#define tmrStatePreInitLocked(pGpu, pEngstate) tmrStatePreInitLocked_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate))
#define tmrStatePreInitUnlocked(pGpu, pEngstate) tmrStatePreInitUnlocked_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate))
#define tmrGetTunableState(pGpu, pEngstate, pTunableState) tmrGetTunableState_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), pTunableState)
#define tmrCompareTunableState(pGpu, pEngstate, pTunables1, pTunables2) tmrCompareTunableState_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), pTunables1, pTunables2)
#define tmrFreeTunableState(pGpu, pEngstate, pTunableState) tmrFreeTunableState_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), pTunableState)
#define tmrStatePostLoad(pGpu, pEngstate, arg0) tmrStatePostLoad_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), arg0)
#define tmrAllocTunableState(pGpu, pEngstate, ppTunableState) tmrAllocTunableState_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), ppTunableState)
#define tmrSetTunableState(pGpu, pEngstate, pTunableState) tmrSetTunableState_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate), pTunableState)
#define tmrIsPresent(pGpu, pEngstate) tmrIsPresent_DISPATCH(pGpu, __staticCast_OBJTMR(pEngstate))
NV_STATUS tmrGetCurrentTime_IMPL(POBJTMR pTmr, NvU64 *pTime);

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrGetCurrentTime(POBJTMR pTmr, NvU64 *pTime) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetCurrentTime(pTmr, pTime) tmrGetCurrentTime_IMPL(__staticCast_OBJTMR(pTmr), pTime)
#endif //__nvoc_objtmr_h_disabled

#define tmrGetCurrentTime_HAL(pTmr, pTime) tmrGetCurrentTime(__staticCast_OBJTMR(pTmr), pTime)

NV_STATUS tmrGetCurrentTimeEx_IMPL(POBJTMR pTmr, NvU64 *pTime, THREAD_STATE_NODE *arg0);

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrGetCurrentTimeEx(POBJTMR pTmr, NvU64 *pTime, THREAD_STATE_NODE *arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetCurrentTimeEx(pTmr, pTime, arg0) tmrGetCurrentTimeEx_IMPL(__staticCast_OBJTMR(pTmr), pTime, arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrGetCurrentTimeEx_HAL(pTmr, pTime, arg0) tmrGetCurrentTimeEx(__staticCast_OBJTMR(pTmr), pTime, arg0)

static inline NV_STATUS tmrSetCurrentTime_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrSetCurrentTime(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetCurrentTime(pGpu, pTmr) tmrSetCurrentTime_46f6a7(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

#define tmrSetCurrentTime_HAL(pGpu, pTmr) tmrSetCurrentTime(pGpu, __staticCast_OBJTMR(pTmr))

NvBool tmrIsEnabled_OSTIMER(struct OBJGPU *pGpu, POBJTMR pTmr);

#ifdef __nvoc_objtmr_h_disabled
static inline NvBool tmrIsEnabled(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_FALSE;
}
#else //__nvoc_objtmr_h_disabled
#define tmrIsEnabled(pGpu, pTmr) tmrIsEnabled_OSTIMER(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

#define tmrIsEnabled_HAL(pGpu, pTmr) tmrIsEnabled(pGpu, __staticCast_OBJTMR(pTmr))

static inline NV_STATUS tmrSetAlarmIntrDisable_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrSetAlarmIntrDisable(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetAlarmIntrDisable(pGpu, pTmr) tmrSetAlarmIntrDisable_46f6a7(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

#define tmrSetAlarmIntrDisable_HAL(pGpu, pTmr) tmrSetAlarmIntrDisable(pGpu, __staticCast_OBJTMR(pTmr))

static inline NV_STATUS tmrSetAlarmIntrEnable_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrSetAlarmIntrEnable(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetAlarmIntrEnable(pGpu, pTmr) tmrSetAlarmIntrEnable_46f6a7(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

#define tmrSetAlarmIntrEnable_HAL(pGpu, pTmr) tmrSetAlarmIntrEnable(pGpu, __staticCast_OBJTMR(pTmr))

static inline NV_STATUS tmrSetAlarmIntrReset_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrSetAlarmIntrReset(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetAlarmIntrReset(pGpu, pTmr, arg0) tmrSetAlarmIntrReset_46f6a7(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrSetAlarmIntrReset_HAL(pGpu, pTmr, arg0) tmrSetAlarmIntrReset(pGpu, __staticCast_OBJTMR(pTmr), arg0)

NV_STATUS tmrGetIntrStatus_OSTIMER(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 *pStatus, THREAD_STATE_NODE *arg0);

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrGetIntrStatus(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 *pStatus, THREAD_STATE_NODE *arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetIntrStatus(pGpu, pTmr, pStatus, arg0) tmrGetIntrStatus_OSTIMER(pGpu, __staticCast_OBJTMR(pTmr), pStatus, arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrGetIntrStatus_HAL(pGpu, pTmr, pStatus, arg0) tmrGetIntrStatus(pGpu, __staticCast_OBJTMR(pTmr), pStatus, arg0)

NvU32 tmrGetTimeLo_OSTIMER(struct OBJGPU *pGpu, POBJTMR pTmr);

#ifdef __nvoc_objtmr_h_disabled
static inline NvU32 tmrGetTimeLo(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return 0;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetTimeLo(pGpu, pTmr) tmrGetTimeLo_OSTIMER(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

#define tmrGetTimeLo_HAL(pGpu, pTmr) tmrGetTimeLo(pGpu, __staticCast_OBJTMR(pTmr))

NvU64 tmrGetTime_OSTIMER(struct OBJGPU *pGpu, POBJTMR pTmr);

#ifdef __nvoc_objtmr_h_disabled
static inline NvU64 tmrGetTime(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return 0;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetTime(pGpu, pTmr) tmrGetTime_OSTIMER(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

#define tmrGetTime_HAL(pGpu, pTmr) tmrGetTime(pGpu, __staticCast_OBJTMR(pTmr))

NvU64 tmrGetTimeEx_OSTIMER(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0);

#ifdef __nvoc_objtmr_h_disabled
static inline NvU64 tmrGetTimeEx(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return 0;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetTimeEx(pGpu, pTmr, arg0) tmrGetTimeEx_OSTIMER(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrGetTimeEx_HAL(pGpu, pTmr, arg0) tmrGetTimeEx(pGpu, __staticCast_OBJTMR(pTmr), arg0)

NvU32 tmrReadTimeLoReg_OSTIMER(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0);

#ifdef __nvoc_objtmr_h_disabled
static inline NvU32 tmrReadTimeLoReg(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return 0;
}
#else //__nvoc_objtmr_h_disabled
#define tmrReadTimeLoReg(pGpu, pTmr, arg0) tmrReadTimeLoReg_OSTIMER(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrReadTimeLoReg_HAL(pGpu, pTmr, arg0) tmrReadTimeLoReg(pGpu, __staticCast_OBJTMR(pTmr), arg0)

NvU32 tmrReadTimeHiReg_OSTIMER(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0);

#ifdef __nvoc_objtmr_h_disabled
static inline NvU32 tmrReadTimeHiReg(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return 0;
}
#else //__nvoc_objtmr_h_disabled
#define tmrReadTimeHiReg(pGpu, pTmr, arg0) tmrReadTimeHiReg_OSTIMER(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrReadTimeHiReg_HAL(pGpu, pTmr, arg0) tmrReadTimeHiReg(pGpu, __staticCast_OBJTMR(pTmr), arg0)

static inline NV_STATUS tmrSetAlarm_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr, NvU64 alarm, THREAD_STATE_NODE *pThreadState) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrSetAlarm(struct OBJGPU *pGpu, POBJTMR pTmr, NvU64 alarm, THREAD_STATE_NODE *pThreadState) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetAlarm(pGpu, pTmr, alarm, pThreadState) tmrSetAlarm_46f6a7(pGpu, __staticCast_OBJTMR(pTmr), alarm, pThreadState)
#endif //__nvoc_objtmr_h_disabled

#define tmrSetAlarm_HAL(pGpu, pTmr, alarm, pThreadState) tmrSetAlarm(pGpu, __staticCast_OBJTMR(pTmr), alarm, pThreadState)

static inline NvBool tmrGetAlarmPending_491d52(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    return ((NvBool)(0 != 0));
}

#ifdef __nvoc_objtmr_h_disabled
static inline NvBool tmrGetAlarmPending(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_FALSE;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetAlarmPending(pGpu, pTmr, arg0) tmrGetAlarmPending_491d52(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrGetAlarmPending_HAL(pGpu, pTmr, arg0) tmrGetAlarmPending(pGpu, __staticCast_OBJTMR(pTmr), arg0)

static inline NV_STATUS tmrSetCountdownIntrDisable_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrSetCountdownIntrDisable(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetCountdownIntrDisable(pGpu, pTmr) tmrSetCountdownIntrDisable_46f6a7(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

#define tmrSetCountdownIntrDisable_HAL(pGpu, pTmr) tmrSetCountdownIntrDisable(pGpu, __staticCast_OBJTMR(pTmr))

static inline NV_STATUS tmrSetCountdownIntrEnable_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrSetCountdownIntrEnable(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetCountdownIntrEnable(pGpu, pTmr) tmrSetCountdownIntrEnable_46f6a7(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

#define tmrSetCountdownIntrEnable_HAL(pGpu, pTmr) tmrSetCountdownIntrEnable(pGpu, __staticCast_OBJTMR(pTmr))

static inline NV_STATUS tmrSetCountdownIntrReset_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrSetCountdownIntrReset(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetCountdownIntrReset(pGpu, pTmr, arg0) tmrSetCountdownIntrReset_46f6a7(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrSetCountdownIntrReset_HAL(pGpu, pTmr, arg0) tmrSetCountdownIntrReset(pGpu, __staticCast_OBJTMR(pTmr), arg0)

static inline NvBool tmrGetCountdownPending_491d52(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    return ((NvBool)(0 != 0));
}

#ifdef __nvoc_objtmr_h_disabled
static inline NvBool tmrGetCountdownPending(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_FALSE;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetCountdownPending(pGpu, pTmr, arg0) tmrGetCountdownPending_491d52(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrGetCountdownPending_HAL(pGpu, pTmr, arg0) tmrGetCountdownPending(pGpu, __staticCast_OBJTMR(pTmr), arg0)

static inline NV_STATUS tmrSetCountdown_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 arg0, NvU32 arg1, THREAD_STATE_NODE *arg2) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrSetCountdown(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 arg0, NvU32 arg1, THREAD_STATE_NODE *arg2) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetCountdown(pGpu, pTmr, arg0, arg1, arg2) tmrSetCountdown_46f6a7(pGpu, __staticCast_OBJTMR(pTmr), arg0, arg1, arg2)
#endif //__nvoc_objtmr_h_disabled

#define tmrSetCountdown_HAL(pGpu, pTmr, arg0, arg1, arg2) tmrSetCountdown(pGpu, __staticCast_OBJTMR(pTmr), arg0, arg1, arg2)

static inline NV_STATUS tmrGetTimerBar0MapInfo_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr, NvU64 *arg0, NvU32 *arg1) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrGetTimerBar0MapInfo(struct OBJGPU *pGpu, POBJTMR pTmr, NvU64 *arg0, NvU32 *arg1) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetTimerBar0MapInfo(pGpu, pTmr, arg0, arg1) tmrGetTimerBar0MapInfo_46f6a7(pGpu, __staticCast_OBJTMR(pTmr), arg0, arg1)
#endif //__nvoc_objtmr_h_disabled

#define tmrGetTimerBar0MapInfo_HAL(pGpu, pTmr, arg0, arg1) tmrGetTimerBar0MapInfo(pGpu, __staticCast_OBJTMR(pTmr), arg0, arg1)

static inline NV_STATUS tmrGrTickFreqChange_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr, NvBool arg0) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrGrTickFreqChange(struct OBJGPU *pGpu, POBJTMR pTmr, NvBool arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGrTickFreqChange(pGpu, pTmr, arg0) tmrGrTickFreqChange_46f6a7(pGpu, __staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

#define tmrGrTickFreqChange_HAL(pGpu, pTmr, arg0) tmrGrTickFreqChange(pGpu, __staticCast_OBJTMR(pTmr), arg0)

static inline NvU32 tmrGetUtilsClkScaleFactor_4a4dee(struct OBJGPU *pGpu, POBJTMR pTmr) {
    return 0;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NvU32 tmrGetUtilsClkScaleFactor(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return 0;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetUtilsClkScaleFactor(pGpu, pTmr) tmrGetUtilsClkScaleFactor_4a4dee(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

#define tmrGetUtilsClkScaleFactor_HAL(pGpu, pTmr) tmrGetUtilsClkScaleFactor(pGpu, __staticCast_OBJTMR(pTmr))

NV_STATUS tmrGetGpuAndCpuTimestampPair_OSTIMER(struct OBJGPU *pGpu, POBJTMR pTmr, NvU64 *arg0, NvU64 *arg1);

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrGetGpuAndCpuTimestampPair(struct OBJGPU *pGpu, POBJTMR pTmr, NvU64 *arg0, NvU64 *arg1) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetGpuAndCpuTimestampPair(pGpu, pTmr, arg0, arg1) tmrGetGpuAndCpuTimestampPair_OSTIMER(pGpu, __staticCast_OBJTMR(pTmr), arg0, arg1)
#endif //__nvoc_objtmr_h_disabled

#define tmrGetGpuAndCpuTimestampPair_HAL(pGpu, pTmr, arg0, arg1) tmrGetGpuAndCpuTimestampPair(pGpu, __staticCast_OBJTMR(pTmr), arg0, arg1)

static inline NV_STATUS tmrGetGpuPtimerOffset_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 *arg0, NvU32 *arg1) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrGetGpuPtimerOffset(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 *arg0, NvU32 *arg1) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetGpuPtimerOffset(pGpu, pTmr, arg0, arg1) tmrGetGpuPtimerOffset_46f6a7(pGpu, __staticCast_OBJTMR(pTmr), arg0, arg1)
#endif //__nvoc_objtmr_h_disabled

#define tmrGetGpuPtimerOffset_HAL(pGpu, pTmr, arg0, arg1) tmrGetGpuPtimerOffset(pGpu, __staticCast_OBJTMR(pTmr), arg0, arg1)

static inline void tmrResetTimerRegistersForVF_b3696a(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 gfid) {
    return;
}

#ifdef __nvoc_objtmr_h_disabled
static inline void tmrResetTimerRegistersForVF(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 gfid) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
}
#else //__nvoc_objtmr_h_disabled
#define tmrResetTimerRegistersForVF(pGpu, pTmr, gfid) tmrResetTimerRegistersForVF_b3696a(pGpu, __staticCast_OBJTMR(pTmr), gfid)
#endif //__nvoc_objtmr_h_disabled

#define tmrResetTimerRegistersForVF_HAL(pGpu, pTmr, gfid) tmrResetTimerRegistersForVF(pGpu, __staticCast_OBJTMR(pTmr), gfid)

static inline NV_STATUS tmrEventCreateOSTimer_46f6a7(POBJTMR pTmr, PTMR_EVENT pEvent) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrEventCreateOSTimer(POBJTMR pTmr, PTMR_EVENT pEvent) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventCreateOSTimer(pTmr, pEvent) tmrEventCreateOSTimer_46f6a7(__staticCast_OBJTMR(pTmr), pEvent)
#endif //__nvoc_objtmr_h_disabled

#define tmrEventCreateOSTimer_HAL(pTmr, pEvent) tmrEventCreateOSTimer(__staticCast_OBJTMR(pTmr), pEvent)

static inline NV_STATUS tmrEventScheduleAbsOSTimer_46f6a7(POBJTMR pTmr, PTMR_EVENT pEvent, NvU64 timeAbs) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrEventScheduleAbsOSTimer(POBJTMR pTmr, PTMR_EVENT pEvent, NvU64 timeAbs) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventScheduleAbsOSTimer(pTmr, pEvent, timeAbs) tmrEventScheduleAbsOSTimer_46f6a7(__staticCast_OBJTMR(pTmr), pEvent, timeAbs)
#endif //__nvoc_objtmr_h_disabled

#define tmrEventScheduleAbsOSTimer_HAL(pTmr, pEvent, timeAbs) tmrEventScheduleAbsOSTimer(__staticCast_OBJTMR(pTmr), pEvent, timeAbs)

static inline NV_STATUS tmrEventServiceOSTimerCallback_46f6a7(struct OBJGPU *pGpu, POBJTMR pTmr, PTMR_EVENT pEvent) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrEventServiceOSTimerCallback(struct OBJGPU *pGpu, POBJTMR pTmr, PTMR_EVENT pEvent) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventServiceOSTimerCallback(pGpu, pTmr, pEvent) tmrEventServiceOSTimerCallback_46f6a7(pGpu, __staticCast_OBJTMR(pTmr), pEvent)
#endif //__nvoc_objtmr_h_disabled

#define tmrEventServiceOSTimerCallback_HAL(pGpu, pTmr, pEvent) tmrEventServiceOSTimerCallback(pGpu, __staticCast_OBJTMR(pTmr), pEvent)

static inline NV_STATUS tmrEventCancelOSTimer_46f6a7(POBJTMR pTmr, PTMR_EVENT pEvent) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrEventCancelOSTimer(POBJTMR pTmr, PTMR_EVENT pEvent) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventCancelOSTimer(pTmr, pEvent) tmrEventCancelOSTimer_46f6a7(__staticCast_OBJTMR(pTmr), pEvent)
#endif //__nvoc_objtmr_h_disabled

#define tmrEventCancelOSTimer_HAL(pTmr, pEvent) tmrEventCancelOSTimer(__staticCast_OBJTMR(pTmr), pEvent)

static inline NV_STATUS tmrEventDestroyOSTimer_46f6a7(POBJTMR pTmr, PTMR_EVENT pEvent) {
    return NV_ERR_NOT_SUPPORTED;
}

#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrEventDestroyOSTimer(POBJTMR pTmr, PTMR_EVENT pEvent) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventDestroyOSTimer(pTmr, pEvent) tmrEventDestroyOSTimer_46f6a7(__staticCast_OBJTMR(pTmr), pEvent)
#endif //__nvoc_objtmr_h_disabled

#define tmrEventDestroyOSTimer_HAL(pTmr, pEvent) tmrEventDestroyOSTimer(__staticCast_OBJTMR(pTmr), pEvent)

NV_STATUS tmrConstructEngine_IMPL(struct OBJGPU *pGpu, POBJTMR pTmr, ENGDESCRIPTOR arg0);

static inline NV_STATUS tmrConstructEngine_DISPATCH(struct OBJGPU *pGpu, POBJTMR pTmr, ENGDESCRIPTOR arg0) {
    return pTmr->__tmrConstructEngine__(pGpu, pTmr, arg0);
}

NV_STATUS tmrStateInitLocked_IMPL(struct OBJGPU *pGpu, POBJTMR pTmr);

static inline NV_STATUS tmrStateInitLocked_DISPATCH(struct OBJGPU *pGpu, POBJTMR pTmr) {
    return pTmr->__tmrStateInitLocked__(pGpu, pTmr);
}

NV_STATUS tmrStateLoad_IMPL(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 arg0);

static inline NV_STATUS tmrStateLoad_DISPATCH(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 arg0) {
    return pTmr->__tmrStateLoad__(pGpu, pTmr, arg0);
}

NV_STATUS tmrStateUnload_IMPL(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 arg0);

static inline NV_STATUS tmrStateUnload_DISPATCH(struct OBJGPU *pGpu, POBJTMR pTmr, NvU32 arg0) {
    return pTmr->__tmrStateUnload__(pGpu, pTmr, arg0);
}

void tmrStateDestroy_IMPL(struct OBJGPU *pGpu, POBJTMR pTmr);

static inline void tmrStateDestroy_DISPATCH(struct OBJGPU *pGpu, POBJTMR pTmr) {
    pTmr->__tmrStateDestroy__(pGpu, pTmr);
}

static inline NV_STATUS tmrReconcileTunableState_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, void *pTunableState) {
    return pEngstate->__tmrReconcileTunableState__(pGpu, pEngstate, pTunableState);
}

static inline NV_STATUS tmrStatePreLoad_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, NvU32 arg0) {
    return pEngstate->__tmrStatePreLoad__(pGpu, pEngstate, arg0);
}

static inline NV_STATUS tmrStatePostUnload_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, NvU32 arg0) {
    return pEngstate->__tmrStatePostUnload__(pGpu, pEngstate, arg0);
}

static inline NV_STATUS tmrStatePreUnload_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, NvU32 arg0) {
    return pEngstate->__tmrStatePreUnload__(pGpu, pEngstate, arg0);
}

static inline NV_STATUS tmrStateInitUnlocked_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate) {
    return pEngstate->__tmrStateInitUnlocked__(pGpu, pEngstate);
}

static inline void tmrInitMissing_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate) {
    pEngstate->__tmrInitMissing__(pGpu, pEngstate);
}

static inline NV_STATUS tmrStatePreInitLocked_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate) {
    return pEngstate->__tmrStatePreInitLocked__(pGpu, pEngstate);
}

static inline NV_STATUS tmrStatePreInitUnlocked_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate) {
    return pEngstate->__tmrStatePreInitUnlocked__(pGpu, pEngstate);
}

static inline NV_STATUS tmrGetTunableState_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, void *pTunableState) {
    return pEngstate->__tmrGetTunableState__(pGpu, pEngstate, pTunableState);
}

static inline NV_STATUS tmrCompareTunableState_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, void *pTunables1, void *pTunables2) {
    return pEngstate->__tmrCompareTunableState__(pGpu, pEngstate, pTunables1, pTunables2);
}

static inline void tmrFreeTunableState_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, void *pTunableState) {
    pEngstate->__tmrFreeTunableState__(pGpu, pEngstate, pTunableState);
}

static inline NV_STATUS tmrStatePostLoad_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, NvU32 arg0) {
    return pEngstate->__tmrStatePostLoad__(pGpu, pEngstate, arg0);
}

static inline NV_STATUS tmrAllocTunableState_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, void **ppTunableState) {
    return pEngstate->__tmrAllocTunableState__(pGpu, pEngstate, ppTunableState);
}

static inline NV_STATUS tmrSetTunableState_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate, void *pTunableState) {
    return pEngstate->__tmrSetTunableState__(pGpu, pEngstate, pTunableState);
}

static inline NvBool tmrIsPresent_DISPATCH(POBJGPU pGpu, struct OBJTMR *pEngstate) {
    return pEngstate->__tmrIsPresent__(pGpu, pEngstate);
}

static inline NvBool tmrServiceSwrlCallbacksPmcTree(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    return ((NvBool)(0 != 0));
}

static inline NvBool tmrClearSwrlCallbacksSemaphore(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    return ((NvBool)(0 != 0));
}

static inline void tmrServiceSwrlCallbacks(struct OBJGPU *pGpu, POBJTMR pTmr, THREAD_STATE_NODE *arg0) {
    return;
}

static inline NvBool tmrServiceSwrlWrapper(struct OBJGPU *pGpu, POBJTMR pTmr, MC_ENGINE_BITVECTOR *arg0, THREAD_STATE_NODE *arg1) {
    return ((NvBool)(0 != 0));
}

void tmrDestruct_IMPL(POBJTMR pTmr);
#define __nvoc_tmrDestruct(pTmr) tmrDestruct_IMPL(__staticCast_OBJTMR(pTmr))
NV_STATUS tmrEventCreate_IMPL(POBJTMR pTmr, PTMR_EVENT *ppEvent, TIMEPROC callbackFn, void *pUserData, NvU32 flags);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrEventCreate(POBJTMR pTmr, PTMR_EVENT *ppEvent, TIMEPROC callbackFn, void *pUserData, NvU32 flags) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventCreate(pTmr, ppEvent, callbackFn, pUserData, flags) tmrEventCreate_IMPL(__staticCast_OBJTMR(pTmr), ppEvent, callbackFn, pUserData, flags)
#endif //__nvoc_objtmr_h_disabled

void tmrEventCancel_IMPL(POBJTMR pTmr, PTMR_EVENT pEvent);
#ifdef __nvoc_objtmr_h_disabled
static inline void tmrEventCancel(POBJTMR pTmr, PTMR_EVENT pEvent) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventCancel(pTmr, pEvent) tmrEventCancel_IMPL(__staticCast_OBJTMR(pTmr), pEvent)
#endif //__nvoc_objtmr_h_disabled

void tmrEventDestroy_IMPL(POBJTMR pTmr, PTMR_EVENT pEvent);
#ifdef __nvoc_objtmr_h_disabled
static inline void tmrEventDestroy(POBJTMR pTmr, PTMR_EVENT pEvent) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventDestroy(pTmr, pEvent) tmrEventDestroy_IMPL(__staticCast_OBJTMR(pTmr), pEvent)
#endif //__nvoc_objtmr_h_disabled

void tmrInitCallbacks_IMPL(POBJTMR pTmr);
#ifdef __nvoc_objtmr_h_disabled
static inline void tmrInitCallbacks(POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
}
#else //__nvoc_objtmr_h_disabled
#define tmrInitCallbacks(pTmr) tmrInitCallbacks_IMPL(__staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

void tmrSetCountdownCallback_IMPL(POBJTMR pTmr, TIMEPROC_COUNTDOWN arg0);
#ifdef __nvoc_objtmr_h_disabled
static inline void tmrSetCountdownCallback(POBJTMR pTmr, TIMEPROC_COUNTDOWN arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
}
#else //__nvoc_objtmr_h_disabled
#define tmrSetCountdownCallback(pTmr, arg0) tmrSetCountdownCallback_IMPL(__staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

NV_STATUS tmrCancelCallback_IMPL(POBJTMR pTmr, void *pObject);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrCancelCallback(POBJTMR pTmr, void *pObject) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrCancelCallback(pTmr, pObject) tmrCancelCallback_IMPL(__staticCast_OBJTMR(pTmr), pObject)
#endif //__nvoc_objtmr_h_disabled

NV_STATUS tmrGetCurrentDiffTime_IMPL(POBJTMR pTmr, NvU64 arg0, NvU64 *arg1);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrGetCurrentDiffTime(POBJTMR pTmr, NvU64 arg0, NvU64 *arg1) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetCurrentDiffTime(pTmr, arg0, arg1) tmrGetCurrentDiffTime_IMPL(__staticCast_OBJTMR(pTmr), arg0, arg1)
#endif //__nvoc_objtmr_h_disabled

void tmrGetSystemTime_IMPL(POBJTMR pTmr, PDAYMSECTIME pTime);
#ifdef __nvoc_objtmr_h_disabled
static inline void tmrGetSystemTime(POBJTMR pTmr, PDAYMSECTIME pTime) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetSystemTime(pTmr, pTime) tmrGetSystemTime_IMPL(__staticCast_OBJTMR(pTmr), pTime)
#endif //__nvoc_objtmr_h_disabled

NvBool tmrCheckCallbacksReleaseSem_IMPL(POBJTMR pTmr, NvU32 chId);
#ifdef __nvoc_objtmr_h_disabled
static inline NvBool tmrCheckCallbacksReleaseSem(POBJTMR pTmr, NvU32 chId) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_FALSE;
}
#else //__nvoc_objtmr_h_disabled
#define tmrCheckCallbacksReleaseSem(pTmr, chId) tmrCheckCallbacksReleaseSem_IMPL(__staticCast_OBJTMR(pTmr), chId)
#endif //__nvoc_objtmr_h_disabled

NvBool tmrDiffExceedsTime_IMPL(POBJTMR pTmr, PDAYMSECTIME pFutureTime, PDAYMSECTIME pPastTime, NvU32 time);
#ifdef __nvoc_objtmr_h_disabled
static inline NvBool tmrDiffExceedsTime(POBJTMR pTmr, PDAYMSECTIME pFutureTime, PDAYMSECTIME pPastTime, NvU32 time) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_FALSE;
}
#else //__nvoc_objtmr_h_disabled
#define tmrDiffExceedsTime(pTmr, pFutureTime, pPastTime, time) tmrDiffExceedsTime_IMPL(__staticCast_OBJTMR(pTmr), pFutureTime, pPastTime, time)
#endif //__nvoc_objtmr_h_disabled

NV_STATUS tmrEventScheduleAbs_IMPL(POBJTMR pTmr, PTMR_EVENT pEvent, NvU64 timeAbs);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrEventScheduleAbs(POBJTMR pTmr, PTMR_EVENT pEvent, NvU64 timeAbs) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventScheduleAbs(pTmr, pEvent, timeAbs) tmrEventScheduleAbs_IMPL(__staticCast_OBJTMR(pTmr), pEvent, timeAbs)
#endif //__nvoc_objtmr_h_disabled

NV_STATUS tmrScheduleCallbackAbs_IMPL(POBJTMR pTmr, TIMEPROC_OBSOLETE arg0, void *arg1, NvU64 arg2, NvU32 arg3, NvU32 arg4);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrScheduleCallbackAbs(POBJTMR pTmr, TIMEPROC_OBSOLETE arg0, void *arg1, NvU64 arg2, NvU32 arg3, NvU32 arg4) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrScheduleCallbackAbs(pTmr, arg0, arg1, arg2, arg3, arg4) tmrScheduleCallbackAbs_IMPL(__staticCast_OBJTMR(pTmr), arg0, arg1, arg2, arg3, arg4)
#endif //__nvoc_objtmr_h_disabled

NV_STATUS tmrEventScheduleRel_IMPL(POBJTMR pTmr, PTMR_EVENT pEvent, NvU64 timeRel);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrEventScheduleRel(POBJTMR pTmr, PTMR_EVENT pEvent, NvU64 timeRel) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventScheduleRel(pTmr, pEvent, timeRel) tmrEventScheduleRel_IMPL(__staticCast_OBJTMR(pTmr), pEvent, timeRel)
#endif //__nvoc_objtmr_h_disabled

NV_STATUS tmrScheduleCallbackRel_IMPL(POBJTMR pTmr, TIMEPROC_OBSOLETE arg0, void *arg1, NvU64 arg2, NvU32 arg3, NvU32 arg4);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrScheduleCallbackRel(POBJTMR pTmr, TIMEPROC_OBSOLETE arg0, void *arg1, NvU64 arg2, NvU32 arg3, NvU32 arg4) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrScheduleCallbackRel(pTmr, arg0, arg1, arg2, arg3, arg4) tmrScheduleCallbackRel_IMPL(__staticCast_OBJTMR(pTmr), arg0, arg1, arg2, arg3, arg4)
#endif //__nvoc_objtmr_h_disabled

NV_STATUS tmrScheduleCallbackRelSec_IMPL(POBJTMR pTmr, TIMEPROC_OBSOLETE arg0, void *arg1, NvU32 arg2, NvU32 arg3, NvU32 arg4);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrScheduleCallbackRelSec(POBJTMR pTmr, TIMEPROC_OBSOLETE arg0, void *arg1, NvU32 arg2, NvU32 arg3, NvU32 arg4) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrScheduleCallbackRelSec(pTmr, arg0, arg1, arg2, arg3, arg4) tmrScheduleCallbackRelSec_IMPL(__staticCast_OBJTMR(pTmr), arg0, arg1, arg2, arg3, arg4)
#endif //__nvoc_objtmr_h_disabled

NvBool tmrEventOnList_IMPL(POBJTMR pTmr, PTMR_EVENT pEvent);
#ifdef __nvoc_objtmr_h_disabled
static inline NvBool tmrEventOnList(POBJTMR pTmr, PTMR_EVENT pEvent) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_FALSE;
}
#else //__nvoc_objtmr_h_disabled
#define tmrEventOnList(pTmr, pEvent) tmrEventOnList_IMPL(__staticCast_OBJTMR(pTmr), pEvent)
#endif //__nvoc_objtmr_h_disabled

NvBool tmrCallbackOnList_IMPL(POBJTMR pTmr, TIMEPROC_OBSOLETE arg0, void *arg1);
#ifdef __nvoc_objtmr_h_disabled
static inline NvBool tmrCallbackOnList(POBJTMR pTmr, TIMEPROC_OBSOLETE arg0, void *arg1) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_FALSE;
}
#else //__nvoc_objtmr_h_disabled
#define tmrCallbackOnList(pTmr, arg0, arg1) tmrCallbackOnList_IMPL(__staticCast_OBJTMR(pTmr), arg0, arg1)
#endif //__nvoc_objtmr_h_disabled

NV_STATUS tmrDelay_IMPL(POBJTMR pTmr, NvU32 arg0);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrDelay(POBJTMR pTmr, NvU32 arg0) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrDelay(pTmr, arg0) tmrDelay_IMPL(__staticCast_OBJTMR(pTmr), arg0)
#endif //__nvoc_objtmr_h_disabled

void tmrRmCallbackIntrEnable_IMPL(POBJTMR pTmr, struct OBJGPU *pGpu);
#ifdef __nvoc_objtmr_h_disabled
static inline void tmrRmCallbackIntrEnable(POBJTMR pTmr, struct OBJGPU *pGpu) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
}
#else //__nvoc_objtmr_h_disabled
#define tmrRmCallbackIntrEnable(pTmr, pGpu) tmrRmCallbackIntrEnable_IMPL(__staticCast_OBJTMR(pTmr), pGpu)
#endif //__nvoc_objtmr_h_disabled

void tmrRmCallbackIntrDisable_IMPL(POBJTMR pTmr, struct OBJGPU *pGpu);
#ifdef __nvoc_objtmr_h_disabled
static inline void tmrRmCallbackIntrDisable(POBJTMR pTmr, struct OBJGPU *pGpu) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
}
#else //__nvoc_objtmr_h_disabled
#define tmrRmCallbackIntrDisable(pTmr, pGpu) tmrRmCallbackIntrDisable_IMPL(__staticCast_OBJTMR(pTmr), pGpu)
#endif //__nvoc_objtmr_h_disabled

NV_STATUS tmrTimeUntilNextCallback_IMPL(struct OBJGPU *pGpu, POBJTMR pTmr, NvU64 *pTimeUntilCallbackNs);
#ifdef __nvoc_objtmr_h_disabled
static inline NV_STATUS tmrTimeUntilNextCallback(struct OBJGPU *pGpu, POBJTMR pTmr, NvU64 *pTimeUntilCallbackNs) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_ERR_NOT_SUPPORTED;
}
#else //__nvoc_objtmr_h_disabled
#define tmrTimeUntilNextCallback(pGpu, pTmr, pTimeUntilCallbackNs) tmrTimeUntilNextCallback_IMPL(pGpu, __staticCast_OBJTMR(pTmr), pTimeUntilCallbackNs)
#endif //__nvoc_objtmr_h_disabled

NvBool tmrCallExpiredCallbacks_IMPL(struct OBJGPU *pGpu, POBJTMR pTmr);
#ifdef __nvoc_objtmr_h_disabled
static inline NvBool tmrCallExpiredCallbacks(struct OBJGPU *pGpu, POBJTMR pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_FALSE;
}
#else //__nvoc_objtmr_h_disabled
#define tmrCallExpiredCallbacks(pGpu, pTmr) tmrCallExpiredCallbacks_IMPL(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

void tmrResetCallbackInterrupt_IMPL(struct OBJGPU *pGpu, struct OBJTMR *pTmr);
#ifdef __nvoc_objtmr_h_disabled
static inline void tmrResetCallbackInterrupt(struct OBJGPU *pGpu, struct OBJTMR *pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
}
#else //__nvoc_objtmr_h_disabled
#define tmrResetCallbackInterrupt(pGpu, pTmr) tmrResetCallbackInterrupt_IMPL(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled

NvBool tmrGetCallbackInterruptPending_IMPL(struct OBJGPU *pGpu, struct OBJTMR *pTmr);
#ifdef __nvoc_objtmr_h_disabled
static inline NvBool tmrGetCallbackInterruptPending(struct OBJGPU *pGpu, struct OBJTMR *pTmr) {
    NV_ASSERT_FAILED_PRECOMP("OBJTMR was disabled!");
    return NV_FALSE;
}
#else //__nvoc_objtmr_h_disabled
#define tmrGetCallbackInterruptPending(pGpu, pTmr) tmrGetCallbackInterruptPending_IMPL(pGpu, __staticCast_OBJTMR(pTmr))
#endif //__nvoc_objtmr_h_disabled



NV_STATUS tmrCtrlCmdEventCreate(struct OBJGPU *pGpu, TMR_EVENT_SET_PARAMS *pParams);
NV_STATUS tmrCtrlCmdEventSchedule(struct OBJGPU *pGpu, TMR_EVENT_SCHEDULE_PARAMS *pParams);
NV_STATUS tmrCtrlCmdEventCancel(struct OBJGPU *pGpu, TMR_EVENT_GENERAL_PARAMS *pParams);
NV_STATUS tmrCtrlCmdEventDestroy(struct OBJGPU *pGpu, TMR_EVENT_GENERAL_PARAMS *pParams);
#endif // _OBJTMR_H_

#ifdef __cplusplus
} // extern "C"
#endif
#endif // _G_OBJTMR_NVOC_H_
