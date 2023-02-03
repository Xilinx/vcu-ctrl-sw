/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_rtos/lib_rtos.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/Nuts.h"
#include "lib_common/SliceHeader.h"
#include "lib_common/Utils.h"

#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_common_dec/DecPicParam.h"
#include "lib_common_dec/DecDpbMode.h"

#define MAX_STACK_SIZE 16

#define MAX_BUF_HELD_BY_NEXT_COMPONENT MAX_REF /*!< e.g. display / encoder / .. */
#define PIC_ID_POOL_SIZE MAX_REF
#define MAX_DPB_SIZE (MAX_REF + MAX_STACK_SIZE + REC_BUF + CONCEAL_BUF)
#define FRM_BUF_POOL_SIZE (MAX_DPB_SIZE + MAX_BUF_HELD_BY_NEXT_COMPONENT)

#define uEndOfList 0xFF /*!< End Of List marker for Reference List */
#define UndefID 0xFF/*!< Unused Buffer ID*/

/*************************************************************************//*!
   \brief Picture status enum
*****************************************************************************/
typedef enum e_PicStatus
{
  AL_NOT_NEEDED_FOR_OUTPUT,
  AL_NOT_READY_FOR_OUTPUT,
  AL_READY_FOR_OUTPUT
}AL_EPicStatus;

/*************************************************************************//*!
   \brief Picture Manager Callback prototype
   \ingroup BufPool
*****************************************************************************/
typedef struct
{
  void (* pfnIncrementFrmBuf)(void* pUserParam, int iFrameID); /*!< Callback Callback Function to signal that a Frame Buffer is used by the DPB */
  void (* pfnDecrementFrmBuf)(void* pUserParam, int iFrameID); /*!< Callback Callback Function to signal that a frame buffer is no more used by the DPB */
  void (* pfnOutputFrmBuf)(void* pUserParam, int iFrameID); /*!< Callback Function to signal that a frame buffer need to be displayed */

  void (* pfnIncrementMvBuf)(void* pUserParam, uint8_t MvID); /*!< Callback Callback Function to signal that a Motion_vector buffer is used by the DPB */
  void (* pfnDecrementMvBuf)(void* pUserParam, uint8_t MvID); /*!< Callback Callback Function to signal that a Motion_vector buffer is no more used by the DPB */

  void* pUserParam; /*!< pointer to be passed each time one to the following callback is called */
}AL_TDpbCallback;

/*************************************************************************//*!
   \ingroup BufPool
   \brief Fifo of frame buffer to be displayed
*****************************************************************************/
typedef struct t_DispFifo
{
  uint8_t pFrmIDs[FRM_BUF_POOL_SIZE];
  uint32_t pPicLatency[FRM_BUF_POOL_SIZE];

  AL_EPicStatus pFrmStatus[FRM_BUF_POOL_SIZE]; /*!< Picture has been fully decoded */

  uint8_t uFirstFrm;
  uint8_t uNumFrm;
}AL_TDispFifo;

/*************************************************************************//*!
   \brief Single node used by Reference Buffer Pool.
*****************************************************************************/
typedef struct t_DpbNode
{
  uint8_t uNodeID;
  uint8_t uFrmID; /*!< Index of the Frame buffer associated with this node */
  uint8_t uMvID; /*!< Index of the MotionVector buffer associated with this node */
  uint8_t uPicID;

  uint8_t uPrevPOC; /*!< Index of the previous node in POC order  */
  uint8_t uNextPOC; /*!< Index of the   next   node in POC order  */

  uint8_t uPrevPocLsb; /*!< Index of the previous node in poc_lsb order  */
  uint8_t uNextPocLsb; /*!< Index of the   next   node in poc_lsb order  */

  uint8_t uPrevDecOrder; /*!< Index of the previous node in Decoding order  */
  uint8_t uNextDecOrder; /*!< Index of the   next   node in Decoding order  */

  /* info on the reference picture */
  int32_t iFramePOC; /*!< POC of this reference node */
  AL_EPicStruct ePicStruct; /*!< Picture structure of this reference node */
  uint32_t slice_pic_order_cnt_lsb;
  AL_EMarkingRef eMarking_flag; /*!< status of this reference node */

  int32_t iFrame_num;
  int32_t iSlice_frame_num;
  int32_t iFrame_num_wrap;
  int32_t iLong_term_frame_idx;
  int32_t iLong_term_pic_num;
  int32_t iPic_num;

  uint8_t pic_output_flag; /*!< whether picture must be displayed or not */
  bool bIsReset; /*!< Node has been reseted or not */
  bool bIsDisplayed; /*!< Picture in displayed list */

  uint32_t uPicLatency;
  uint8_t non_existing;
  AL_ENut eNUT;
  uint8_t uSubpicFlag; /*!< Frame with subpicture */
}AL_TDpbNode;

