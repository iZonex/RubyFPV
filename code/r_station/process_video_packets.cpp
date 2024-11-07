/*
    Ruby Licence
    Copyright (c) 2024 Petru Soroaga petrusoroaga@yahoo.com
    All rights reserved.

    Redistribution and use in source and/or binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Copyright info and developer info must be preserved as is in the user
        interface, additions could be made to that info.
        * Neither the name of the organization nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.
        * Military use is not permited.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE AUTHOR (PETRU SOROAGA) BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "process_video_packets.h"
#include "relay_rx.h"
#include "../base/models.h"
#include "../base/models_list.h"
#include "../base/controller_rt_info.h"
#include "../common/relay_utils.h"
#include "adaptive_video.h"
#include "shared_vars.h"
#include "timers.h"


ProcessorRxVideo* _find_create_rx_video_processor(u32 uVehicleId, u32 uVideoStreamIndex)
{
   for( int i=0; i<MAX_VIDEO_PROCESSORS; i++ )
   {
      if ( NULL != g_pVideoProcessorRxList[i] )
      if ( g_pVideoProcessorRxList[i]->m_uVehicleId == uVehicleId )
      if ( g_pVideoProcessorRxList[i]->m_uVideoStreamIndex == uVideoStreamIndex )
         return g_pVideoProcessorRxList[i];
   }


   int iFirstFreeSlot = -1;
   for( int i=0; i<MAX_VIDEO_PROCESSORS; i++ )
   {
      if ( NULL == g_pVideoProcessorRxList[i] )
      {
         iFirstFreeSlot = i;
         break;
      }
   }

   if ( -1 == iFirstFreeSlot )
   {
      log_softerror_and_alarm("No more free slots to create new video rx processor, for VID: %u", uVehicleId);
      return NULL;
   }

   log_line("Creating new video Rx processor for VID %u, video stream id %d", uVehicleId, uVideoStreamIndex);
   g_pVideoProcessorRxList[iFirstFreeSlot] = new ProcessorRxVideo(uVehicleId, uVideoStreamIndex);
   g_pVideoProcessorRxList[iFirstFreeSlot]->init();
   return g_pVideoProcessorRxList[iFirstFreeSlot];
}

// Returns 1 if end of a video block was reached
// Returns -1 if the packet is not for this vehicle or was not processed
int _process_received_video_data_packet(int iInterfaceIndex, u8* pPacket, int iPacketLength)
{
   t_packet_header* pPH = (t_packet_header*)pPacket;
   t_packet_header_video_full_98* pPHVF = (t_packet_header_video_full_98*) (pPacket+sizeof(t_packet_header));
   u32 uVehicleId = pPH->vehicle_id_src;
   Model* pModel = findModelWithId(uVehicleId, 111);
   if ( NULL == pModel )
      return -1;

   //log_line("DEBUG recv [%u/%u] %u, end %d", pPHVF->uCurrentBlockIndex, pPHVF->uCurrentBlockPacketIndex, pPH->radio_link_packet_index, pPHVF->uVideoStatusFlags2 & VIDEO_STATUS_FLAGS2_END_FRAME);
   //if ( pPHVF->uVideoStatusFlags2 & VIDEO_STATUS_FLAGS2_END_FRAME )
   //   log_line("DEBUG end frame");

   /*
   static u32 s_uTmpDebugLastBlock = 0;
   static u32 s_uTmpDebugLastPacket =0;
   int iMissing = 0;
   if ( pPHVF->uCurrentBlockIndex == s_uTmpDebugLastBlock )
   if ( pPHVF->uCurrentBlockPacketIndex > s_uTmpDebugLastPacket+1 )
      iMissing = pPHVF->uCurrentBlockPacketIndex - s_uTmpDebugLastPacket - 1;
   if ( pPHVF->uCurrentBlockIndex > s_uTmpDebugLastBlock+1 )
      iMissing = 1;
   if ( pPHVF->uCurrentBlockIndex == s_uTmpDebugLastBlock+1 )
   if ( (pPHVF->uCurrentBlockPacketIndex != 0) || (s_uTmpDebugLastPacket != pPHVF->uCurrentBlockDataPackets + pPHVF->uCurrentBlockECPackets -1 ) )
      iMissing = 1;

   if ( iMissing )
      log_line("DEBUG recv video [%u/%u] %s%d %d/%d bytes", pPHVF->uCurrentBlockIndex, pPHVF->uCurrentBlockPacketIndex,
         iMissing?"***":"   ", iMissing, pPHVF->uCurrentBlockPacketSize, pPH->total_length - sizeof(t_packet_header) - sizeof(t_packet_header_video_full_98));
   
   s_uTmpDebugLastBlock = pPHVF->uCurrentBlockIndex;
   s_uTmpDebugLastPacket = pPHVF->uCurrentBlockPacketIndex;
   */

     
   bool bIsRelayedPacket = relay_controller_is_vehicle_id_relayed_vehicle(g_pCurrentModel, uVehicleId);
   u32 uVideoStreamIndex = 0;
   ProcessorRxVideo* pProcessorVideo = _find_create_rx_video_processor(uVehicleId, uVideoStreamIndex);

   if ( NULL == pProcessorVideo )
      return -1;

   pProcessorVideo->handleReceivedVideoPacket(iInterfaceIndex, pPacket, iPacketLength);

   // Did we received a new video stream? Add info for it.
