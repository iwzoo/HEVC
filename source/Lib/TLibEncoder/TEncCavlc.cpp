/* ====================================================================================================================

  The copyright in this software is being made available under the License included below.
  This software may be subject to other third party and   contributor rights, including patent rights, and no such
  rights are granted under this license.

  Copyright (c) 2010, SAMSUNG ELECTRONICS CO., LTD. and BRITISH BROADCASTING CORPORATION
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted only for
  the purpose of developing standards within the Joint Collaborative Team on Video Coding and for testing and
  promoting such standards. The following conditions are required to be met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
      the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
      the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of SAMSUNG ELECTRONICS CO., LTD. nor the name of the BRITISH BROADCASTING CORPORATION
      may be used to endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * ====================================================================================================================
*/

/** \file     TEncCavlc.cpp
    \brief    CAVLC encoder class
*/

#include "TEncCavlc.h"

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncCavlc::TEncCavlc()
{
  m_pcBitIf           = NULL;
  m_bRunLengthCoding  = false;   //  m_bRunLengthCoding  = !rcSliceHeader.isIntra();
  m_uiCoeffCost       = 0;
  m_bAlfCtrl = false;
  m_uiMaxAlfCtrlDepth = 0;
  
  m_bAdaptFlag        = true;    // adaptive VLC table
}

TEncCavlc::~TEncCavlc()
{
}


// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncCavlc::resetEntropy()
{
  m_bRunLengthCoding = ! m_pcSlice->isIntra();
  m_uiRun = 0;
  ::memcpy(m_uiLPTableE8, g_auiLPTableE8, 10*128*sizeof(UInt));
  ::memcpy(m_uiLPTableD8, g_auiLPTableD8, 10*128*sizeof(UInt));
  ::memcpy(m_uiLPTableE4, g_auiLPTableE4, 3*32*sizeof(UInt));
  ::memcpy(m_uiLPTableD4, g_auiLPTableD4, 3*32*sizeof(UInt));
  ::memcpy(m_uiLastPosVlcIndex, g_auiLastPosVlcIndex, 10*sizeof(UInt));
  
#if LCEC_INTRA_MODE
  ::memcpy(m_uiIntraModeTableD17, g_auiIntraModeTableD17, 16*sizeof(UInt));
  ::memcpy(m_uiIntraModeTableE17, g_auiIntraModeTableE17, 16*sizeof(UInt));

  ::memcpy(m_uiIntraModeTableD34, g_auiIntraModeTableD34, 33*sizeof(UInt));
  ::memcpy(m_uiIntraModeTableE34, g_auiIntraModeTableE34, 33*sizeof(UInt));
#endif

  ::memcpy(m_uiCBPTableE, g_auiCBPTableE, 2*8*sizeof(UInt));
  ::memcpy(m_uiCBPTableD, g_auiCBPTableD, 2*8*sizeof(UInt));
  m_uiCbpVlcIdx[0] = 0;
  m_uiCbpVlcIdx[1] = 0;
  
#if QC_BLK_CBP
  ::memcpy(m_uiBlkCBPTableE, g_auiBlkCBPTableE, 2*15*sizeof(UInt));
  ::memcpy(m_uiBlkCBPTableD, g_auiBlkCBPTableD, 2*15*sizeof(UInt));
  m_uiBlkCbpVlcIdx = 0;
#endif
  
  ::memcpy(m_uiMI1TableE, g_auiMI1TableE, 8*sizeof(UInt));
  ::memcpy(m_uiMI1TableD, g_auiMI1TableD, 8*sizeof(UInt));
  ::memcpy(m_uiMI2TableE, g_auiMI2TableE, 15*sizeof(UInt));
  ::memcpy(m_uiMI2TableD, g_auiMI2TableD, 15*sizeof(UInt));
  
#if MS_NO_BACK_PRED_IN_B0
#if DCM_COMB_LIST
  if ( m_pcSlice->getNoBackPredFlag() || m_pcSlice->getNumRefIdx(REF_PIC_LIST_C)>0)
#else
  if ( m_pcSlice->getNoBackPredFlag() )
#endif
  {
    ::memcpy(m_uiMI1TableE, g_auiMI1TableENoL1, 8*sizeof(UInt));
    ::memcpy(m_uiMI1TableD, g_auiMI1TableDNoL1, 8*sizeof(UInt));
    ::memcpy(m_uiMI2TableE, g_auiMI2TableENoL1, 15*sizeof(UInt));
    ::memcpy(m_uiMI2TableD, g_auiMI2TableDNoL1, 15*sizeof(UInt));
  }
#endif
#if MS_LCEC_ONE_FRAME
  if ( m_pcSlice->getNumRefIdx(REF_PIC_LIST_0) <= 1 && m_pcSlice->getNumRefIdx(REF_PIC_LIST_1) <= 1 )
  {
    if ( m_pcSlice->getNoBackPredFlag() || ( m_pcSlice->getNumRefIdx(REF_PIC_LIST_C) > 0 && m_pcSlice->getNumRefIdx(REF_PIC_LIST_C) <= 1 ) )
    {
      ::memcpy(m_uiMI1TableE, g_auiMI1TableEOnly1RefNoL1, 8*sizeof(UInt));
      ::memcpy(m_uiMI1TableD, g_auiMI1TableDOnly1RefNoL1, 8*sizeof(UInt));
    }
    else
    {
      ::memcpy(m_uiMI1TableE, g_auiMI1TableEOnly1Ref, 8*sizeof(UInt));
      ::memcpy(m_uiMI1TableD, g_auiMI1TableDOnly1Ref, 8*sizeof(UInt));
    }
  }
#endif
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  if (m_pcSlice->getNumRefIdx(REF_PIC_LIST_C)>0)
  {
    m_uiMI1TableE[8] = 8;
    m_uiMI1TableD[8] = 8;
  }
  else  // GPB case
  {
    m_uiMI1TableD[8] = m_uiMI1TableD[6];
    m_uiMI1TableD[6] = 8;
    
    m_uiMI1TableE[m_uiMI1TableD[8]] = 8;
    m_uiMI1TableE[m_uiMI1TableD[6]] = 6;
  }
#endif
#if QC_LCEC_INTER_MODE
  ::memcpy(m_uiSplitTableE, g_auiInterModeTableE, 4*7*sizeof(UInt));
  ::memcpy(m_uiSplitTableD, g_auiInterModeTableD, 4*7*sizeof(UInt));
#endif
  
  m_uiMITableVlcIdx = 0;  
}

UInt* TEncCavlc::GetLP8Table()
{   
  return &m_uiLPTableE8[0][0];
}

UInt* TEncCavlc::GetLP4Table()
{   
  return &m_uiLPTableE4[0][0];
}

#if QC_MOD_LCEC
UInt* TEncCavlc::GetLastPosVlcIndexTable()
{   
  return &m_uiLastPosVlcIndex[0];
}
#endif


Void TEncCavlc::codePPS( TComPPS* pcPPS )
{
  // uiFirstByte
  codeNALUnitHeader( NAL_UNIT_PPS, NAL_REF_IDC_PRIORITY_HIGHEST );

#if CONSTRAINED_INTRA_PRED
  xWriteFlag( pcPPS->getConstrainedIntraPred() ? 1 : 0 );
#endif
  return;
}

Void TEncCavlc::codeNALUnitHeader( NalUnitType eNalUnitType, NalRefIdc eNalRefIdc, UInt TemporalId, Bool bOutputFlag )
{
  // uiFirstByte
  xWriteCode( 0, 1);            // forbidden_zero_flag
  xWriteCode( eNalRefIdc, 2);   // nal_ref_idc
  xWriteCode( eNalUnitType, 5); // nal_unit_type

  // to be enabled when duplicate header coding is removed
  /*
  if ( (eNalUnitType == NAL_UNIT_CODED_SLICE) || (eNalUnitType == NAL_UNIT_CODED_SLICE_IDR) || (eNalUnitType == NAL_UNIT_CODED_SLICE_CDR) )
  {
    xWriteCode( TemporalId, 3);   // temporal_id
    xWriteFlag( bOutputFlag );    // output_flag
    xWriteCode( 1, 4);            // reseved_one_4bits
  }
  */
}