/*************************************************************************//*!
   \ingroup RefPool
   \brief Reference Buffers Pool object
*****************************************************************************/
typedef struct t_DPB
{
  AL_TDpbNode Nodes[MAX_DPB_SIZE]; /*!< Array of nodes */

  AL_TDispFifo DispFifo;
  AL_MUTEX Mutex;

  uint8_t PicId2NodeId[PIC_ID_POOL_SIZE];
  uint8_t PicId2FrmId[PIC_ID_POOL_SIZE];
  uint8_t PicId2MvId[PIC_ID_POOL_SIZE];
  uint8_t FreePicIDs[PIC_ID_POOL_SIZE];
  uint8_t FreePicIdCnt;

  /* dpb retro-action members */
  bool bPicWaiting;
  uint8_t uNodeWaiting;
  uint8_t uFrmWaiting;
  uint8_t uMvWaiting;

  int pDeletedFrmIDLst[FRM_BUF_POOL_SIZE];
  int pDeletedMvIDLst[MAX_DPB_SIZE];

  int iDeletedFrmLstHead;
  int iDeletedFrmLstTail;

  int iDeletedMvLstHead;
  int iDeletedMvLstTail;

  int iNumDeletedPic;

  /* dpb members */
  int iLastDisplayedPOC;
  uint8_t uNumOutputPic;
  bool bNewSeq;

  uint8_t uNumRef; /*!< Maximum number of reference managed by the DPB */

  uint8_t uHeadPOC;      /*!< Index of the first node in POC order     */
  uint8_t uHeadPocLsb;   /*!< Index of the first node in poc_lsb order */
  uint8_t uLastPOC;      /*!< Index of the last node in POC order      */
  uint8_t uHeadDecOrder; /*!< Index of the first node in arrival order */

  /*decoding members*/
  uint8_t uCurRef;
  uint8_t uCountRef;            /*!< Number of used node in the reference list */
  uint8_t uCountPic;            /*!< Number of used node in the reference list */
  int32_t MaxLongTermFrameIdx;  /*!< Number of the max long term index used in picture marking process */
  bool bLastHasMMCO5;
  AL_EDpbMode eMode; /*!< Possible DPB mode */

  AL_TDpbCallback tCallbacks; /*!< Callbacks to picture manager */
}AL_TDpb;

/*************************************************************************//*!
   \brief Initializes the specified DPB context object
   \param[in,out] pDpb               Pointer to a DPB context object
   \param[in]     uNumRef            Number of reference to manage
   \param[in]     eMode              Mode choose by user for the dpb
   \param[in]     tCallbacks         Picture manager callbacks
*****************************************************************************/
void AL_Dpb_Init(AL_TDpb* pDpb, uint8_t uNumRef, AL_EDpbMode eMode, AL_TDpbCallback tCallbacks);

/*************************************************************************//*!
   \brief Flush last DPB removal orders
   \param[in,out] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_Terminate(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief Uninitialize the specified DPB context object
   \param[in,out] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_Deinit(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief This function retrieves the number of reference present in the DPB
   \param[in] pDpb Pointer to a DPB context object
   \return returns the number of picture present in the DPB
*****************************************************************************/
uint8_t AL_Dpb_GetRefCount(AL_TDpb const* pDpb);

