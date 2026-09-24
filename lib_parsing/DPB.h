// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/

#pragma once

#include "lib_rtos/lib_rtos.h"
#include "I_ReferenceManager.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/Nuts.h"
#include "lib_common/Index.h"
#include "lib_common/Utils.h"

#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_common_dec/DecPicParam.h"
#include "lib_common_dec/DecDpbMode.h"

#include "lib_common/AvcHeaders.h"

/*****************************************************************************
   \brief Picture status enum
*****************************************************************************/
typedef enum
{
  AL_NOT_NEEDED_FOR_OUTPUT,
  AL_NOT_READY_FOR_OUTPUT,
  AL_READY_FOR_OUTPUT,
}AL_EPicStatus;

/*****************************************************************************
   \ingroup BufPool
   \brief Fifo of frame buffer to be displayed
*****************************************************************************/
typedef struct
{
  AL_TIndex pFrmIDs[AL_REFMNGR_MAX_POOL_SIZE];
  uint32_t pPicLatency[AL_REFMNGR_MAX_POOL_SIZE];

  AL_EPicStatus pFrmStatus[AL_REFMNGR_MAX_POOL_SIZE]; /*!< Picture has been fully decoded */

  AL_TIndex tFirstFrmID;
  uint8_t uNumFrm;
}AL_TDispFifo;

/*****************************************************************************
   \brief Single node used by Reference Buffer Pool.
*****************************************************************************/
typedef struct
{
  AL_TIndex tNodeID;
  AL_TIndex tFrameID; /*!< Index of the Frame buffer associated with this node */
  AL_TIndex tMvID; /*!< Index of the MotionVector buffer associated with this node */
  AL_TIndex tPicID;

  AL_TIndex tPrevPOCNodeID; /*!< Index of the previous node in POC order */
  AL_TIndex tNextPOCNodeID; /*!< Index of the next node in POC order */

  AL_TIndex tPrevPocLsbNodeID; /*!< Index of the previous node in poc_lsb order */
  AL_TIndex tNextPocLsbNodeID; /*!< Index of the next node in poc_lsb order */

  AL_TIndex tPrevDecOrderNodeID; /*!< Index of the previous node in Decoding order */
  AL_TIndex tNextDecOrderNodeID; /*!< Index of the next node in Decoding order */

  /* info on the reference picture */
  int32_t iFramePOC; /*!< POC of this reference node */
  AL_EPicStruct ePicStruct; /*!< Picture structure of this reference node */
  int32_t slice_pic_order_cnt_lsb;
  AL_EMarkingRef eMarkingFlag; /*!< status of this reference node */

  int32_t iFrameNumber;
  int32_t iSliceFrameNumber;
  int32_t iFrameNumberWrap;
  int32_t iLongTermFrameIdx;
  int32_t iLongTermPicNum;
  int32_t iPicNum;

  bool bPicOutputFlag; /*!< whether picture must be displayed or not */
  bool bIsReset; /*!< Node has been reset or not */
  bool bIsDisplayed; /*!< Picture in displayed list */

  uint32_t uPicLatency;
  bool bNonExisting;
  AL_ENut eNUT;
  bool bSubpicFlag; /*!< Frame with subpicture */
}AL_TDpbNode;

/*****************************************************************************
   \brief Reference Picture List Context
*****************************************************************************/
/* reference picture list construction variables */
typedef struct
{
  int32_t PocStCurrBefore[AL_REF_MNGR_MAX_BUF_SIZE];
  int32_t PocStCurrAfter[AL_REF_MNGR_MAX_BUF_SIZE];
  int32_t PocStFoll[AL_REF_MNGR_MAX_BUF_SIZE];

  int32_t PocLtCurr[AL_REF_MNGR_MAX_BUF_SIZE];
  int32_t PocLtFoll[AL_REF_MNGR_MAX_BUF_SIZE];

  AL_TIndex RefPicSetStCurrBefore[AL_REF_MNGR_MAX_BUF_SIZE];
  AL_TIndex RefPicSetStCurrAfter[AL_REF_MNGR_MAX_BUF_SIZE];
  AL_TIndex RefPicSetStFoll[AL_REF_MNGR_MAX_BUF_SIZE];
  AL_TIndex RefPicSetLtCurr[AL_REF_MNGR_MAX_BUF_SIZE];
  AL_TIndex RefPicSetLtFoll[AL_REF_MNGR_MAX_BUF_SIZE];
}AL_THevcRefPicCtx;