Void TEncCavlc::codeSPS( TComSPS* pcSPS )
{
  // uiFirstByte
  codeNALUnitHeader( NAL_UNIT_SPS, NAL_REF_IDC_PRIORITY_HIGHEST );
  
  // Structure
  xWriteUvlc  ( pcSPS->getWidth () );
  xWriteUvlc  ( pcSPS->getHeight() );
  
  xWriteUvlc  ( pcSPS->getPad (0) );
  xWriteUvlc  ( pcSPS->getPad (1) );
  
  assert( pcSPS->getMaxCUWidth() == pcSPS->getMaxCUHeight() );
  xWriteUvlc  ( pcSPS->getMaxCUWidth()   );
  xWriteUvlc  ( pcSPS->getMaxCUDepth()-g_uiAddCUDepth );
  
  xWriteUvlc( pcSPS->getQuadtreeTULog2MinSize() - 2 );
  xWriteUvlc( pcSPS->getQuadtreeTULog2MaxSize() - pcSPS->getQuadtreeTULog2MinSize() );
  xWriteUvlc( pcSPS->getQuadtreeTUMaxDepthInter() - 1 );
  xWriteUvlc( pcSPS->getQuadtreeTUMaxDepthIntra() - 1 );
  
  
  // Tools
  xWriteFlag  ( (pcSPS->getUseALF ()) ? 1 : 0 );
  xWriteFlag  ( (pcSPS->getUseDQP ()) ? 1 : 0 );
  xWriteFlag  ( (pcSPS->getUseLDC ()) ? 1 : 0 );
  xWriteFlag  ( (pcSPS->getUseMRG ()) ? 1 : 0 ); // SOPH:
  
#if HHI_RMP_SWITCH
  xWriteFlag  ( (pcSPS->getUseRMP()) ? 1 : 0 );
#endif
    
  // AMVP mode for each depth
  for (Int i = 0; i < pcSPS->getMaxCUDepth(); i++)
  {
    xWriteFlag( pcSPS->getAMVPMode(i) ? 1 : 0);
  }
  
  // Bit-depth information
#if FULL_NBIT
  xWriteUvlc( pcSPS->getBitDepth() - 8 );
#else
#if ENABLE_IBDI
  xWriteUvlc( pcSPS->getBitDepth() - 8 );
#endif
  xWriteUvlc( pcSPS->getBitIncrement() );
#endif

#if MTK_NONCROSS_INLOOP_FILTER
  xWriteFlag( pcSPS->getLFCrossSliceBoundaryFlag()?1 : 0);
#endif

}

Void TEncCavlc::codeSliceHeader         ( TComSlice* pcSlice )
{
  // here someone can add an appropriated NalRefIdc type 
#if DCM_DECODING_REFRESH
  codeNALUnitHeader (pcSlice->getNalUnitType(), NAL_REF_IDC_PRIORITY_HIGHEST, 1, true);
#else
  codeNALUnitHeader (NAL_UNIT_CODED_SLICE, NAL_REF_IDC_PRIORITY_HIGHEST);
#endif

#if AD_HOC_SLICES && SHARP_ENTROPY_SLICE
  Bool bEntropySlice = false;
  if (pcSlice->isNextSlice())
  {
    xWriteFlag( 0 ); // Entropy slice flag
  }
  else
  {
    bEntropySlice = true;
    xWriteFlag( 1 ); // Entropy slice flag
  }
  if (!bEntropySlice)
  {
#endif
  xWriteCode  (pcSlice->getPOC(), 10 );   //  9 == SPS->Log2MaxFrameNum
  xWriteUvlc  (pcSlice->getSliceType() );
  xWriteSvlc  (pcSlice->getSliceQp() );
#if AD_HOC_SLICES 
#if SHARP_ENTROPY_SLICE
  }
  if (pcSlice->isNextSlice())
  {
    xWriteUvlc(pcSlice->getSliceCurStartCUAddr());        // start CU addr for slice
  }
  else
  {
    xWriteUvlc(pcSlice->getEntropySliceCurStartCUAddr()); // start CU addr for entropy slice
  }
  if (!bEntropySlice)
  {
#else
  xWriteUvlc(pcSlice->getSliceCurStartCUAddr()); // start CU addr for slice
#endif
#endif
  
  xWriteFlag  (pcSlice->getSymbolMode() > 0 ? 1 : 0);
  
  if (!pcSlice->isIntra())
  {
    xWriteFlag  (pcSlice->isReferenced() ? 1 : 0);
#if !HIGH_ACCURACY_BI
#ifdef ROUNDING_CONTROL_BIPRED
    xWriteFlag  (pcSlice->isRounding() ? 1 : 0);
#endif
#endif
  }
  
  xWriteFlag  (pcSlice->getLoopFilterDisable());
  
  if (!pcSlice->isIntra())
  {
    xWriteCode  ((pcSlice->getNumRefIdx( REF_PIC_LIST_0 )), 3 );
  }
  else
  {
    pcSlice->setNumRefIdx(REF_PIC_LIST_0, 0);
  }
  if (pcSlice->isInterB())
  {
    xWriteCode  ((pcSlice->getNumRefIdx( REF_PIC_LIST_1 )), 3 );
  }
  else
  {
    pcSlice->setNumRefIdx(REF_PIC_LIST_1, 0);
  }
  
#if DCM_COMB_LIST
  if (pcSlice->isInterB())
  {
    xWriteFlag  (pcSlice->getRefPicListCombinationFlag() ? 1 : 0 );
    if(pcSlice->getRefPicListCombinationFlag())
    {
      xWriteUvlc( pcSlice->getNumRefIdx(REF_PIC_LIST_C)-1);

      xWriteFlag  (pcSlice->getRefPicListModificationFlagLC() ? 1 : 0 );
      if(pcSlice->getRefPicListModificationFlagLC())
      {
        for (UInt i=0;i<pcSlice->getNumRefIdx(REF_PIC_LIST_C);i++)
        {
          xWriteFlag( pcSlice->getListIdFromIdxOfLC(i));
          xWriteUvlc( pcSlice->getRefIdxFromIdxOfLC(i));
        }
      }
    }
  }
#endif

  xWriteFlag  (pcSlice->getDRBFlag() ? 1 : 0 );
  if ( !pcSlice->getDRBFlag() )
  {
    xWriteCode  (pcSlice->getERBIndex(), 2);
  }
  
#if AMVP_NEIGH_COL
  if ( pcSlice->getSliceType() == B_SLICE )
  {
    xWriteFlag( pcSlice->getColDir() );
  }
#endif
#if AD_HOC_SLICES && SHARP_ENTROPY_SLICE
  }
#endif
}

Void TEncCavlc::codeTerminatingBit      ( UInt uilsLast )
{
}

Void TEncCavlc::codeSliceFinish ()
{
  if ( m_bRunLengthCoding && m_uiRun)
  {
    xWriteUvlc(m_uiRun);
  }
}

Void TEncCavlc::codeMVPIdx ( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  Int iSymbol = pcCU->getMVPIdx(eRefList, uiAbsPartIdx);
  Int iNum    = pcCU->getMVPNum(eRefList, uiAbsPartIdx);
  
  xWriteUnaryMaxSymbol(iSymbol, iNum-1);
}
#if QC_LCEC_INTER_MODE
Void TEncCavlc::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if ( pcCU->getSlice()->isIntra() && pcCU->isIntra( uiAbsPartIdx ) )
  {
#if MTK_DISABLE_INTRA_NxN_SPLIT
    if( uiDepth == (g_uiMaxCUDepth - g_uiAddCUDepth))
#endif
      xWriteFlag( pcCU->getPartitionSize(uiAbsPartIdx ) == SIZE_2Nx2N? 1 : 0 );
    return;
  }


#if MTK_DISABLE_INTRA_NxN_SPLIT && HHI_DISABLE_INTER_NxN_SPLIT 
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
  {    
    if ((pcCU->getPartitionSize(uiAbsPartIdx ) == SIZE_NxN) || pcCU->isIntra( uiAbsPartIdx ))
    {
  	  UInt uiIntraFlag = ( pcCU->isIntra(uiAbsPartIdx));
      if (pcCU->getPartitionSize(uiAbsPartIdx ) == SIZE_2Nx2N)
      {
        xWriteFlag(1);
      }
      else
      {
        xWriteFlag(0);
#if MTK_DISABLE_INTRA_NxN_SPLIT && !HHI_DISABLE_INTER_NxN_SPLIT 
        if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#elif !MTK_DISABLE_INTRA_NxN_SPLIT && HHI_DISABLE_INTER_NxN_SPLIT 
        if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
        xWriteFlag( uiIntraFlag? 1 : 0 );
      }

      return;
    }
  }
}
#else
Void TEncCavlc::codePartSize( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  PartSize eSize         = pcCU->getPartitionSize( uiAbsPartIdx );
  
  if ( pcCU->getSlice()->isInterB() && pcCU->isIntra( uiAbsPartIdx ) )
  {
    xWriteFlag( 0 );
#if HHI_RMP_SWITCH
    if( pcCU->getSlice()->getSPS()->getUseRMP() )
#endif
    {
      xWriteFlag( 0 );
      xWriteFlag( 0 );
    }
#if HHI_DISABLE_INTER_NxN_SPLIT
    if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    {
      xWriteFlag( 0 );
    }
#else
    xWriteFlag( 0 );
#endif
#if MTK_DISABLE_INTRA_NxN_SPLIT
    if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
    {
      xWriteFlag( (eSize == SIZE_2Nx2N? 0 : 1) );
    }
    return;
  }
  
  if ( pcCU->isIntra( uiAbsPartIdx ) )
  {
#if MTK_DISABLE_INTRA_NxN_SPLIT
    if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
    {
      xWriteFlag( eSize == SIZE_2Nx2N? 1 : 0 );
    }
    return;
  }
  
  switch(eSize)
  {
    case SIZE_2Nx2N:
    {
      xWriteFlag( 1 );
      break;
    }
    case SIZE_2NxN:
    {
      xWriteFlag( 0 );
      xWriteFlag( 1 );
      break;
    }
    case SIZE_Nx2N:
    {
      xWriteFlag( 0 );
      xWriteFlag( 0 );
      xWriteFlag( 1 );
      break;
    }
    case SIZE_NxN:
    {
#if HHI_DISABLE_INTER_NxN_SPLIT
      if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
#endif
      {
        xWriteFlag( 0 );
#if HHI_RMP_SWITCH
        if( pcCU->getSlice()->getSPS()->getUseRMP())
#endif
        {
          xWriteFlag( 0 );
          xWriteFlag( 0 );
        }
        if (pcCU->getSlice()->isInterB())
        {
          xWriteFlag( 1 );
        }
      }
      break;
    }
    default:
    {
      assert(0);
    }
  }
}
#endif