/*************************************************************************//*!
   \brief This function retrieves the number of picture present in the DPB
   \param[in] pDpb Pointer to a DPB context object
   \return returns the number of picture present in the DPB
*****************************************************************************/
uint8_t AL_Dpb_GetPicCount(AL_TDpb const* pDpb);

/*************************************************************************//*!
   \brief This function retrieves the next Node of a specific Node
   \param[in] pDpb Pointer to a DPB context object
   \return return the Node ID of the picture with the smallest poc value
*****************************************************************************/
uint8_t AL_Dpb_GetHeadPOC(AL_TDpb const* pDpb);

/*************************************************************************//*!
   \brief This function retrieves the number of picture needed for output
   \param[in] pDpb Pointer to a DPB context object
   \return returns the number of picture needed for output
*****************************************************************************/
uint8_t AL_Dpb_GetNumOutputPict(AL_TDpb const* pDpb);

/*************************************************************************//*!
   \brief This function return the Pic ID of the last inserted frame
   \param[in] pDpb    Pointer to a DPB context object
   \return returns the Pic ID of the last inserted frame
        0xFF if the DPB is empty
*****************************************************************************/
uint8_t AL_Dpb_GetLastPicID(AL_TDpb const* pDpb);

/*************************************************************************//*!
   \brief This function gets the number of managed references
   \param[in,out] pDpb    Pointer to a DPB context object
   \return returns the number of references managed by the DPB
*****************************************************************************/
uint8_t AL_Dpb_GetNumRef(AL_TDpb const* pDpb);

/*************************************************************************//*!
   \brief This function must be called after each DPB flushing
   \param[in] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_BeginNewSeq(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief This function must be called after each DPB flushing
   \param[in,out] pDpb    Pointer to a DPB context object
   \param[in]     uMaxRef Number of reference to be managed
*****************************************************************************/
void AL_Dpb_SetNumRef(AL_TDpb* pDpb, uint8_t uMaxRef);

/*************************************************************************//*!
   \brief Checks if the last picture has a MMCO 5 opcode
   \param[in,out] pDpb Pointer to a DPB context object
   \return true if the last picture has a MMCO 5 opcode
        false otherwise
*****************************************************************************/
bool AL_Dpb_LastHasMMCO5(AL_TDpb const* pDpb);

/*************************************************************************//*!
   \brief Takes into account the presence of the MMCO 5 opcode
   \param[in,out] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_SetMMCO5(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief Takes into account the non-presence of the MMCO 5 opcode
   \param[in,out] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_ResetMMCO5(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief Returns the frame buffer index of the next picture to be displayed
   \param[in,out] pDpb    Pointer to a DPB context object
   \return The Frame buffer index of the removed node
*****************************************************************************/
uint8_t AL_Dpb_GetDisplayBuffer(AL_TDpb const* pDpb);