/*****************************************************************************/
typedef struct
{
  uint8_t uNumRef;
  AL_EDpbMode eMode;
}AL_TDpbInitParam;

/*****************************************************************************/
typedef struct
{
  int32_t iFramePOC;
  AL_EPicStruct ePicStruct;
  int32_t iPocLsb;
  bool bPicOutputFlag;
  AL_EMarkingRef eMarkingFlag;
  bool bNonExisting;
  AL_ENut eNUT;
  bool bSubpicFlag;
}AL_TDpbInsertParam;

/*****************************************************************************
   \ingroup RefPool
   \brief Reference Buffers Pool object
*****************************************************************************/
typedef struct
{
  AL_IReferenceManagerVtable const* vtable;

  AL_TDpbNode Nodes[AL_REF_MNGR_MAX_BUF_SIZE]; /*!< Array of nodes */

  AL_TDispFifo DispFifo;
  AL_MUTEX Mutex;

  AL_TIndex PicId2NodeId[AL_MAX_REF];
  AL_TIndex PicId2FrmId[AL_MAX_REF];
  AL_TIndex PicId2MvId[AL_MAX_REF];
  AL_TIndex FreePicIDs[AL_MAX_REF];
  uint8_t FreePicIdCnt;

  /* dpb retro-action members */
  bool bPicWaiting;
  AL_TIndex tNodeWaitingID;
  AL_TIndex tFrmWaitingID;
  AL_TIndex tMvWaitingID;

  int32_t pDeletedFrmIDLst[AL_REFMNGR_MAX_POOL_SIZE];
  int32_t pDeletedMvIDLst[AL_REF_MNGR_MAX_BUF_SIZE];

  int32_t iDeletedFrmLstHead;
  int32_t iDeletedFrmLstTail;

  int32_t iDeletedMvLstHead;
  int32_t iDeletedMvLstTail;

  int32_t iNumDeletedPic;

  /* dpb members */
  int32_t iLastDisplayedPOC;
  uint8_t uNumOutputPic;

  uint8_t uNumRef; /*!< Maximum number of reference managed by the DPB */

  AL_TIndex tHeadPOCNodeID;      /*!< Index of the first node in POC order */
  AL_TIndex tHeadPocLsbNodeID;   /*!< Index of the first node in poc_lsb order */
  AL_TIndex tLastPOCNodeID;      /*!< Index of the last node in POC order */
  AL_TIndex tHeadDecOrderNodeID; /*!< Index of the first node in arrival order */

  /*decoding members*/
  AL_TIndex tCurRefNodeID;
  uint8_t uCountRef; /*!< Number of used node in the reference list */
  uint8_t uCountPic; /*!< Number of used node in the reference list */
  int32_t iMaxLongTermFrameIdx; /*!< Number of the max long term index used in picture marking process */
  bool bLastHasMMCO5;
  AL_EDpbMode eMode; /*!< Possible DPB mode */

  /*info needed for POC calculation*/
  int32_t iCurFramePOC;
  int32_t iPrevPocLSB;
  int32_t iPrevPocMSB;
  int32_t iPrevPOC;
  AL_64S iPrevFrameNumOffset;
  AL_64S iPrevFrameNum;
  int32_t iTopFieldOrderCnt;
  int32_t iBotFieldOrderCnt;
  AL_EPicStruct ePicStruct;

  union
  {
    AL_THevcRefPicCtx HevcRef;
  };

  AL_TPictMngrCallbacks tCallbacks;
}AL_TDpb;

/*****************************************************************************/
AL_IReferenceManager* AL_Dpb_Create(AL_TDpbInitParam* pParams);

/*****************************************************************************
   \brief Flush last DPB removal orders
*****************************************************************************/
void AL_Dpb_Terminate(AL_IReferenceManager* pICtx);

/*****************************************************************************
   \brief This function retrieves the number of reference present in the DPB
   \param[in] pDpb Pointer to a DPB context object
   \return returns the number of picture present in the DPB
*****************************************************************************/
uint8_t AL_Dpb_GetRefCount(AL_TDpb const* pDpb);