Void TEncCavlc::codePredMode( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
#if QC_LCEC_INTER_MODE
	codeInterModeFlag(pcCU, uiAbsPartIdx,(UInt)pcCU->getDepth(uiAbsPartIdx),2);
	return;
#else
  // get context function is here
  Int iPredMode = pcCU->getPredictionMode( uiAbsPartIdx );
  if ( pcCU->getSlice()->isInterB() )
  {
    return;
  }
  
  if ( iPredMode != MODE_SKIP )
  {
    xWriteFlag( iPredMode == MODE_INTER ? 0 : 1 );
  }
#endif
}

Void TEncCavlc::codeMergeFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
#if QC_LCEC_INTER_MODE
  if (pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N )
     return;
#endif
  UInt uiSymbol = pcCU->getMergeFlag( uiAbsPartIdx ) ? 1 : 0;
  xWriteFlag( uiSymbol );
}

Void TEncCavlc::codeMergeIndex    ( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Bool bLeftInvolved = false;
  Bool bAboveInvolved = false;
  Bool bCollocatedInvolved = false;
  Bool bCornerInvolved = false;
  UInt uiNumCand = 0;
  for( UInt uiIter = 0; uiIter < HHI_NUM_MRG_CAND; ++uiIter )
  {
    if( pcCU->getNeighbourCandIdx( uiIter, uiAbsPartIdx ) == uiIter + 1 )
    {
      uiNumCand++;
      if( uiIter == 0 )
      {
        bLeftInvolved = true;
      }
      else if( uiIter == 1 )
      {
        bAboveInvolved = true;
      }
      else if( uiIter == 2 )
      {
        bCollocatedInvolved = true;
      }
      else if( uiIter == 3 )
      {
        bCornerInvolved = true;
      }
    }
  }
  assert( uiNumCand > 1 );
  UInt uiUnaryIdx = pcCU->getMergeIndex( uiAbsPartIdx );
  if( !bCornerInvolved && uiUnaryIdx > 3 )
  {
    --uiUnaryIdx;
  }
  if( !bCollocatedInvolved && uiUnaryIdx > 2 )
  {
    --uiUnaryIdx;
  }
  if( !bAboveInvolved && uiUnaryIdx > 1 )
  {
    --uiUnaryIdx;
  }
  if( !bLeftInvolved && uiUnaryIdx > 0 )
  {
    --uiUnaryIdx;
  }
  for( UInt ui = 0; ui < uiNumCand - 1; ++ui )
  {
    const UInt uiSymbol = ui == uiUnaryIdx ? 0 : 1;
    xWriteFlag( uiSymbol );
    if( uiSymbol == 0 )
    {
      break;
    }
  }
}

Void TEncCavlc::codeAlfCtrlFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{  
  if (!m_bAlfCtrl)
    return;
  
  if( pcCU->getDepth(uiAbsPartIdx) > m_uiMaxAlfCtrlDepth && !pcCU->isFirstAbsZorderIdxInDepth(uiAbsPartIdx, m_uiMaxAlfCtrlDepth))
  {
    return;
  }
  
  // get context function is here
  UInt uiSymbol = pcCU->getAlfCtrlFlag( uiAbsPartIdx ) ? 1 : 0;
  
  xWriteFlag( uiSymbol );
}

Void TEncCavlc::codeAlfCtrlDepth()
{  
  if (!m_bAlfCtrl)
    return;
  
  UInt uiDepth = m_uiMaxAlfCtrlDepth;
  
    xWriteUnaryMaxSymbol(uiDepth, g_uiMaxCUDepth-1);
}
#if QC_LCEC_INTER_MODE
Void TEncCavlc::codeInterModeFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiEncMode )
{
	Bool bHasSplit = ( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )? 0 : 1;
	UInt uiSplitFlag = ( pcCU->getDepth( uiAbsPartIdx ) > uiDepth ) ? 1 : 0;
	UInt uiMode=0,uiControl=0;
	if(!uiSplitFlag || !bHasSplit)
	{
		uiMode = 1;
    uiControl = 1;
		if (!pcCU->isSkipped(uiAbsPartIdx ))
		{
      uiControl = 2;
      uiMode = 6;
      if (pcCU->getPredictionMode(uiAbsPartIdx) == MODE_INTER)
      {
        if(pcCU->getPartitionSize(uiAbsPartIdx) == SIZE_2Nx2N)
           uiMode=pcCU->getMergeFlag(uiAbsPartIdx) ? 2 : 3;
        else 
           uiMode=3+(UInt)pcCU->getPartitionSize(uiAbsPartIdx);
      }
		}
	}
  if (uiEncMode != uiControl )
		return;
  UInt uiEndSym = bHasSplit ? 7 : 6;
  UInt uiLength = m_uiSplitTableE[uiDepth][uiMode] + 1;
  if (uiLength == uiEndSym)
  {
		  xWriteCode( 0, uiLength - 1);
  }
  else
	{
      xWriteCode( 1, uiLength );
  }
 	UInt x = uiMode;
  UInt cx = m_uiSplitTableE[uiDepth][x];	
  /* Adapt table */
  if ( m_bAdaptFlag)
  {   
    if(cx>0)
    {
       UInt cy = Max(0,cx-1);
       UInt y = m_uiSplitTableD[uiDepth][cy];
		   m_uiSplitTableD[uiDepth][cy] = x;
		   m_uiSplitTableD[uiDepth][cx] = y;
		   m_uiSplitTableE[uiDepth][x] = cy;
		   m_uiSplitTableE[uiDepth][y] = cx; 
    }
  }
  return;
}
#endif
Void TEncCavlc::codeSkipFlag( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
#if QC_LCEC_INTER_MODE
	codeInterModeFlag(pcCU,uiAbsPartIdx,(UInt)pcCU->getDepth(uiAbsPartIdx),1);
	return;
#else
  // get context function is here
  UInt uiSymbol = pcCU->isSkipped( uiAbsPartIdx ) ? 1 : 0;
  xWriteFlag( uiSymbol );
#endif
}

Void TEncCavlc::codeSplitFlag   ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth )
{
  if( uiDepth == g_uiMaxCUDepth - g_uiAddCUDepth )
    return;
#if QC_LCEC_INTER_MODE
  if (!pcCU->getSlice()->isIntra())
  {
	     codeInterModeFlag(pcCU,uiAbsPartIdx,uiDepth,0);
	     return;
  }
#endif
  UInt uiCurrSplitFlag = ( pcCU->getDepth( uiAbsPartIdx ) > uiDepth ) ? 1 : 0;
  
  xWriteFlag( uiCurrSplitFlag );
  return;
}

Void TEncCavlc::codeTransformSubdivFlag( UInt uiSymbol, UInt uiCtx )
{
  xWriteFlag( uiSymbol );
}

Void TEncCavlc::codeQtCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  UInt uiCbf = pcCU->getCbf( uiAbsPartIdx, eType, uiTrDepth );
  xWriteFlag( uiCbf );
}

Void TEncCavlc::codeQtRootCbf( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiCbf = pcCU->getQtRootCbf( uiAbsPartIdx );
  xWriteFlag( uiCbf ? 1 : 0 );
}