// To fix
     /*
   t_packet_header_video_full_77* pPHVF = (t_packet_header_video_full_77*) (pPacket+sizeof(t_packet_header));
   u8 uVideoStreamIndexAndType = pPHVF->video_stream_and_type;
   u8 uVideoStreamIndex = (uVideoStreamIndexAndType & 0x0F);
   u8 uVideoStreamType = ((uVideoStreamIndexAndType & 0xF0) >> 4);

   
   if ( ! ( pPH->packet_flags & PACKET_FLAGS_BIT_RETRANSMITED) )
   if ( pPHVF->video_block_packet_index < pPHVF->block_packets )
   if ( uVideoStreamType == VIDEO_TYPE_H264 )
   if ( pModel->osd_params.osd_flags[pModel->osd_params.layout] & OSD_FLAG_SHOW_STATS_VIDEO_KEYFRAMES_INFO )
   if ( get_ControllerSettings()->iShowVideoStreamInfoCompactType == 0 )
      _parse_single_packet_h264_data(pPacket, bIsRelayedPacket);

   if ( ! (pPH->packet_flags & PACKET_FLAGS_BIT_RETRANSMITED) )
   if ( pPHVF->uVideoStatusFlags2 & VIDEO_STATUS_FLAGS2_HAS_DEBUG_TIMESTAMPS )
   if ( pPHVF->video_block_packet_index < pPHVF->block_packets)
   {
      u8* pExtraData = pPacket + sizeof(t_packet_header) + sizeof(t_packet_header_video_full_77) + pPHVF->video_data_length;
      u32* pExtraDataU32 = (u32*)pExtraData;
      pExtraDataU32[5] = get_current_timestamp_ms();
   }

   int nRet = 0;

   if ( bIsRelayedPacket )
   {
      if ( relay_controller_must_display_remote_video(g_pCurrentModel) )
         nRet = g_pVideoProcessorRxList[iRuntimeIndex]->handleReceivedVideoPacket(iInterfaceIndex, pPacket, pPH->total_length);
   }
   else
   {
      if ( relay_controller_must_display_main_video(g_pCurrentModel) )
         nRet = g_pVideoProcessorRxList[iRuntimeIndex]->handleReceivedVideoPacket(iInterfaceIndex, pPacket, pPH->total_length);
   }
   return nRet;
   */
     return 0;
}

// Returns 1 if end of a video block was reached
// Returns -1 if the packet is not for this vehicle or was not processed

int process_received_video_packet(int iInterfaceIndex, u8* pPacket, int iPacketLength)
{
   if ( g_bSearching || (NULL == pPacket) || (iPacketLength <= 0) )
         return -1;

   t_packet_header* pPH = (t_packet_header*)pPacket;
   u32 uVehicleId = pPH->vehicle_id_src;
   Model* pModel = findModelWithId(uVehicleId, 111);
   if ( (NULL == pModel) || (get_sw_version_build(pModel) < 242) )
      return -1;

   int nRet = 0;
  
   if ( pPH->packet_type == PACKET_TYPE_VIDEO_DATA_98 )
   {
      if ( pPH->packet_flags & PACKET_FLAGS_BIT_RETRANSMITED )
      {
          t_packet_header_video_full_98* pPHVF = (t_packet_header_video_full_98*) (pPacket+sizeof(t_packet_header));
          //log_line("DEBUG recv retr video [%u/%u]", pPHVF->uCurrentBlockIndex, pPHVF->uCurrentBlockPacketIndex);
      }

      nRet = _process_received_video_data_packet(iInterfaceIndex, pPacket, iPacketLength);
   }

   if ( pPH->packet_type == PACKET_TYPE_VIDEO_SWITCH_TO_ADAPTIVE_VIDEO_LEVEL_ACK )
   {
      u32 uRequestId = 0;
      u8 uVideoProfile = 0;
      memcpy((u8*)&uRequestId, pPacket + sizeof(t_packet_header), sizeof(u32));
      memcpy((u8*)&uVideoProfile, pPacket + sizeof(t_packet_header) + sizeof(u32), sizeof(u8));
      adaptive_video_received_video_profile_switch_confirmation(uRequestId, uVideoProfile, pPH->vehicle_id_src);
   }
   return nRet;
}