/*****************************************************************************
   \brief This function retrieves the number of picture present in the DPB
   \param[in] pDpb Pointer to a DPB context object
   \return returns the number of picture present in the DPB
*****************************************************************************/
uint8_t AL_Dpb_GetPicCount(AL_TDpb const* pDpb);

/*****************************************************************************
   \brief This function retrieves the next Node of a specific Node
   \param[in] pDpb Pointer to a DPB context object
   \return return the Node ID of the picture with the smallest poc value
*****************************************************************************/
AL_TIndex AL_Dpb_GetHeadPOC(AL_TDpb const* pDpb);

/*****************************************************************************
   \brief This function return the Pic ID of the last inserted frame
   \param[in] pDpb    Pointer to a DPB context object
   \return returns the Pic ID of the last inserted frame
        0xFF if the DPB is empty
*****************************************************************************/
AL_TIndex AL_Dpb_GetLastPicID(AL_TDpb const* pDpb);

/*****************************************************************************
   \brief This function gets the number of managed references
   \param[in,out] pDpb    Pointer to a DPB context object
   \return returns the number of references managed by the DPB
*****************************************************************************/
uint8_t AL_Dpb_GetNumRef(AL_TDpb const* pDpb);

/*****************************************************************************
   \brief This function must be called after each DPB flushing
   \param[in]     uMaxRef Number of reference to be managed
*****************************************************************************/
void AL_Dpb_SetNumRef(AL_TDpb* pDpb, uint8_t uMaxRef);

/*****************************************************************************
   \brief Checks if the last picture has a MMCO 5 opcode
   \param[in,out] pDpb Pointer to a DPB context object
   \return true if the last picture has a MMCO 5 opcode
        false otherwise
*****************************************************************************/
bool AL_Dpb_LastHasMMCO5(AL_TDpb const* pDpb);

/*****************************************************************************
   \brief Takes into account the non-presence of the MMCO 5 opcode
   \param[in,out] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_ResetMMCO5(AL_TDpb* pDpb);

/*****************************************************************************
   \brief Remove from output all pictures present within the DPB
   \param[in,out] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_ClearOutput(AL_TDpb* pDpb);

/*****************************************************************************
   \brief Remove from the DPB all unused pictures(non-reference and not needed for output
   \param[in,out] pDpb        Pointer to a DPB context object
   \param[in]     uMaxLatency Maximum DPB latency for a picture
   \param[in]     uMaxOutput  Maximum number of picture needed for output hold by the DPB
*****************************************************************************/
void AL_Dpb_HEVC_Cleanup(AL_TDpb* pDpb, uint32_t uMaxLatency, uint8_t uMaxOutput);

/*****************************************************************************
   \brief Remove all non-existing pictures and oldest non-used reference from the DPB
   \param[in] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_AVC_Cleanup(AL_TDpb* pDpb);

/*****************************************************************************
   \brief Remove the First Node (Decoding order) from the pool
   \param[in,out] pDpb   Pointer to a DPB context object
   \return return the frame buffer identifier of the deleted picture
*****************************************************************************/
AL_TIndex AL_Dpb_RemoveHead(AL_TDpb* pDpb);

/*****************************************************************************
   \brief Calculate the pic_num of each reference picture
   \param[in] pDpb   Pointer to a DPB context object
   \param[in] pSlice Current slice header
*****************************************************************************/
void AL_Dpb_PictNumberProcess(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice);

/*****************************************************************************
   \brief Updates the reference status of the pictures present in the DPB
   \param[in] pDpb          Pointer to a DPB context object
   \param[in] pSlice        Current slice header
   \param[in]  iCurFramePOC POC of the current picture
*****************************************************************************/
void AL_Dpb_MarkingProcess(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, int32_t iCurFramePOC);

/*****************************************************************************
   \brief Initializes the reference list for a P slice
   \param[in]  pDpb           Pointer to a DPB context object
   \param[in]  eCurrPicStruct Picture structure of the current frame
   \param[out] pRefList       Pointer on the reference picture list object
*****************************************************************************/
void AL_Dpb_InitPSlice_RefList(AL_TDpb const* pDpb, AL_EPicStruct eCurrPicStruct, TBufferRef* pRefList);