#if LCEC_INTRA_MODE
Void TEncCavlc::codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDir         = pcCU->getLumaIntraDir( uiAbsPartIdx );
  Int iMostProbable = pcCU->getMostProbableIntraDirLuma( uiAbsPartIdx );
  Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
  UInt uiCode, uiLength;
  Int iRankIntraMode, iRankIntraModeLarger, iDirLarger;

  UInt ind=(pcCU->getLeftIntraDirLuma( uiAbsPartIdx )==pcCU->getAboveIntraDirLuma( uiAbsPartIdx ))? 0 : 1;
  
  const UInt *huff17=huff17_2[ind];
  const UInt *lengthHuff17=lengthHuff17_2[ind];
  const UInt *huff34=huff34_2[ind];
  const UInt *lengthHuff34=lengthHuff34_2[ind];

  if ( g_aucIntraModeBitsAng[iIntraIdx] < 5 )
  {
    if (iDir == iMostProbable)
      xWriteFlag( 1 );
    else{
      if (iDir>iMostProbable)
        iDir--;
      xWriteFlag( 0 );
      xWriteFlag( iDir & 0x01 ? 1 : 0 );
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) xWriteFlag( iDir & 0x02 ? 1 : 0 );
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) xWriteFlag( iDir & 0x04 ? 1 : 0 );
    }
  }
  else if ( g_aucIntraModeBitsAng[iIntraIdx] == 5 ){

    if (iDir==iMostProbable){
      uiCode=huff17[0];
      uiLength=lengthHuff17[0];
    }
    else{ 
      if (iDir>iMostProbable){
        iDir--;
      }
      iRankIntraMode=m_uiIntraModeTableE17[iDir];

      uiCode=huff17[iRankIntraMode+1];
      uiLength=lengthHuff17[iRankIntraMode+1];

      if ( m_bAdaptFlag )
      {
        iRankIntraModeLarger = Max(0,iRankIntraMode-1);
        iDirLarger = m_uiIntraModeTableD17[iRankIntraModeLarger];
        
        m_uiIntraModeTableD17[iRankIntraModeLarger] = iDir;
        m_uiIntraModeTableD17[iRankIntraMode] = iDirLarger;
        m_uiIntraModeTableE17[iDir] = iRankIntraModeLarger;
        m_uiIntraModeTableE17[iDirLarger] = iRankIntraMode;
      }
    }
    xWriteCode(uiCode, uiLength);
  }
  else{
    if (iDir==iMostProbable){
      uiCode=huff34[0];
      uiLength=lengthHuff34[0];
    }
    else{
      if (iDir>iMostProbable){
        iDir--;
      }
      iRankIntraMode=m_uiIntraModeTableE34[iDir];

      uiCode=huff34[iRankIntraMode+1];
      uiLength=lengthHuff34[iRankIntraMode+1];

      if ( m_bAdaptFlag )
      {
        iRankIntraModeLarger = Max(0,iRankIntraMode-1);
        iDirLarger = m_uiIntraModeTableD34[iRankIntraModeLarger];

        m_uiIntraModeTableD34[iRankIntraModeLarger] = iDir;
        m_uiIntraModeTableD34[iRankIntraMode] = iDirLarger;
        m_uiIntraModeTableE34[iDir] = iRankIntraModeLarger;
        m_uiIntraModeTableE34[iDirLarger] = iRankIntraMode;
      }
    }

    xWriteCode(uiCode, uiLength);
  }
}

#else

Void TEncCavlc::codeIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiDir         = pcCU->getLumaIntraDir( uiAbsPartIdx );
  Int  iMostProbable = pcCU->getMostProbableIntraDirLuma( uiAbsPartIdx );
  
  if (uiDir == iMostProbable)
  {
    xWriteFlag( 1 );
  }
  else
  {
    xWriteFlag( 0 );
    uiDir = uiDir > iMostProbable ? uiDir - 1 : uiDir;
    Int iIntraIdx = pcCU->getIntraSizeIdx(uiAbsPartIdx);
    if ( g_aucIntraModeBitsAng[iIntraIdx] < 6 )
    {
      xWriteFlag( uiDir & 0x01 ? 1 : 0 );
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 2 ) xWriteFlag( uiDir & 0x02 ? 1 : 0 );
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 3 ) xWriteFlag( uiDir & 0x04 ? 1 : 0 );
      if ( g_aucIntraModeBitsAng[iIntraIdx] > 4 ) xWriteFlag( uiDir & 0x08 ? 1 : 0 );
    }
    else
    {
      if (uiDir < 31)
      { // uiDir is here 0...32, 5 bits for uiDir 0...30, 31 is an escape code for coding one more bit for 31 and 32
        xWriteFlag( uiDir & 0x01 ? 1 : 0 );
        xWriteFlag( uiDir & 0x02 ? 1 : 0 );
        xWriteFlag( uiDir & 0x04 ? 1 : 0 );
        xWriteFlag( uiDir & 0x08 ? 1 : 0 );
        xWriteFlag( uiDir & 0x10 ? 1 : 0 );
      }
      else
      {
        xWriteFlag( 1 );
        xWriteFlag( 1 );
        xWriteFlag( 1 );
        xWriteFlag( 1 );
        xWriteFlag( 1 );
        xWriteFlag( uiDir == 32 ? 1 : 0 );
      }
    }
  }
}
#endif

Void TEncCavlc::codeIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiIntraDirChroma = pcCU->getChromaIntraDir   ( uiAbsPartIdx );
#if CHROMA_CODEWORD
  UInt uiMode = pcCU->getLumaIntraDir(uiAbsPartIdx);
  Int  iMax = uiMode < 4 ? 3 : 4; 
  
  //switch codeword
  if (uiIntraDirChroma == 4)
  {
    uiIntraDirChroma = 0;
  }
#if CHROMA_CODEWORD_SWITCH 
  else
  {
    if (uiIntraDirChroma < uiMode)
    {
      uiIntraDirChroma++;
    }
    uiIntraDirChroma = ChromaMapping[iMax-3][uiIntraDirChroma];
  }
#else
  else if (uiIntraDirChroma < uiMode)
  {
    uiIntraDirChroma++;
  }
#endif
  xWriteUnaryMaxSymbol( uiIntraDirChroma, iMax);
#else // CHROMA_CODEWORD
  if ( 0 == uiIntraDirChroma )
  {
    xWriteFlag( 0 );
  }
  else
  {
    xWriteFlag( 1 );
    xWriteUnaryMaxSymbol( uiIntraDirChroma - 1, 3 );
  }
#endif
  return;
}

Void TEncCavlc::codeInterDir( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  UInt uiInterDir = pcCU->getInterDir   ( uiAbsPartIdx );
  uiInterDir--;
  
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  if ( pcCU->getSlice()->getRefIdxCombineCoding() )
#else
  if(pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2)
#endif
  {
    Int x,cx,y,cy;
    Int iRefFrame0,iRefFrame1;
    UInt uiIndex;
    
    UInt *m_uiMITableE;
    UInt *m_uiMITableD;
    {      
      m_uiMITableE = m_uiMI1TableE;
      m_uiMITableD = m_uiMI1TableD;
      if (uiInterDir==0)
      { 
#if DCM_COMB_LIST
        if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0)
        {
          iRefFrame0 = pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_0, pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx ));
        }
        else
        {
          iRefFrame0 = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx );
        }
#else
        iRefFrame0 = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx );
#endif
        uiIndex = iRefFrame0;
        
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
        if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0)
        {
          if ( iRefFrame0 >= 4 )
          {
            uiIndex = 8;
          }
        }
        else
        {
          if ( iRefFrame0 > MS_LCEC_UNI_EXCEPTION_THRES )
          {
            uiIndex = 8;
          }
        }        
#endif        
      }
      else if (uiInterDir==1)
      {
#if DCM_COMB_LIST
        if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0)
        {
          iRefFrame1 = pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_1, pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx ));
          uiIndex = iRefFrame1;
        }
        else
        {
          iRefFrame1 = pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx );
          uiIndex = 2 + iRefFrame1;
        }
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
        if(pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0)
        {
          if ( iRefFrame1 >= 4 )
          {
            uiIndex = 8;
          }
        }
        else
        {
          if ( iRefFrame1 > MS_LCEC_UNI_EXCEPTION_THRES )
          {
            uiIndex = 8;
          }
        } 
#endif
#else
        iRefFrame1 = pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx );
        uiIndex = 2 + iRefFrame1;
#endif
      }
      else
      {
        iRefFrame0 = pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx );
        iRefFrame1 = pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx );
        uiIndex = 4 + 2*iRefFrame0 + iRefFrame1;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
        if ( iRefFrame0 >= 2 || iRefFrame1 >= 2 )
        {
          uiIndex = 8;
        }
#endif
      }
    }
    
    x = uiIndex;
    
    cx = m_uiMITableE[x];
    
    /* Adapt table */
#if !MS_LCEC_LOOKUP_TABLE_MAX_VALUE
    UInt vlcn = g_auiMITableVlcNum[m_uiMITableVlcIdx];    
#endif
    if ( m_bAdaptFlag )
    {        
      cy = Max(0,cx-1);
      y = m_uiMITableD[cy];
      m_uiMITableD[cy] = x;
      m_uiMITableD[cx] = y;
      m_uiMITableE[x] = cy;
      m_uiMITableE[y] = cx;   
      m_uiMITableVlcIdx += cx == m_uiMITableVlcIdx ? 0 : (cx < m_uiMITableVlcIdx ? -1 : 1);
    }
    
    {
#if MS_LCEC_LOOKUP_TABLE_MAX_VALUE
      UInt uiMaxVal = 7;
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
      uiMaxVal = 8;
#endif
      if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 1 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 1 )
      {
        if ( pcCU->getSlice()->getNoBackPredFlag() || ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) <= 1 ) )
        {
          uiMaxVal = 1;
        }
        else
        {
          uiMaxVal = 2;
        }
      }
      else if ( pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2 )
      {
        if ( pcCU->getSlice()->getNoBackPredFlag() || ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 && pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) <= 2 ) )
        {
          uiMaxVal = 5;
        }
        else
        {
          uiMaxVal = 7;
        }
      }
      else if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) <= 0 ) // GPB case
      {
        uiMaxVal = 4+1+MS_LCEC_UNI_EXCEPTION_THRES;
      }
      
      xWriteUnaryMaxSymbol( cx, uiMaxVal );
#else
      xWriteVlc( vlcn, cx );
#endif
    }
    
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
    if ( x<8 ) 
#endif   
    {
      return;
    }
  }
  
  xWriteFlag( ( uiInterDir == 2 ? 1 : 0 ));
#if MS_NO_BACK_PRED_IN_B0
  if ( pcCU->getSlice()->getNoBackPredFlag() )
  {
    assert( uiInterDir != 1 );
    return;
  }