/*************************************************************************//*!
   \brief This function releases a picture index previoulsy
       obtained through DPB_GetDisplayBuffer
   \param[in,out] pDpb  Pointer to a DPB context object
   \return return the frame index of the released picture
*****************************************************************************/
uint8_t AL_Dpb_ReleaseDisplayBuffer(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief Remove from output all pictures present within the DPB
   \param[in,out] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_ClearOutput(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief Flush remaining pictures from the DPB
   \param[in,out] pDpb        Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_Flush(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief Remove from the DPB all unused pictures(non-reference and not needed for output
   \param[in,out] pDpb        Pointer to a DPB context object
   \param[in]     uMaxLatency Maximum DPB latency for a picture
   \param[in]     uMaxOutput  Maximum number of picture needed for output hold by the DPB
*****************************************************************************/
void AL_Dpb_HEVC_Cleanup(AL_TDpb* pDpb, uint32_t uMaxLatency, uint8_t uMaxOutput);

/*************************************************************************//*!
   \brief Remove all non-existing pictures and oldest non-used reference from the DPB
   \param[in] pDpb Pointer to a DPB context object
*****************************************************************************/
void AL_Dpb_AVC_Cleanup(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief Remove the First Node (Decoding order) from the pool
   \param[in,out] pDpb   Pointer to a DPB context object
   \return return the frame buffer identifier of the deleted picture
*****************************************************************************/
uint8_t AL_Dpb_RemoveHead(AL_TDpb* pDpb);

/*************************************************************************//*!
   \brief Insert a new frame buffer in a reference buffer pool
   \param[in,out] pDpb            Pointer to a DPB context object
   \param[in]     iFramePOC       Picture order count of the added frame buffer
   \param[in]     ePicStruct      Picture structure of the added frame buffer
   \param[in]     uPocLsb         Value used to identify long term reference picture
   \param[in]     uNode           Node index of the added reference
   \param[in]     uFrmID          Frame Buffer index of the added reference
   \param[in]     uMvID           Associated motion-vector buffer index of the added reference
   \param[in]     pic_output_flag Specifies whether the current picture is displayed or not
   \param[in]     eMarkingFlag    Added reference status
   \param[in]     uNonExisting    Added non existing status
   \param[in]     eNUT            Added Nal Unit Type
   \param[in]     uSubpicFlag     Added subpicture flag
*****************************************************************************/
void AL_Dpb_Insert(AL_TDpb* pDpb, int iFramePOC, AL_EPicStruct ePicStruct, uint32_t uPocLsb, uint8_t uNode, uint8_t uFrmID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT, uint8_t uSubpicFlag);

/*************************************************************************//*!
   \brief Update DPB state after a frame decoding
   \param[in] pDpb   Pointer to a DPB context object
   \param[in] iFrmID FrmID of the decoded frame
*****************************************************************************/
void AL_Dpb_EndDecoding(AL_TDpb* pDpb, int iFrmID);

/*************************************************************************//*!
   \brief Calculate the pic_num of each reference picture
   \param[in] pDpb   Pointer to a DPB context object
   \param[in] pSlice Current slice header
*****************************************************************************/
void AL_Dpb_PictNumberProcess(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice);

/*************************************************************************//*!
   \brief Updates the reference status of the pictures present in the DPB
   \param[in] pDpb          Pointer to a DPB context object
   \param[in] pSlice        Current slice header
   \param[in]  iCurFramePOC POC of the current picture
*****************************************************************************/
void AL_Dpb_MarkingProcess(AL_TDpb* pDpb, AL_TAvcSliceHdr const* pSlice, int iCurFramePOC);

/*************************************************************************//*!
   \brief Initializes the reference list for a P slice
   \param[in]  pDpb           Pointer to a DPB context object
   \param[in]  eCurrPicStruct Picture structure of the current frame
   \param[out] pRefList       Pointer on the reference picture list object
*****************************************************************************/
void AL_Dpb_InitPSlice_RefList(AL_TDpb const* pDpb, AL_EPicStruct eCurrPicStruct, TBufferRef* pRefList);

/*************************************************************************//*!
   \brief Initializes the reference list for a B slice
   \param[in]  pDpb           Pointer to a DPB context object
   \param[in]  iCurFramePOC   POC of the current picture
   \param[in]  eCurrPicStruct Picture structure of the current frame
   \param[out] pRefList       Pointer on the reference picture list object
*****************************************************************************/
void AL_Dpb_InitBSlice_RefList(AL_TDpb const* pDpb, int iCurFramePOC, AL_EPicStruct eCurrPicStruct, TBufferListRef* pRefList);

/*************************************************************************//*!
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
void AL_Dpb_ModifShortTerm(AL_TDpb const* pDpb, AL_TAvcSliceHdr const* pSlice, int iPicNumIdc, uint8_t uOffset, int iL0L1, uint8_t* pRefIdx, int* pPicNumPred, TBufferListRef* pListRef);

/*************************************************************************//*!
   \brief Modifies the reference picture list on long term reference pictures
   \param[in]     pDpb        Pointer to a DPB context object
   \param[in]     pSlice      Current slice header
   \param[in]     uOffset     number of modification processed on short term reference picture
   \param[in]     iL0L1       Reference list ID
   \param[in,out] pRefIdx     reference index of the current modified picture
   \param[in,out] pListRef    Pointer on the reference picture list object
*****************************************************************************/
void AL_Dpb_ModifLongTerm(AL_TDpb const* pDpb, AL_TAvcSliceHdr const* pSlice, uint8_t uOffset, int iL0L1, uint8_t* pRefIdx, TBufferListRef* pListRef);

/*************************************************************************//*!
   \brief Retrieves the number of really existing reference pictures
   \param[in] pDpb      Pointer to a DPB context object
   \param[in] pListRef  Pointer on the reference picture list object
   \return the number of really existing reference pictures
*****************************************************************************/
int AL_Dpb_GetNumExistingRef(AL_TDpb const* pDpb, TBufferListRef const* pListRef);

/*************************************************************************//*!
   \brief Gets the first free node in the list (arrival order)
   \param[in,out] pDpb Pointer to a DPB context object
   \return the first free node index
*****************************************************************************/
uint8_t AL_Dpb_GetNextFreeNode(AL_TDpb const* pDpb);

/*************************************************************************//*!
   \brief Converts a PicID index to a NodeID index
   \param[in] pDpb   Pointer to a DPB context object
   \param[in] uPicID Index to be converted
   \return the converted NodeID
*****************************************************************************/
uint8_t AL_Dpb_ConvertPicIDToNodeID(AL_TDpb const* pDpb, uint8_t uPicID);

/*************************************************************************//*!
   \brief Fills the poc list buffer with their respective reference marking status
   \param[in]  pDpb          Pointer to a DPB context object
   \param[in]  uL0L1         Reference direction
   \param[in]  pListRef      Reference list
   \param[out] pPocList      Poc list output buffer
   \param[out] pLongTermList Reference picture marking status output buffer
   \param[out] pSubpicList   Reference picture subpics flags output buffer
*****************************************************************************/
void AL_Dpb_FillList(AL_TDpb const* pDpb, uint8_t uL0L1, TBufferListRef const* pListRef, int* pPocList, uint32_t* pLongTermList, uint32_t* pSubpicList);

/*************************************************************************//*!
   \brief Searches the picture with the given poc_lsb in the dpb with the correspondig marking flag
   \param[in] pDpb    Pointer to a DPB context object
   \param[in] poc_lsb poc_lsb value to search in the DPB
   \return The node index with the given poc_lsb
*****************************************************************************/
uint8_t AL_Dpb_SearchPocLsb(AL_TDpb const* pDpb, uint32_t poc_lsb);

/*************************************************************************//*!
   \brief Searches the picture with the given iPOC in the dpb with the correspondig marking flag
   \param[in] pDpb Pointer to a DPB context object
   \param[in] iPOC Picture order count value to search in the DPB
   \return The node index with the given iPOC
*****************************************************************************/
uint8_t AL_Dpb_SearchPOC(AL_TDpb const* pDpb, int iPOC);

/*************************************************************************//*!
   \brief This function retrieves the Node ID associated with picture which follows the current picture in poc order
   \param[in] pDpb  Pointer to a DPB context object
   \param[in] uNode Current picture identifier in the DPB Node
   \return return the Node ID of the picture which follows the current picture in poc order
*****************************************************************************/
uint8_t AL_Dpb_GetNextPOC(AL_TDpb const* pDpb, uint8_t uNode);

/*************************************************************************//*!
   \brief This function retrieves the pic_output_flag of a specific picture
   \param[in] pDpb Pointer to a DPB context object
   \param[in] uNode Picture identifier in the DPB Node
   \return return the picture's output flag
*****************************************************************************/
uint8_t AL_Dpb_GetOutputFlag(AL_TDpb const* pDpb, uint8_t uNode);

/*************************************************************************//*!
   \brief This function removes a picture from the picture list needed for output
   \param[in,out] pDpb    Pointer to a DPB context object
   \param[in]     uNode   Picture identifer in the DPB Node
*****************************************************************************/
void AL_Dpb_ResetOutputFlag(AL_TDpb* pDpb, uint8_t uNode); // UNUSED

/*************************************************************************//*!
   \brief This function retrieves the reference status of a specific picture
   \param[in] pDpb Pointer to a DPB context object
   \param[in] uNode Picture identifier in the DPB Node
   \return return the picture's reference status
*****************************************************************************/
uint8_t AL_Dpb_GetMarkingFlag(AL_TDpb const* pDpb, uint8_t uNode);

/*************************************************************************//*!
   \brief This function set the reference status of a specific picture
   \param[in,out] pDpb         Pointer to a DPB context object
   \param[in]     uNode        Picture identifer in the DPB Node
   \param[in]     eMarkingFlag Reference status to apply to the picture
*****************************************************************************/
void AL_Dpb_SetMarkingFlag(AL_TDpb* pDpb, uint8_t uNode, AL_EMarkingRef eMarkingFlag);

/*************************************************************************//*!
   \brief This function gets the latency of a specific picture in the DPB Nodes
   \param[in] pDpb    Pointer to a DPB context object
   \param[in] uNodeID Picture identifier in the DPB Node
   \return return the picture's latency
*****************************************************************************/
uint32_t AL_Dpb_GetPicLatency_FromNode(AL_TDpb const* pDpb, uint8_t uNodeID);

/*************************************************************************//*!
   \brief This function gets the Pic ID of a specific picture in the DPB Nodes
   \param[in] pDpb  Pointer to a DPB context object
   \param[in] uNode Picture identifier in the DPB Node
   \return return the picture's PicID
*****************************************************************************/
uint8_t AL_Dpb_GetPicID_FromNode(AL_TDpb const* pDpb, uint8_t uNode);

/*************************************************************************//*!
   \brief This function gets the frame ID of a specific picture in the DPB Nodes
   \param[in] pDpb  Pointer to a DPB context object
   \param[in] uNode Picture identifier in the DPB Node
   \return return the picture's FrmID
*****************************************************************************/
uint8_t AL_Dpb_GetMvID_FromNode(AL_TDpb const* pDpb, uint8_t uNode);

/*************************************************************************//*!
   \brief This function gets the frame ID of a specific picture in the DPB Nodes
   \param[in] pDpb Pointer to a DPB context object
   \param[in] uNode Picture identifier in the DPB Node
   \return return the picture's FrmID
*****************************************************************************/
uint8_t AL_Dpb_GetFrmID_FromNode(AL_TDpb const* pDpb, uint8_t uNode);

/*************************************************************************//*!
   \brief Increment the latency of a specific pcture when it follows the current picture in output order
   \param[in,out] pDpb         Pointer to a DPB context object
   \param[in]     uNode        Picture identifier in the DPB Node
   \param[in]     iCurFramePOC Picture order count of the current picture
*****************************************************************************/
void AL_Dpb_IncrementPicLatency(AL_TDpb* pDpb, uint8_t uNode, int iCurFramePOC);

/*************************************************************************//*!
   \brief Decrement the latency of a specific pcture
   \param[in,out] pDpb  Pointer to a DPB context object
   \param[in]     uNode Picture identifier in the DPB Node
*****************************************************************************/
void AL_Dpb_DecrementPicLatency(AL_TDpb* pDpb, uint8_t uNode);

/*************************************************************************//*!
   \brief Checks if the Node identified by uNode has been reseted
   \param[in] pDpb  Pointer to a DPB context object
   \param[in] uNode Node identifier
   \return true if the node has been reseted
        false otherwise
*****************************************************************************/
bool AL_Dpb_NodeIsReset(AL_TDpb const* pDpb, uint8_t uNode);

/*************************************************************************//*!
   \brief Adds the specified picture in the display list
   \param[in,out] pDpb  Pointer to a DPB context object
   \param[in]     uNode Index of the node to remove
   \return The Frame buffer index of the removed node
*****************************************************************************/
void AL_Dpb_Display(AL_TDpb* pDpb, uint8_t uNode);

/*************************************************************************//*!
   \brief Remove the specified node from the reference buffer pool
   \param[in,out] pDpb   Pointer to a DPB context object
   \param[in]     uNode  Index of the node to remove
   \return return the frame buffer identifier of the deleted picture
*****************************************************************************/
uint8_t AL_Dpb_Remove(AL_TDpb* pDpb, uint8_t uNode);

/*@}*/