/*****************************************************************************
   \brief Initializes the reference list for a B slice
   \param[in]  pDpb           Pointer to a DPB context object
   \param[in]  iCurFramePOC   POC of the current picture
   \param[in]  eCurrPicStruct Picture structure of the current frame
   \param[out] pRefList       Pointer on the reference picture list object
*****************************************************************************/
void AL_Dpb_InitBSlice_RefList(AL_TDpb const* pDpb, int32_t iCurFramePOC, AL_EPicStruct eCurrPicStruct, TBufferListRef* pRefList);

/*****************************************************************************
   \brief Modifies the reference picture list on short term reference pictures
   \param[in]     pDpb        Pointer to a DPB context object
   \param[in]     pSlice      Current slice header
   \param[in]     iPicNumIdc  picture reordering opcode
   \param[in]     uOffset     number of modification processed on short term reference picture
   \param[in]     iL0L1       Reference list ID
   \param[in,out] pRefIdx     reference index of the current modified picture
   \param[in]     pPicNumPred Pic Num of the processed picture without wrapping
   \param[in,out] pListRef    Pointer on the reference picture list object
*****************************************************************************/
void AL_Dpb_ModifShortTerm(AL_TDpb const* pDpb, AL_TAvcSliceHdr const* pSlice, int32_t iPicNumIdc, uint8_t uOffset, int32_t iL0L1, AL_TIndex* pRefIdx, int32_t* pPicNumPred, TBufferListRef* pListRef);

/*****************************************************************************
   \brief Modifies the reference picture list on long term reference pictures
   \param[in]     pDpb        Pointer to a DPB context object
   \param[in]     pSlice      Current slice header
   \param[in]     uOffset     number of modification processed on short term reference picture
   \param[in]     iL0L1       Reference list ID
   \param[in,out] pRefIdx     reference index of the current modified picture
   \param[in,out] pListRef    Pointer on the reference picture list object
*****************************************************************************/
void AL_Dpb_ModifLongTerm(AL_TDpb const* pDpb, AL_TAvcSliceHdr const* pSlice, uint8_t uOffset, int32_t iL0L1, AL_TIndex* pRefIdx, TBufferListRef* pListRef);

/*****************************************************************************
   \brief Check if at least one frame exists
   \param[in] pDpb      Pointer to a DPB context object
   \param[in] pListRef  Pointer on the reference picture list object
   \return True if one frame exists
*****************************************************************************/
bool AL_Dpb_HasExistingRef(AL_TDpb const* pDpb, TBufferListRef const* pListRef);

/*****************************************************************************
   \brief Converts a PicID index to a NodeID index
   \param[in] pDpb   Pointer to a DPB context object
   \param[in] tPicID Index to be converted
   \return the converted NodeID
*****************************************************************************/
AL_TIndex AL_Dpb_ConvertPicIDToNodeID(AL_TDpb const* pDpb, AL_TIndex tPicID);

/*****************************************************************************
   \brief Fills the poc list buffer with their respective reference marking status
   \param[in] pDpb                   Pointer to a DPB context object
   \param[in] pPOC                   Set of arrays that will be filled with the infos
   \param[in] uFirstLcuSliceSegment  Index of the first LCU in a slice
*****************************************************************************/
void AL_Dpb_FillPocAndLongtermLists(AL_TDpb const* pDpb, TBufferPOC* pPoc, uint32_t uFirstLcuSliceSegment);

/*****************************************************************************
   \brief Searches the picture with the given poc_lsb in the dpb with the corresponding marking flag
   \param[in] pDpb    Pointer to a DPB context object
   \param[in] poc_lsb poc_lsb value to search in the DPB
   \return The node index with the given poc_lsb
*****************************************************************************/
AL_TIndex AL_Dpb_SearchPocLsb(AL_TDpb const* pDpb, int32_t poc_lsb);

/*****************************************************************************
   \brief Searches the picture with the given iPOC in the dpb with the corresponding marking flag
   \param[in] pDpb Pointer to a DPB context object
   \param[in] iPOC Picture order count value to search in the DPB
   \return The node index with the given iPOC
*****************************************************************************/
AL_TIndex AL_Dpb_SearchPOC(AL_TDpb const* pDpb, int32_t iPOC);