#endif
#if DCM_COMB_LIST
  if ( uiInterDir < 2 && pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) <= 0)
#else
  if ( uiInterDir < 2 )
#endif
  {
    xWriteFlag( uiInterDir );
  }
  
  return;
}

Void TEncCavlc::codeRefFrmIdx( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
#if DCM_COMB_LIST
  Int iRefFrame;
  RefPicList eRefListTemp;

  if( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C)>0)
  {
    if ( pcCU->getInterDir( uiAbsPartIdx ) != 3)
    {
      eRefListTemp = REF_PIC_LIST_C;
      iRefFrame = pcCU->getSlice()->getRefIdxOfLC(eRefList, pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx ));
    }
    else
    {
      eRefListTemp = eRefList;
      iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );
    }
  }
  else
  {
    eRefListTemp = eRefList;
    iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );
  }
#else
  Int iRefFrame = pcCU->getCUMvField( eRefList )->getRefIdx( uiAbsPartIdx );
#endif  

  if (pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_0 ) <= 2 && pcCU->getSlice()->getNumRefIdx( REF_PIC_LIST_1 ) <= 2 && pcCU->getSlice()->isInterB())
  {
    return;
  }
  
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  if ( pcCU->getSlice()->getRefIdxCombineCoding() && pcCU->getInterDir(uiAbsPartIdx)==3 &&
      pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx ) < 2 &&
      pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx ) < 2 )
  {
    return;
  }
  else if ( pcCU->getSlice()->getRefIdxCombineCoding() && pcCU->getInterDir(uiAbsPartIdx)==1 && 
           ( ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C)>0  && pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_0, pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx )) < 4 ) || 
            ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C)<=0 && pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx ) <= MS_LCEC_UNI_EXCEPTION_THRES ) ) )
  {
    return;
  }
  else if ( pcCU->getSlice()->getRefIdxCombineCoding() && pcCU->getInterDir(uiAbsPartIdx)==2 && pcCU->getSlice()->getRefIdxOfLC(REF_PIC_LIST_1, pcCU->getCUMvField( REF_PIC_LIST_1 )->getRefIdx( uiAbsPartIdx )) < 4 )
  {
    return;
  }
  
  UInt uiRefFrmIdxMinus = 0;
  if ( pcCU->getSlice()->getRefIdxCombineCoding() )
  {
    if ( pcCU->getInterDir( uiAbsPartIdx ) != 3 )
    {
      if ( pcCU->getSlice()->getNumRefIdx(REF_PIC_LIST_C) > 0 )
      {
        uiRefFrmIdxMinus = 4;
        assert( iRefFrame >=4 );
      }
      else
      {
        uiRefFrmIdxMinus = MS_LCEC_UNI_EXCEPTION_THRES+1;
        assert( iRefFrame > MS_LCEC_UNI_EXCEPTION_THRES );
      }
      
    }
    else if ( eRefList == REF_PIC_LIST_1 && pcCU->getCUMvField( REF_PIC_LIST_0 )->getRefIdx( uiAbsPartIdx ) < 2 )
    {
      uiRefFrmIdxMinus = 2;
      assert( iRefFrame >= 2 );
    }
  }
  
  if ( pcCU->getSlice()->getNumRefIdx( eRefListTemp ) - uiRefFrmIdxMinus <= 1 )
  {
    return;
  }
  xWriteFlag( ( iRefFrame - uiRefFrmIdxMinus == 0 ? 0 : 1 ) );
#else
  xWriteFlag( ( iRefFrame == 0 ? 0 : 1 ) );
#endif
  
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
  if ( iRefFrame - uiRefFrmIdxMinus > 0 )
#else    
  if ( iRefFrame > 0 )
#endif
  {
    {
#if DCM_COMB_LIST
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
      xWriteUnaryMaxSymbol( iRefFrame - 1 - uiRefFrmIdxMinus, pcCU->getSlice()->getNumRefIdx( eRefListTemp )-2 - uiRefFrmIdxMinus );
#else
      xWriteUnaryMaxSymbol( iRefFrame - 1, pcCU->getSlice()->getNumRefIdx( eRefListTemp )-2 );
#endif
#else
#if MS_LCEC_LOOKUP_TABLE_EXCEPTION
      xWriteUnaryMaxSymbol( iRefFrame - 1 - uiRefFrmIdxMinus, pcCU->getSlice()->getNumRefIdx( eRefList )-2 - uiRefFrmIdxMinus );      
#else
      xWriteUnaryMaxSymbol( iRefFrame - 1, pcCU->getSlice()->getNumRefIdx( eRefList )-2 );
#endif
#endif
    }
  }
  return;
}

Void TEncCavlc::codeMvd( TComDataCU* pcCU, UInt uiAbsPartIdx, RefPicList eRefList )
{
  TComCUMvField* pcCUMvField = pcCU->getCUMvField( eRefList );
  Int iHor = pcCUMvField->getMvd( uiAbsPartIdx ).getHor();
  Int iVer = pcCUMvField->getMvd( uiAbsPartIdx ).getVer();
  
  UInt uiAbsPartIdxL, uiAbsPartIdxA;
  Int iHorPred, iVerPred;
  
  TComDataCU* pcCUL   = pcCU->getPULeft ( uiAbsPartIdxL, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  TComDataCU* pcCUA   = pcCU->getPUAbove( uiAbsPartIdxA, pcCU->getZorderIdxInCU() + uiAbsPartIdx );
  
  TComCUMvField* pcCUMvFieldL = ( pcCUL == NULL || pcCUL->isIntra( uiAbsPartIdxL ) ) ? NULL : pcCUL->getCUMvField( eRefList );
  TComCUMvField* pcCUMvFieldA = ( pcCUA == NULL || pcCUA->isIntra( uiAbsPartIdxA ) ) ? NULL : pcCUA->getCUMvField( eRefList );
  iHorPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsHor() ) +
  ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsHor() );
  iVerPred = ( (pcCUMvFieldL == NULL) ? 0 : pcCUMvFieldL->getMvd( uiAbsPartIdxL ).getAbsVer() ) +
  ( (pcCUMvFieldA == NULL) ? 0 : pcCUMvFieldA->getMvd( uiAbsPartIdxA ).getAbsVer() );
  
  xWriteSvlc( iHor );
  xWriteSvlc( iVer );
  
  return;
}

Void TEncCavlc::codeDeltaQP( TComDataCU* pcCU, UInt uiAbsPartIdx )
{
  Int iDQp  = pcCU->getQP( uiAbsPartIdx ) - pcCU->getSlice()->getSliceQp();
  
  if ( iDQp == 0 )
  {
    xWriteFlag( 0 );
  }
  else
  {
    xWriteFlag( 1 );
    xWriteSvlc( iDQp );
  }
  
  return;
}

#if LCEC_CBP_YUV_ROOT
Void TEncCavlc::codeCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth )
{
  if (eType == TEXT_ALL)
  {
    Int n,x,cx,y,cy;
    UInt uiCBFY = pcCU->getCbf(uiAbsPartIdx, TEXT_LUMA, 0);
    UInt uiCBFU = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_U, 0);
    UInt uiCBFV = pcCU->getCbf(uiAbsPartIdx, TEXT_CHROMA_V, 0);
    UInt uiCBP = (uiCBFV<<2) + (uiCBFU<<1) + (uiCBFY<<0);
    
    /* Start adaptation */
    n = pcCU->isIntra( uiAbsPartIdx ) ? 0 : 1;
    x = uiCBP;
    cx = m_uiCBPTableE[n][x];
    
    UInt vlcn = g_auiCbpVlcNum[n][m_uiCbpVlcIdx[n]];
    
    if ( m_bAdaptFlag )
    {                
      
      cy = Max(0,cx-1);
      y = m_uiCBPTableD[n][cy];
      m_uiCBPTableD[n][cy] = x;
      m_uiCBPTableD[n][cx] = y;
      m_uiCBPTableE[n][x] = cy;
      m_uiCBPTableE[n][y] = cx;
      m_uiCbpVlcIdx[n] += cx == m_uiCbpVlcIdx[n] ? 0 : (cx < m_uiCbpVlcIdx[n] ? -1 : 1);
    }
    xWriteVlc( vlcn, cx );
  }
}

Void TEncCavlc::codeBlockCbf( TComDataCU* pcCU, UInt uiAbsPartIdx, TextType eType, UInt uiTrDepth, UInt uiQPartNum, Bool bRD )
{
  UInt uiCbf0 = pcCU->getCbf   ( uiAbsPartIdx, eType, uiTrDepth );
  UInt uiCbf1 = pcCU->getCbf   ( uiAbsPartIdx + uiQPartNum, eType, uiTrDepth );
  UInt uiCbf2 = pcCU->getCbf   ( uiAbsPartIdx + uiQPartNum*2, eType, uiTrDepth );
  UInt uiCbf3 = pcCU->getCbf   ( uiAbsPartIdx + uiQPartNum*3, eType, uiTrDepth );
  UInt uiCbf = (uiCbf0<<3) | (uiCbf1<<2) | (uiCbf2<<1) | uiCbf3;
  
  assert(uiTrDepth > 0);
  
#if QC_BLK_CBP
  if(bRD && uiCbf==0)
  {
    xWriteCode(0, 4); 
    return;
  }
  
  assert(uiCbf > 0);
  
  uiCbf --;
  
  Int x,cx,y,cy;
  
  UInt n = (pcCU->isIntra(uiAbsPartIdx) && eType == TEXT_LUMA)? 0:1;
  cx = m_uiBlkCBPTableE[n][uiCbf];
  x = uiCbf;
  UInt vlcn = (n==0)?g_auiBlkCbpVlcNum[m_uiBlkCbpVlcIdx]:11;
  
  if ( m_bAdaptFlag )
  {                
    cy = Max(0,cx-1);
    y = m_uiBlkCBPTableD[n][cy];
    m_uiBlkCBPTableD[n][cy] = x;
    m_uiBlkCBPTableD[n][cx] = y;
    m_uiBlkCBPTableE[n][x] = cy;
    m_uiBlkCBPTableE[n][y] = cx;
    if(n==0)
      m_uiBlkCbpVlcIdx += cx == m_uiBlkCbpVlcIdx ? 0 : (cx < m_uiBlkCbpVlcIdx ? -1 : 1);
    
  }
  
  xWriteVlc( vlcn, cx );
  return;
#else
  xWriteCode(uiCbf, 4);
#endif
}
#endif

Void TEncCavlc::codeCoeffNxN    ( TComDataCU* pcCU, TCoeff* pcCoef, UInt uiAbsPartIdx, UInt uiWidth, UInt uiHeight, UInt uiDepth, TextType eTType, Bool bRD )
{
  if ( uiWidth > m_pcSlice->getSPS()->getMaxTrSize() )
  {
    uiWidth  = m_pcSlice->getSPS()->getMaxTrSize();
    uiHeight = m_pcSlice->getSPS()->getMaxTrSize();
  }
  UInt uiSize   = uiWidth*uiHeight;
  
  // point to coefficient
  TCoeff* piCoeff = pcCoef;
  UInt uiNumSig = 0;
  UInt uiScanning;
  
  // compute number of significant coefficients
  UInt  uiPart = 0;
  xCheckCoeff(piCoeff, uiWidth, 0, uiNumSig, uiPart );
  
  if ( bRD )
  {
    UInt uiTempDepth = uiDepth - pcCU->getDepth( uiAbsPartIdx );
    pcCU->setCbfSubParts( ( uiNumSig ? 1 : 0 ) << uiTempDepth, eTType, uiAbsPartIdx, uiDepth );
    codeCbf( pcCU, uiAbsPartIdx, eTType, uiTempDepth );
  }
  
  if ( uiNumSig == 0 )
  {
    return;
  }
  
  // initialize scan
  const UInt*  pucScan;
  //UInt uiConvBit = g_aucConvertToBit[ Min(8,uiWidth)    ];
  UInt uiConvBit = g_aucConvertToBit[ pcCU->isIntra( uiAbsPartIdx ) ? uiWidth : Min(8,uiWidth)    ];
  pucScan        = g_auiFrameScanXY [ uiConvBit + 1 ];
  
#if QC_MDCS
  UInt uiBlkPos;
  UInt uiLog2BlkSize = g_aucConvertToBit[ pcCU->isIntra( uiAbsPartIdx ) ? uiWidth : Min(8,uiWidth)    ] + 2;
  const UInt uiScanIdx = pcCU->getCoefScanIdx(uiAbsPartIdx, uiWidth, eTType==TEXT_LUMA, pcCU->isIntra(uiAbsPartIdx));
#endif //QC_MDCS
  
  TCoeff scoeff[64];
  Int iBlockType;
  UInt uiCodeDCCoef = 0;
  TCoeff dcCoeff = 0;
  if (pcCU->isIntra(uiAbsPartIdx))
  {
    UInt uiAbsPartIdxL, uiAbsPartIdxA;
    TComDataCU* pcCUL   = pcCU->getPULeft (uiAbsPartIdxL, pcCU->getZorderIdxInCU() + uiAbsPartIdx);
    TComDataCU* pcCUA   = pcCU->getPUAbove(uiAbsPartIdxA, pcCU->getZorderIdxInCU() + uiAbsPartIdx);
    if (pcCUL == NULL && pcCUA == NULL)
    {
      uiCodeDCCoef = 1;
      xWriteVlc((eTType == TEXT_LUMA ? 3 : 1) , abs(piCoeff[0]));
      if (piCoeff[0] != 0)
      {
        UInt sign = (piCoeff[0] < 0) ? 1 : 0;
        xWriteFlag(sign);
      }
      dcCoeff = piCoeff[0];
      piCoeff[0] = 1;
    }
  }
  
  if( uiSize == 2*2 )
  {
    // hack: re-use 4x4 coding
    ::memset( scoeff, 0, 16*sizeof(TCoeff) );
    for (uiScanning=0; uiScanning<4; uiScanning++)
    {
#if QC_MDCS
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
      scoeff[15-uiScanning] = piCoeff[ uiBlkPos ];
#else
      scoeff[15-uiScanning] = piCoeff[ pucScan[ uiScanning ] ];
#endif //QC_MDCS
    }
#if QC_MOD_LCEC
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
      iBlockType = eTType-2;
    else
      iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
#else
    iBlockType = pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType();
#endif
    
    xCodeCoeff4x4( scoeff, iBlockType );
  }
  else if ( uiSize == 4*4 )
  {
    for (uiScanning=0; uiScanning<16; uiScanning++)
    {
#if QC_MDCS
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
      scoeff[15-uiScanning] = piCoeff[ uiBlkPos ];
#else
      scoeff[15-uiScanning] = piCoeff[ pucScan[ uiScanning ] ];
#endif //QC_MDCS
    }
#if QC_MOD_LCEC
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
      iBlockType = eTType-2;
    else
      iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
#else
    iBlockType = pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType();
#endif
    
    xCodeCoeff4x4( scoeff, iBlockType );
  }
  else if ( uiSize == 8*8 )
  {
    for (uiScanning=0; uiScanning<64; uiScanning++)
    {
#if QC_MDCS
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
      scoeff[63-uiScanning] = piCoeff[ uiBlkPos ];
#else
      scoeff[63-uiScanning] = piCoeff[ pucScan[ uiScanning ] ];
#endif //QC_MDCS
    }
    if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V)
      iBlockType = eTType-2;
    else
      iBlockType = 2 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
    
    xCodeCoeff8x8( scoeff, iBlockType );
  }
  else
  {
    if(!pcCU->isIntra( uiAbsPartIdx ))
    {
      for (uiScanning=0; uiScanning<64; uiScanning++)
      {
#if QC_MDCS
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
      uiBlkPos = (uiBlkPos/8) * uiWidth + uiBlkPos%8;
      scoeff[63-uiScanning] = piCoeff[ uiBlkPos ];
#else
        scoeff[63-uiScanning] = piCoeff[(pucScan[uiScanning]/8)*uiWidth + (pucScan[uiScanning]%8)];      
#endif //QC_MDCS
      }
      if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V) 
        iBlockType = eTType-2;
      else
        iBlockType = 5 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
      xCodeCoeff8x8( scoeff, iBlockType );
      return;
    }    
    
    if(pcCU->isIntra( uiAbsPartIdx ))
    {
      for (uiScanning=0; uiScanning<64; uiScanning++)
      {
#if QC_MDCS
      uiBlkPos = g_auiSigLastScan[uiScanIdx][uiLog2BlkSize-1][uiScanning]; 
      scoeff[63-uiScanning] = piCoeff[ uiBlkPos ];
#else
        scoeff[63-uiScanning] = piCoeff[ pucScan[ uiScanning ] ];
#endif //QC_MDCS
      }
      
      if (eTType==TEXT_CHROMA_U || eTType==TEXT_CHROMA_V) 
        iBlockType = eTType-2;
      else
        iBlockType = 5 + ( pcCU->isIntra(uiAbsPartIdx) ? 0 : pcCU->getSlice()->getSliceType() );
      xCodeCoeff8x8( scoeff, iBlockType );
    }
    //#endif
  }
  
  if (uiCodeDCCoef == 1)
  {
    piCoeff[0] = dcCoeff;
  }
}

Void TEncCavlc::codeAlfFlag( UInt uiCode )
{
  
  xWriteFlag( uiCode );
}

#if TSB_ALF_HEADER
Void TEncCavlc::codeAlfFlagNum( UInt uiCode, UInt minValue )
{
  UInt uiLength = 0;
  UInt maxValue = (minValue << (this->getMaxAlfCtrlDepth()*2));
  assert((uiCode>=minValue)&&(uiCode<=maxValue));
  UInt temp = maxValue - minValue;
  for(UInt i=0; i<32; i++)
  {
    if(temp&0x1)
    {
      uiLength = i+1;
    }
    temp = (temp >> 1);
  }
  if(uiLength)
  {
    xWriteCode( uiCode - minValue, uiLength );
  }
}

Void TEncCavlc::codeAlfCtrlFlag( UInt uiSymbol )
{
  xWriteFlag( uiSymbol );
}
#endif

Void TEncCavlc::codeAlfUvlc( UInt uiCode )
{
  xWriteUvlc( uiCode );
}