/*****************************************************************************
   \brief This function retrieves the Node ID associated with picture which follows the current picture in poc order
   \param[in] pDpb  Pointer to a DPB context object
   \param[in] tNodeID Current picture identifier in the DPB Node
   \return return the Node ID of the picture which follows the current picture in poc order
*****************************************************************************/
AL_TIndex AL_Dpb_GetNextPOC(AL_TDpb const* pDpb, AL_TIndex tNodeID);

/*****************************************************************************
   \brief This function retrieves the bPicOutputFlag of a specific picture
   \param[in] pDpb Pointer to a DPB context object
   \param[in] tNodeID Picture identifier in the DPB Node
   \return return true if the picture's output flag is set, false otherwise
*****************************************************************************/
bool AL_Dpb_GetOutputFlag(AL_TDpb const* pDpb, AL_TIndex tNodeID);

/*****************************************************************************
   \brief This function retrieves the reference status of a specific picture
   \param[in] pDpb Pointer to a DPB context object
   \param[in] tNodeID Picture identifier in the DPB Node
   \return return the picture's reference status
*****************************************************************************/
uint8_t AL_Dpb_GetMarkingFlag(AL_TDpb const* pDpb, AL_TIndex tNodeID);

/*****************************************************************************
   \brief This function set the reference status of a specific picture
   \param[in,out] pDpb Pointer to a DPB context object
   \param[in] tNodeID Picture identifier in the DPB Node
   \param[in] eMarkingFlag Reference status to apply to the picture
*****************************************************************************/
void AL_Dpb_SetMarkingFlag(AL_TDpb* pDpb, AL_TIndex tNodeID, AL_EMarkingRef eMarkingFlag);

/*****************************************************************************
   \brief This function gets the Pic ID of a specific picture in the DPB Nodes
   \param[in] pDpb Pointer to a DPB context object
   \param[in] tNodeID Picture identifier in the DPB Node
   \return return the picture's PicID
*****************************************************************************/
AL_TIndex AL_Dpb_GetPicID_FromNode(AL_TDpb const* pDpb, AL_TIndex tNodeID);

/*****************************************************************************
   \brief This function gets the frame ID of a specific picture in the DPB Nodes
   \param[in] pDpb Pointer to a DPB context object
   \param[in] tNodeID Picture identifier in the DPB Node
   \return return the picture's FrmID
*****************************************************************************/
AL_TIndex AL_Dpb_GetMvID_FromNode(AL_TDpb const* pDpb, AL_TIndex tNodeID);

/*****************************************************************************
   \brief This function gets the frame ID of a specific picture in the DPB Nodes
   \param[in] pDpb Pointer to a DPB context object
   \param[in] tNodeID Picture identifier in the DPB Node
   \return return the picture's FrmID
*****************************************************************************/
AL_TIndex AL_Dpb_GetFrmID_FromNode(AL_TDpb const* pDpb, AL_TIndex tNodeID);

/*****************************************************************************
   \brief Increment the latency of a specific pcture when it follows the current picture in output order
   \param[in,out] pDpb Pointer to a DPB context object
   \param[in] tNodeID Picture identifier in the DPB Node
*****************************************************************************/
void AL_Dpb_IncrementPicLatency(AL_TDpb* pDpb, AL_TIndex tNodeID);

/*****************************************************************************
   \brief Adds the specified picture in the display list
   \param[in,out] pDpb Pointer to a DPB context object
   \param[in] tNodeID Index of the node to remove
   \return The Frame buffer index of the removed node
*****************************************************************************/
void AL_Dpb_Display(AL_TDpb* pDpb, AL_TIndex tNodeID);

/*****************************************************************************
   \brief Remove the specified node from the reference buffer pool
   \param[in,out] pDpb Pointer to a DPB context object
   \param[in] tNodeID Index of the node to remove
   \return return the frame buffer identifier of the deleted picture
*****************************************************************************/
AL_TIndex AL_Dpb_Remove(AL_TDpb* pDpb, AL_TIndex tNodeID);

/*!@}*/