Void TEncCavlc::codeAlfSvlc( Int iCode )
{
  xWriteSvlc( iCode );
}

Void TEncCavlc::estBit( estBitsSbacStruct* pcEstBitsCabac, UInt uiCTXIdx, TextType eTType )
{
  // printf("error : no VLC mode support in this version\n");
  return;
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TEncCavlc::xWriteCode     ( UInt uiCode, UInt uiLength )
{
  assert ( uiLength > 0 );
  m_pcBitIf->write( uiCode, uiLength );
}

Void TEncCavlc::xWriteUvlc     ( UInt uiCode )
{
  UInt uiLength = 1;
  UInt uiTemp = ++uiCode;
  
  assert ( uiTemp );
  
  while( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }
  
  m_pcBitIf->write( uiCode, uiLength );
}

Void TEncCavlc::xWriteSvlc     ( Int iCode )
{
  UInt uiCode;
  
  uiCode = xConvertToUInt( iCode );
  xWriteUvlc( uiCode );
}

Void TEncCavlc::xWriteFlag( UInt uiCode )
{
  m_pcBitIf->write( uiCode, 1 );
}

Void TEncCavlc::xCheckCoeff( TCoeff* pcCoef, UInt uiSize, UInt uiDepth, UInt& uiNumofCoeff, UInt& uiPart )
{
  UInt ui = uiSize>>uiDepth;
  if( uiPart == 0 )
  {
    if( ui <= 4 )
    {
      UInt x, y;
      TCoeff* pCeoff = pcCoef;
      for( y=0 ; y<ui ; y++ )
      {
        for( x=0 ; x<ui ; x++ )
        {
          if( pCeoff[x] != 0 )
          {
            uiNumofCoeff++;
          }
        }
        pCeoff += uiSize;
      }
    }
    else
    {
      xCheckCoeff( pcCoef,                            uiSize, uiDepth+1, uiNumofCoeff, uiPart ); uiPart++; //1st Part
      xCheckCoeff( pcCoef             + (ui>>1),      uiSize, uiDepth+1, uiNumofCoeff, uiPart ); uiPart++; //2nd Part
      xCheckCoeff( pcCoef + (ui>>1)*uiSize,           uiSize, uiDepth+1, uiNumofCoeff, uiPart ); uiPart++; //3rd Part
      xCheckCoeff( pcCoef + (ui>>1)*uiSize + (ui>>1), uiSize, uiDepth+1, uiNumofCoeff, uiPart );           //4th Part
    }
  }
  else
  {
    UInt x, y;
    TCoeff* pCeoff = pcCoef;
    for( y=0 ; y<ui ; y++ )
    {
      for( x=0 ; x<ui ; x++ )
      {
        if( pCeoff[x] != 0 )
        {
          uiNumofCoeff++;
        }
      }
      pCeoff += uiSize;
    }
  }
}

Void TEncCavlc::xWriteUnaryMaxSymbol( UInt uiSymbol, UInt uiMaxSymbol )
{
  if (uiMaxSymbol == 0)
  {
    return;
  }
  xWriteFlag( uiSymbol ? 1 : 0 );
  if ( uiSymbol == 0 )
  {
    return;
  }
  
  Bool bCodeLast = ( uiMaxSymbol > uiSymbol );
  
  while( --uiSymbol )
  {
    xWriteFlag( 1 );
  }
  if( bCodeLast )
  {
    xWriteFlag( 0 );
  }
  return;
}

Void TEncCavlc::xWriteExGolombLevel( UInt uiSymbol )
{
  if( uiSymbol )
  {
    xWriteFlag( 1 );
    UInt uiCount = 0;
    Bool bNoExGo = (uiSymbol < 13);
    
    while( --uiSymbol && ++uiCount < 13 )
    {
      xWriteFlag( 1 );
    }
    if( bNoExGo )
    {
      xWriteFlag( 0 );
    }
    else
    {
      xWriteEpExGolomb( uiSymbol, 0 );
    }
  }
  else
  {
    xWriteFlag( 0 );
  }
  return;
}

Void TEncCavlc::xWriteEpExGolomb( UInt uiSymbol, UInt uiCount )
{
  while( uiSymbol >= (UInt)(1<<uiCount) )
  {
    xWriteFlag( 1 );
    uiSymbol -= 1<<uiCount;
    uiCount  ++;
  }
  xWriteFlag( 0 );
  while( uiCount-- )
  {
    xWriteFlag( (uiSymbol>>uiCount) & 1 );
  }
  return;
}

#if !QC_MOD_LCEC_RDOQ
UInt TEncCavlc::xLeadingZeros(UInt uiCode)
{
  UInt uiCount = 0;
  Int iDone = 0;
  
  if (uiCode)
  {
    while (!iDone)
    {
      uiCode >>= 1;
      if (!uiCode) iDone = 1;
      else uiCount++;
    }
  }
  return uiCount;
}
#endif

Void TEncCavlc::xWriteVlc(UInt uiTableNumber, UInt uiCodeNumber)
{
#if QC_BLK_CBP
  assert( uiTableNumber<=11 );
#else
  assert( uiTableNumber<=10 );
#endif
  
  UInt uiTemp;
  UInt uiLength = 0;
  UInt uiCode = 0;
  
  if ( uiTableNumber < 5 )
  {
    if ((Int)uiCodeNumber < (6 * (1 << uiTableNumber)))
    {
      uiTemp = 1<<uiTableNumber;
      uiCode = uiTemp+uiCodeNumber%uiTemp;
      uiLength = 1+uiTableNumber+(uiCodeNumber>>uiTableNumber);
    }
    else
    {
      uiCode = uiCodeNumber - (6 * (1 << uiTableNumber)) + (1 << uiTableNumber);
      uiLength = (6-uiTableNumber)+1+2*xLeadingZeros(uiCode);
    }
  }
  else if (uiTableNumber < 8)
  {
    uiTemp = 1<<(uiTableNumber-4);
    uiCode = uiTemp+uiCodeNumber%uiTemp;
    uiLength = 1+(uiTableNumber-4)+(uiCodeNumber>>(uiTableNumber-4));
  }
  else if (uiTableNumber == 8)
  {
    assert( uiCodeNumber<=2 );
    if (uiCodeNumber == 0)
    {
      uiCode = 1;
      uiLength = 1;
    }
    else if (uiCodeNumber == 1)
    {
      uiCode = 1;
      uiLength = 2;
    }
    else if (uiCodeNumber == 2)
    {
      uiCode = 0;
      uiLength = 2;
    }
  }
  else if (uiTableNumber == 9)
  {
    if (uiCodeNumber == 0)
    {
      uiCode = 4;
      uiLength = 3;
    }
    else if (uiCodeNumber == 1)
    {
      uiCode = 10;
      uiLength = 4;
    }
    else if (uiCodeNumber == 2)
    {
      uiCode = 11;
      uiLength = 4;
    }
    else if (uiCodeNumber < 11)
    {
      uiCode = uiCodeNumber+21;
      uiLength = 5;
    }
    else
    {
      uiTemp = 1<<4;
      uiCode = uiTemp+(uiCodeNumber+5)%uiTemp;
      uiLength = 5+((uiCodeNumber+5)>>4);
    }
  }
  else if (uiTableNumber == 10)
  {
    uiCode = uiCodeNumber+1;
    uiLength = 1+2*xLeadingZeros(uiCode);
  }
#if QC_BLK_CBP
  else if (uiTableNumber == 11)
  {
    if (uiCodeNumber == 0)
    {
      uiCode = 0;
      uiLength = 3;
    }
    else
    {
      uiCode = uiCodeNumber + 1;
      uiLength = 4;
    }
  }
#endif
  xWriteCode(uiCode, uiLength);
}

Void TEncCavlc::xCodeCoeff4x4(TCoeff* scoeff, Int n )
{
  Int i;
  UInt cn;
  Int level,vlc,sign,done,last_pos,start;
  Int run_done,maxrun,run,lev;
#if QC_MOD_LCEC
  Int vlc_adaptive=0;
#else
  Int tmprun, vlc_adaptive=0;
#endif
  static const int atable[5] = {4,6,14,28,0xfffffff};
  Int tmp;
#if QC_MOD_LCEC
  Int nTab = max(0,n-2);
  Int tr1;
#endif
  
  /* Do the last coefficient first */
  i = 0;
  done = 0;
  
  while (!done && i < 16)
  {
    if (scoeff[i])
    {
      done = 1;
    }
    else
    {
      i++;
    }
  }
  if (i == 16)
  {
    return;
  }
  
  last_pos = 15-i;
  level = abs(scoeff[i]);
  lev = (level == 1) ? 0 : 1;
  
#if QC_MOD_LCEC
  if (level>1){
    tr1=0;
  }
  else{
    tr1=1;
  }
#endif

  {
    int x,y,cx,cy,vlcNum;
    int vlcTable[3] = {2,2,2};
    
    x = 16*lev + last_pos;
    
#if QC_MOD_LCEC
    cx = m_uiLPTableE4[nTab][x];
    vlcNum = vlcTable[nTab];
#else
    cx = m_uiLPTableE4[n][x];
    vlcNum = vlcTable[n];
#endif
    
      xWriteVlc( vlcNum, cx );
    
    if ( m_bAdaptFlag )
    {
      
      cy = Max( 0, cx-1 );
#if QC_MOD_LCEC
      y = m_uiLPTableD4[nTab][cy];
      m_uiLPTableD4[nTab][cy] = x;
      m_uiLPTableD4[nTab][cx] = y;
      m_uiLPTableE4[nTab][x] = cy;
      m_uiLPTableE4[nTab][y] = cx;
#else
      y = m_uiLPTableD4[n][cy];
      m_uiLPTableD4[n][cy] = x;
      m_uiLPTableD4[n][cx] = y;
      m_uiLPTableE4[n][x] = cy;
      m_uiLPTableE4[n][y] = cx;
#endif
    }
  }
  
  sign = (scoeff[i] < 0) ? 1 : 0;
  if (level > 1)
  {
    xWriteVlc( 0, 2*(level-2)+sign );
  }
  else
  {
    xWriteFlag( sign );
  }
  i++;
  
  if (i < 16)
  {
    /* Go into run mode */
    run_done = 0;
    while (!run_done)
    {
      maxrun = 15-i;
#if QC_MOD_LCEC
      if ( n == 2 )
        vlc = g_auiVlcTable8x8Intra[maxrun];
      else
        vlc = g_auiVlcTable8x8Inter[maxrun];
#else
      tmprun = maxrun;
      if (maxrun > 27)
      {
        vlc = 3;
        tmprun = 28;
      }
      else
      {
        vlc = g_auiVlcTable8x8[maxrun];
      }
#endif
      
      run = 0;
      done = 0;
      while (!done)
      {
        if (!scoeff[i])
        {
          run++;
        }
        else
        {
          level = abs(scoeff[i]);
          lev = (level == 1) ? 0 : 1;
#if QC_MOD_LCEC
          if ( n == 2 ){
            cn = xRunLevelInd(lev, run, maxrun, g_auiLumaRunTr14x4[tr1][maxrun]);
          }
          else{
            cn = g_auiLumaRun8x8[maxrun][lev][run];
          }
#else
          if (maxrun > 27)
          {
            cn = g_auiLumaRun8x8[28][lev][run];
          }
          else
          {
            cn = g_auiLumaRun8x8[maxrun][lev][run];
          }
#endif
            xWriteVlc( vlc, cn );
          
#if QC_MOD_LCEC
          if (tr1>0 && tr1 < MAX_TR1)
          {
            tr1++;
          }
#endif
          sign = (scoeff[i] < 0) ? 1 : 0;
          if (level > 1)
          {
            xWriteVlc( 0, 2*(level-2)+sign );
            run_done = 1;
          }
          else
          {
            xWriteFlag( sign );
          }
          
          run = 0;
          done = 1;
        }
        if (i == 15)
        {
          done = 1;
          run_done = 1;
          if (run)
          {
#if QC_MOD_LCEC
            if (n==2){
              cn=xRunLevelInd(0, run, maxrun, g_auiLumaRunTr14x4[tr1][maxrun]);
            }
            else{
              cn = g_auiLumaRun8x8[maxrun][0][run];
            }
#else
            if (maxrun > 27)
            {
              cn = g_auiLumaRun8x8[28][0][run];
            }
            else
            {
              cn = g_auiLumaRun8x8[maxrun][0][run];
            }
#endif
            xWriteVlc( vlc, cn );
          }
        }
        i++;
      }
    }
  }
  
  /* Code the rest in level mode */
  start = i;
  for ( i=start; i<16; i++ )
  {
    tmp = abs(scoeff[i]);
    xWriteVlc( vlc_adaptive, tmp );
    if (scoeff[i])
    {
      sign = (scoeff[i] < 0) ? 1 : 0;
      xWriteFlag( sign );
    }
    if ( tmp > atable[vlc_adaptive] )
    {
      vlc_adaptive++;
    }
  }
  return;
}

Void TEncCavlc::xCodeCoeff8x8( TCoeff* scoeff, Int n )
{
  int i;
  unsigned int cn;
  int level,vlc,sign,done,last_pos,start;
  int run_done,maxrun,run,lev;
#if QC_MOD_LCEC
  int vlc_adaptive=0;
#else
  int tmprun,vlc_adaptive=0;
#endif
  static const int atable[5] = {4,6,14,28,0xfffffff};
  int tmp;
#if QC_MOD_LCEC
  Int tr1;
#endif
  
  static const int switch_thr[10] = {49,49,0,49,49,0,49,49,49,49};
  int sum_big_coef = 0;
  
  
  /* Do the last coefficient first */
  i = 0;
  done = 0;
  while (!done && i < 64)
  {
    if (scoeff[i])
    {
      done = 1;
    }
    else
    {
      i++;
    }
  }
  if (i == 64)
  {
    return;
  }
  
  last_pos = 63-i;
  level = abs(scoeff[i]);
  lev = (level == 1) ? 0 : 1;
  
  {
    int x,y,cx,cy,vlcNum;
    x = 64*lev + last_pos;
    
    cx = m_uiLPTableE8[n][x];
    // ADAPT_VLC_NUM
    vlcNum = g_auiLastPosVlcNum[n][Min(16,m_uiLastPosVlcIndex[n])];
    xWriteVlc( vlcNum, cx );
    
    if ( m_bAdaptFlag )
    {
      
      // ADAPT_VLC_NUM
      m_uiLastPosVlcIndex[n] += cx == m_uiLastPosVlcIndex[n] ? 0 : (cx < m_uiLastPosVlcIndex[n] ? -1 : 1);
      cy = Max(0,cx-1);
      y = m_uiLPTableD8[n][cy];
      m_uiLPTableD8[n][cy] = x;
      m_uiLPTableD8[n][cx] = y;
      m_uiLPTableE8[n][x] = cy;
      m_uiLPTableE8[n][y] = cx;
    }
  }
  
  sign = (scoeff[i] < 0) ? 1 : 0;
  if (level > 1)
  {
    xWriteVlc( 0, 2*(level-2)+sign );
  }
  else
  {
    xWriteFlag( sign );
  }
  i++;
#if QC_MOD_LCEC
  if (level>1){
    tr1=0;
  }
  else
  {
    tr1=1;
  }
#endif
  
  if (i < 64)
  {
    /* Go into run mode */
    run_done = 0;
    while ( !run_done )
    {
      maxrun = 63-i;
#if QC_MOD_LCEC
      if(n == 2 || n == 5)
        vlc = g_auiVlcTable8x8Intra[Min(maxrun,28)];
      else
        vlc = g_auiVlcTable8x8Inter[Min(maxrun,28)];
#else
      tmprun = maxrun;
      if (maxrun > 27)
      {
        vlc = 3;
        tmprun = 28;
      }
      else
      {
        vlc = g_auiVlcTable8x8[maxrun];
      }
#endif   
      
      run = 0;
      done = 0;
      while (!done)
      {
        if (!scoeff[i])
        {
          run++;
        }
        else
        {
          level = abs(scoeff[i]);
          lev = (level == 1) ? 0 : 1;
#if QC_MOD_LCEC
          if(n == 2 || n == 5)
            cn = xRunLevelInd(lev, run, maxrun, g_auiLumaRunTr18x8[tr1][min(maxrun,28)]);
          else
            cn = g_auiLumaRun8x8[min(maxrun,28)][lev][run];
#else
          if (maxrun > 27)
          {
            cn = g_auiLumaRun8x8[28][lev][run];
          }
          else
          {
            cn = g_auiLumaRun8x8[maxrun][lev][run];
          }
#endif
          xWriteVlc( vlc, cn );
          
#if QC_MOD_LCEC
          if (tr1==0 || level >=2)
          {
            tr1=0;
          }
          else if (tr1 < MAX_TR1)
          {
            tr1++;
          }
#endif
          sign = (scoeff[i] < 0) ? 1 : 0;
          if (level > 1)
          {
            xWriteVlc( 0, 2*(level-2)+sign );
            
            sum_big_coef += level;
            if (i > switch_thr[n] || sum_big_coef > 2)
            {
              run_done = 1;
            }
          }
          else
          {
            xWriteFlag( sign );
          }
          run = 0;
          done = 1;
        }
        if (i == 63)
        {
          done = 1;
          run_done = 1;
          if (run)
          {
#if QC_MOD_LCEC
            if(n == 2 || n == 5)
              cn=xRunLevelInd(0, run, maxrun, g_auiLumaRunTr18x8[tr1][min(maxrun,28)]);
            else
              cn = g_auiLumaRun8x8[min(maxrun,28)][0][run];
#else
            if (maxrun > 27)
            {
              cn = g_auiLumaRun8x8[28][0][run];
            }
            else
            {
              cn = g_auiLumaRun8x8[maxrun][0][run];
            }
#endif
            xWriteVlc( vlc, cn );
          }
        }
        i++;
      }
    }
  }
  
  /* Code the rest in level mode */
  start = i;
  for ( i=start; i<64; i++ )
  {
    tmp = abs(scoeff[i]);
    xWriteVlc( vlc_adaptive, tmp );
    if (scoeff[i])
    {
      sign = (scoeff[i] < 0) ? 1 : 0;
      xWriteFlag( sign );
    }
    if (tmp>atable[vlc_adaptive])
    {
      vlc_adaptive++;
    }
  }
  
  return;
}
