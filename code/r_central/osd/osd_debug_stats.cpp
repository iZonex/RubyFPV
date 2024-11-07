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

#include "../../base/base.h"
#include "../../base/utils.h"
#include "../../base/ctrl_preferences.h"
#include "../../common/string_utils.h"
#include <math.h>
#include "osd_debug_stats.h"
#include "osd_common.h"
#include "../colors.h"
#include "../shared_vars.h"
#include "../timers.h"

extern float s_OSDStatsLineSpacing;
extern float s_fOSDStatsMargin;
extern float s_fOSDStatsGraphLinesAlpha;
extern u32 s_idFontStats;
extern u32 s_idFontStatsSmall;

bool s_bDebugStatsControllerInfoFreeze = false;
int s_bDebugStatsControllerInfoZoom = 0;
controller_runtime_info s_ControllerRTInfoFreeze;
vehicle_runtime_info s_VehicleRTInfoFreeze;

void osd_debug_stats_toggle_zoom()
{
   s_bDebugStatsControllerInfoZoom++;
   if ( s_bDebugStatsControllerInfoZoom > 2 )
      s_bDebugStatsControllerInfoZoom = 0;
}

void osd_debug_stats_toggle_freeze()
{
   s_bDebugStatsControllerInfoFreeze = ! s_bDebugStatsControllerInfoFreeze;
   if ( s_bDebugStatsControllerInfoFreeze )
   {
      memcpy(&s_ControllerRTInfoFreeze, &g_SMControllerRTInfo, sizeof(controller_runtime_info));
      memcpy(&s_VehicleRTInfoFreeze, &g_SMVehicleRTInfo, sizeof(vehicle_runtime_info));
   }
}

float _osd_render_debug_stats_graph_lines(float xPos, float yPos, float hGraph, float fWidth, int* pValues, int* pValuesMin, int* pValuesMax, int* pValuesAvg, int iCountValues)
{
   char szBuff[32];
   float height_text = g_pRenderEngine->textHeight(g_idFontStats);
   float height_text_small = g_pRenderEngine->textHeight(g_idFontStatsSmall);

   int iMax = -1000;
   int iMin = 1000;
   for( int i=0; i<iCountValues; i++ )
   {
      if ( NULL != pValues )
      if ( pValues[i] < 1000 )
      if ( pValues[i] > iMax )
         iMax = pValues[i];

      if ( NULL != pValuesMax )
      if ( pValuesMax[i] < 1000 )
      if ( pValuesMax[i] > iMax )
         iMax = pValuesMax[i];

      if ( NULL != pValues )
      if ( pValues[i] < 1000 )
      if ( pValues[i] < iMin )
         iMin = pValues[i];

      if ( NULL != pValuesMin )
      if ( pValuesMin[i] < 1000 )
      if ( pValuesMin[i] < iMin )
         iMin = pValuesMin[i];
   }
   g_pRenderEngine->setStrokeSize(OSD_STRIKE_WIDTH);
   
   osd_set_colors();
   g_pRenderEngine->setColors(get_Color_Dev());
   sprintf(szBuff, "%d", iMin);
   g_pRenderEngine->drawText(xPos, yPos+hGraph - height_text_small*0.7, g_idFontStatsSmall, szBuff);
   sprintf(szBuff, "%d", iMax);
   g_pRenderEngine->drawText(xPos, yPos - height_text_small*0.3, g_idFontStatsSmall, szBuff);
   sprintf(szBuff, "%d", (iMax+iMin)/2);
   g_pRenderEngine->drawText(xPos, yPos +hGraph*0.5 - height_text_small*0.5, g_idFontStatsSmall, szBuff);

   float dx = g_pRenderEngine->textWidth(g_idFontStats, "000");
   xPos += dx;
   fWidth -= dx;

   g_pRenderEngine->setStroke(250,250,250,0.5);
   for( float xx=xPos; xx<xPos+fWidth; xx += 0.02 )
   {
      g_pRenderEngine->drawLine(xx, yPos, xx + 0.008, yPos);
      g_pRenderEngine->drawLine(xx, yPos + hGraph*0.5, xx + 0.008, yPos+hGraph*0.5);
   }


   float xBar = xPos;
   float fWidthBar = fWidth / iCountValues;
   float hPoint = 0;
   float hPointPrev = 0;
   for( int i=0; i<iCountValues; i++ )
   {
      if ( NULL != pValuesMin )
      if ( pValuesMin[i] < 1000 )
      {
         hPoint = hGraph * (float)(pValuesMin[i]-(float)iMin) / ((float)iMax-(float)iMin);
         g_pRenderEngine->setStroke(200, 200, 200, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(200, 200, 200, s_fOSDStatsGraphLinesAlpha);
         g_pRenderEngine->drawLine(xBar, yPos + hGraph - hPoint, xBar+fWidthBar, yPos + hGraph - hPoint);
         if ( i > 0 )
         if ( pValuesMin[i-1] < 1000 )
         {
            hPointPrev = hGraph * (float)(pValuesMin[i-1]-(float)iMin) / ((float)iMax-(float)iMin);
            g_pRenderEngine->drawLine(xBar, yPos + hGraph - hPoint, xBar, yPos + hGraph - hPointPrev);
         }
      }
      
      if ( NULL != pValuesMin )
      if ( pValuesMax[i] < 1000 )
      {
         hPoint = hGraph * (float)(pValuesMax[i]-(float)iMin) / ((float)iMax-(float)iMin);
         g_pRenderEngine->setStroke(200, 200, 200, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(200, 200, 200, s_fOSDStatsGraphLinesAlpha);
         g_pRenderEngine->drawLine(xBar, yPos + hGraph - hPoint, xBar+fWidthBar, yPos + hGraph - hPoint);
         if ( i > 0 )
         if ( pValuesMax[i-1] < 1000 )
         {
            hPointPrev = hGraph * (float)(pValuesMax[i-1]-(float)iMin) / ((float)iMax-(float)iMin);
            g_pRenderEngine->drawLine(xBar, yPos + hGraph - hPoint, xBar, yPos + hGraph - hPointPrev);
         }
      }

      if ( NULL != pValuesAvg )
      if ( pValuesAvg[i] < 1000 )
      {
         hPoint = hGraph * (float)(pValuesAvg[i]-(float)iMin) / ((float)iMax-(float)iMin);
         g_pRenderEngine->setStroke(100, 100, 250, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(100, 100, 250, s_fOSDStatsGraphLinesAlpha);
         //g_pRenderEngine->drawLine(xBar, yPos + hGraph - hPoint, xBar+fWidthBar, yPos + hGraph - hPoint);
         if ( i > 0 )
         if ( pValuesAvg[i-1] < 1000 )
         {
            hPointPrev = hGraph * (float)(pValuesAvg[i-1]-(float)iMin) / ((float)iMax-(float)iMin);
            //g_pRenderEngine->drawLine(xBar, yPos + hGraph - hPoint, xBar, yPos + hGraph - hPointPrev);
            g_pRenderEngine->drawLine(xBar-fWidthBar, yPos + hGraph - hPointPrev, xBar, yPos + hGraph - hPoint);
         }
      }

      xBar += fWidthBar;
   }
   osd_set_colors();
   g_pRenderEngine->setColors(get_Color_Dev());
   return hGraph;
}

float _osd_render_debug_stats_graph_bars(float xPos, float yPos, float hGraph, float fWidth, u8* pValues, u8* pValues2, int iCountValues, int iLog)
{
   char szBuff[32];
   float height_text = g_pRenderEngine->textHeight(g_idFontStats);
   float height_text_small = g_pRenderEngine->textHeight(g_idFontStatsSmall);

   int iMax = 0;
   for( int i=0; i<iCountValues; i++ )
   {
      if ( pValues[i] > iMax )
         iMax = pValues[i];
      if ( NULL != pValues2 )
      if ( (int)pValues[i] + (int)pValues2[i] > iMax )
         iMax = (int)pValues[i] + (int)pValues2[i];
   }
   float fMaxLog = logf((float)iMax);

   g_pRenderEngine->setStrokeSize(OSD_STRIKE_WIDTH);
   
   osd_set_colors();
   g_pRenderEngine->setColors(get_Color_Dev());
   g_pRenderEngine->drawText(xPos, yPos+hGraph - height_text_small*0.7, g_idFontStatsSmall, "0");
   sprintf(szBuff, "%d", iMax);
   g_pRenderEngine->drawText(xPos, yPos - height_text_small*0.3, g_idFontStatsSmall, szBuff);
   sprintf(szBuff, "%d", iMax/2);
   g_pRenderEngine->drawText(xPos, yPos +hGraph*0.5 - height_text_small*0.5, g_idFontStatsSmall, szBuff);

   float dx = g_pRenderEngine->textWidth(g_idFontStats, "000");
   xPos += dx;
   fWidth -= dx;
   float xBar = xPos;
   float fWidthBar = fWidth / iCountValues;

   for( int i=0; i<iCountValues; i++ )
   {
      float hBar = hGraph * (float)pValues[i] / (float)iMax;
      float hBar2 = 0;
      if ( NULL != pValues2 )
         hBar2 = hGraph * (float)pValues2[i] / (float)iMax;

      if ( iLog )
      {
         if ( pValues[i] > 0 )
            hBar = hGraph * logf((float)pValues[i]) / fMaxLog;
         if ( NULL != pValues2 )
         if ( pValues2[i] > 0 )
         //   hBar2 = hGraph * logf((float)pValues2[i]) / fMaxLog;
            hBar2 = hGraph * logf((float)pValues[i] + (float)pValues2[i]) / fMaxLog - hBar;
      }

      if ( pValues[i] > 0 )
      {
         g_pRenderEngine->setStroke(200, 200, 200, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(200, 200, 200, s_fOSDStatsGraphLinesAlpha);
         g_pRenderEngine->drawRect(xBar, yPos + hGraph - hBar, fWidthBar - g_pRenderEngine->getPixelWidth(), hBar);
      }
      if ( NULL != pValues2 )
      if ( pValues2[i] > 0 )
      {
         g_pRenderEngine->setStroke(0, 200, 0, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(0, 200, 0, s_fOSDStatsGraphLinesAlpha);
         g_pRenderEngine->drawRect(xBar, yPos + hGraph - hBar-hBar2, fWidthBar - g_pRenderEngine->getPixelWidth(), hBar2);
      }
      xBar += fWidthBar;
   }
   osd_set_colors();
   g_pRenderEngine->setColors(get_Color_Dev());
   return hGraph;
}


float _osd_render_debug_stats_graph_values(float xPos, float yPos, float hGraph, float fWidth, u8* pValues, int iCountValues)
{
   char szBuff[32];
   float height_text = g_pRenderEngine->textHeight(g_idFontStats);
   float height_text_small = g_pRenderEngine->textHeight(g_idFontStatsSmall);

   int iMax = 0;
   for( int i=0; i<iCountValues; i++ )
   {
      if ( pValues[i] > iMax )
         iMax = pValues[i];
   }

   g_pRenderEngine->setStrokeSize(OSD_STRIKE_WIDTH);
   
   osd_set_colors();
   g_pRenderEngine->setColors(get_Color_Dev());
   g_pRenderEngine->drawText(xPos, yPos+hGraph - height_text_small*0.7, g_idFontStatsSmall, "0");
   sprintf(szBuff, "%d", iMax);
   g_pRenderEngine->drawText(xPos, yPos - height_text_small*0.3, g_idFontStatsSmall, szBuff);

   float dx = g_pRenderEngine->textWidth(g_idFontStats, "000");
   xPos += dx;
   fWidth -= dx;
   float xBar = xPos;
   float fWidthBar = fWidth / iCountValues;

   for( int i=0; i<iCountValues; i++ )
   {
      float hBar = hGraph*0.5;
      float dyBar = 0.0;
      if ( pValues[i] == 0 )
      {
         xBar += fWidthBar;
         continue;
      }
      if ( pValues[i] == 1 )
      {
         g_pRenderEngine->setStroke(220, 0, 0, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(220, 0, 0, s_fOSDStatsGraphLinesAlpha);
         hBar = hGraph * 0.7;
      }
      else if ( pValues[i] == 2 )
      {
         g_pRenderEngine->setStroke(250, 130, 130, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(250, 130, 130, s_fOSDStatsGraphLinesAlpha);
         dyBar = 0.5*hGraph;
      }
      else if ( pValues[i] == 3 )
      {
         g_pRenderEngine->setStroke(80, 80, 250, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(80, 80, 250, s_fOSDStatsGraphLinesAlpha);
         dyBar = 0.5*hGraph;
      }
      else if ( pValues[i] == 4 )
      {
         hBar = hGraph;
         g_pRenderEngine->setStroke(250, 250, 50, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(250, 250, 50, s_fOSDStatsGraphLinesAlpha);
      }
      else if ( pValues[i] > 4 )
      {
         hBar = hGraph;
         g_pRenderEngine->setStroke(250, 250, 250, OSD_STRIKE_WIDTH);
         g_pRenderEngine->setFill(250, 250, 250, s_fOSDStatsGraphLinesAlpha);
      }
      g_pRenderEngine->drawRect(xBar, yPos + hGraph - hBar - dyBar, fWidthBar - g_pRenderEngine->getPixelWidth(), hBar);
      xBar += fWidthBar;
   }
   osd_set_colors();
   g_pRenderEngine->setColors(get_Color_Dev());
   return hGraph;
}

void osd_render_debug_stats()
{
   Preferences* pP = get_Preferences();
   Model* pActiveModel = osd_get_current_data_source_vehicle_model();
   controller_runtime_info* pCRTInfo = &g_SMControllerRTInfo;
   //vehicle_runtime_info* pVRTInfo = &g_SMVehicleRTInfo;
   if ( s_bDebugStatsControllerInfoFreeze )
   {
       pCRTInfo = &s_ControllerRTInfoFreeze;
       //pVRTInfo = &s_VehicleRTInfoFreeze;
   }
   controller_runtime_info_vehicle* pCRTInfoVehicle = controller_rt_info_get_vehicle_info(pCRTInfo, pActiveModel->uVehicleId);

   int iStartIntervals = 0;
   int iCountIntervals = SYSTEM_RT_INFO_INTERVALS;
   if ( 1 == s_bDebugStatsControllerInfoZoom )
   {
     iCountIntervals = SYSTEM_RT_INFO_INTERVALS/2;
   }
   if ( 2 == s_bDebugStatsControllerInfoZoom )
   {
      iStartIntervals = SYSTEM_RT_INFO_INTERVALS/2-1;
      iCountIntervals = SYSTEM_RT_INFO_INTERVALS/2;
   }
   int iIndexVehicleStart = iStartIntervals + pCRTInfo->iDeltaIndexFromVehicle;
   if ( iIndexVehicleStart < 0 )
      iIndexVehicleStart += SYSTEM_RT_INFO_INTERVALS;
   if ( iIndexVehicleStart >= SYSTEM_RT_INFO_INTERVALS )
      iIndexVehicleStart -= SYSTEM_RT_INFO_INTERVALS;

   s_idFontStats = g_idFontStats;
   s_idFontStatsSmall = g_idFontStatsSmall;
   s_OSDStatsLineSpacing = 1.0;

   float height_text = g_pRenderEngine->textHeight(s_idFontStats);
   float height_text_small = g_pRenderEngine->textHeight(s_idFontStatsSmall);
   float hGraph = height_text * 3.0;
   float hGraphSmall = height_text * 1.2;

   float xPos = 0.03;
   float yPos = 0.17;
   float width = 0.94;
   float height = 0.7;

   char szBuff[128];

   osd_set_colors_background_fill(g_fOSDStatsBgTransparency);
   g_pRenderEngine->drawRoundRect(xPos, yPos, width, height, 1.5*POPUP_ROUND_MARGIN);
   osd_set_colors();
   g_pRenderEngine->setColors(get_Color_Dev());


   xPos += s_fOSDStatsMargin/g_pRenderEngine->getAspectRatio();
   yPos += s_fOSDStatsMargin*0.7;
   width -= 2*s_fOSDStatsMargin/g_pRenderEngine->getAspectRatio();
   height -= s_fOSDStatsMargin*0.7*2.0;

   float widthMax = width;
   float rightMargin = xPos + width;

   float fGraphXStart = xPos;
   float fWidthGraph = widthMax;

   float dx = g_pRenderEngine->textWidth(g_idFontStats, "000");
   float xPosSlice = fGraphXStart + dx;
   float fWidthBar = (fWidthGraph-dx) / iCountIntervals;
   float fWidthPixel = g_pRenderEngine->getPixelWidth();

   g_pRenderEngine->drawText(xPos, yPos, s_idFontStats, "Debug Stats");
   
   sprintf(szBuff, "%d %u ms", g_SMControllerRTInfo.iCurrentIndex, g_SMControllerRTInfo.uCurrentSliceStartTime);
   g_pRenderEngine->drawTextLeft(rightMargin, yPos, s_idFontStatsSmall, szBuff);
   float y = yPos;
   y += height_text*s_OSDStatsLineSpacing;

   float yTop = y;

   g_pRenderEngine->setStrokeSize(OSD_STRIKE_WIDTH);
   g_pRenderEngine->setStroke(180,180,180, OSD_STRIKE_WIDTH);

   for( float yLine=yPos; yLine<yPos+height; yLine += 0.03 )
   {
      for( float dxLine=0.1; dxLine<1.0; dxLine += 0.1 )
      g_pRenderEngine->drawLine(fGraphXStart+dx + fWidthGraph*dxLine, yLine, fGraphXStart+dx + fWidthGraph*dxLine, yLine + 0.03);
   }
   osd_set_colors();
   g_pRenderEngine->setColors(get_Color_Dev());


   u8 uTmp[SYSTEM_RT_INFO_INTERVALS];
   u8 uTmp1[SYSTEM_RT_INFO_INTERVALS];
   u8 uTmp2[SYSTEM_RT_INFO_INTERVALS];
   u8 uTmp3[SYSTEM_RT_INFO_INTERVALS];
   u8 uTmp4[SYSTEM_RT_INFO_INTERVALS];

   int iTmp1[SYSTEM_RT_INFO_INTERVALS];
   int iTmp2[SYSTEM_RT_INFO_INTERVALS];
   int iTmp3[SYSTEM_RT_INFO_INTERVALS];
   int iTmp4[SYSTEM_RT_INFO_INTERVALS];

   //--------------------------------------------
   /*
   int iVehicleIndex = iIndexVehicleStart;
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pVRTInfo->uSentVideoDataPackets[iVehicleIndex];
      uTmp2[i] = pVRTInfo->uSentVideoECPackets[iVehicleIndex];
      iVehicleIndex++;
      if ( iVehicleIndex >= SYSTEM_RT_INFO_INTERVALS )
         iVehicleIndex = 0;
   }

   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Vehicle Tx Video Packets");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_bars(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, uTmp2, iCountIntervals, 1);
   y += height_text_small;
   /**/

   //--------------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_RX_VIDEO_DATA_PACKETS )
   {
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uRxVideoPackets[i+iStartIntervals][0];
      uTmp2[i] = pCRTInfo->uRxDataPackets[i+iStartIntervals][0];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Rx Video/Data Packets");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_bars(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, uTmp2, iCountIntervals, 1);
   y += height_text_small;
   }

   //--------------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_RX_H264265_FRAMES )
   {
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uRecvEndOfFrame[i+iStartIntervals];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Received P/I frames end");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_values(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, iCountIntervals);
   y += height_text_small;
   }

   //-----------------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_RX_DBM )
   {
   for( int iInt=0; iInt<hardware_get_radio_interfaces_count(); iInt++)
   {
   for( int iAnt=0; iAnt<pCRTInfo->radioInterfacesDbm[0][iInt].iCountAntennas; iAnt++ )
   {
      for( int i=0; i<iCountIntervals; i++ )
      {
         iTmp1[i] = pCRTInfo->radioInterfacesDbm[i+iStartIntervals][iInt].iDbmLast[iAnt];
         iTmp2[i] = pCRTInfo->radioInterfacesDbm[i+iStartIntervals][iInt].iDbmMin[iAnt];
         iTmp3[i] = pCRTInfo->radioInterfacesDbm[i+iStartIntervals][iInt].iDbmMax[iAnt];
         iTmp4[i] = pCRTInfo->radioInterfacesDbm[i+iStartIntervals][iInt].iDbmAvg[iAnt];
      }
      sprintf(szBuff, "Dbm (interface %d, antenna: %d)", iInt+1, iAnt+1);
      g_pRenderEngine->drawText(xPos, y, s_idFontStats, szBuff);
      g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Dbm");
      y += height_text*1.3;
      y += _osd_render_debug_stats_graph_lines(fGraphXStart, y, hGraph, fWidthGraph, iTmp1, iTmp2, iTmp3, iTmp4, iCountIntervals);
      y += height_text_small;
   }
   }
   }

   //-----------------------------------------------
   /*
   for( int iInt=0; iInt<hardware_get_radio_interfaces_count(); iInt++)
   {
   for( int iAnt=0; iAnt<pCRTInfo->radioInterfacesDbm[0][iInt].iCountAntennas; iAnt++ )
   {
      for( int i=0; i<iCountIntervals; i++ )
      {
         iTmp2[i] = pCRTInfo->radioInterfacesDbm[i+iStartIntervals][iInt].iDbmChangeSpeedMin[iAnt];
         iTmp3[i] = pCRTInfo->radioInterfacesDbm[i+iStartIntervals][iInt].iDbmChangeSpeedMax[iAnt];
      }
      sprintf(szBuff, "Dbm Change Speed (interface %d, antenna: %d)", iInt+1, iAnt+1);
      g_pRenderEngine->drawText(xPos, y, s_idFontStats, szBuff);
      g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Dbm");
      y += height_text*1.3;
      y += _osd_render_debug_stats_graph_lines(fGraphXStart, y, hGraph, fWidthGraph, NULL, iTmp2, iTmp3, NULL, iCountIntervals);
      y += height_text_small;
   }
   }
   */

   //--------------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_RX_MISSING_PACKETS )
   {
   for( int iInt=0; iInt<hardware_get_radio_interfaces_count(); iInt++)
   {
      for( int i=0; i<iCountIntervals; i++ )
      {
         uTmp[i] = pCRTInfo->uRxMissingPackets[i+iStartIntervals][iInt];
      }
      sprintf(szBuff, "Rx Missing Packets (interface %d)", iInt+1);
      g_pRenderEngine->drawText(xPos, y, s_idFontStats, szBuff);
      y += height_text*1.3;
      y += _osd_render_debug_stats_graph_values(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, iCountIntervals);
      y += height_text_small;
   }
   }

   //--------------------------------------------------------------------------

   g_pRenderEngine->setStrokeSize(1.0);
   g_pRenderEngine->setStroke(255,255,255, 1.0);
   xPosSlice = fGraphXStart + dx;
   for( int i=0; i<iCountIntervals; i++ )
   {
      xPosSlice += fWidthBar;
      g_pRenderEngine->drawLine(xPosSlice + fWidthBar*0.5, y, xPosSlice + fWidthBar*0.5, y + height_text_small);
      if ( 0 != s_bDebugStatsControllerInfoZoom )
      {
         g_pRenderEngine->drawLine(xPosSlice + fWidthBar*0.5 + fWidthPixel, y, xPosSlice + fWidthBar*0.5 + fWidthPixel, y + height_text_small);
         g_pRenderEngine->drawLine(xPosSlice + fWidthBar*0.5 + fWidthPixel*2.0, y, xPosSlice + fWidthBar*0.5 + fWidthPixel*2.0, y + height_text_small);
      }
   }
   y += height_text_small;

   //--------------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_RX_MISSING_PACKETS_MAX_GAP )
   {
   for( int iInt=0; iInt<hardware_get_radio_interfaces_count(); iInt++)
   {
      for( int i=0; i<iCountIntervals; i++ )
      {
         uTmp[i] = pCRTInfo->uRxMissingPacketsMaxGap[i+iStartIntervals][iInt];
      }
      sprintf(szBuff, "Rx Missing Packets Max Gap (interface %d)", iInt+1);
      g_pRenderEngine->drawText(xPos, y, s_idFontStats, szBuff);
      y += height_text*1.3;
      y += _osd_render_debug_stats_graph_bars(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, NULL, iCountIntervals, 0);
      y += height_text_small;
   }
   }

   //--------------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_RX_CONSUMED_PACKETS )
   {
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uRxProcessedPackets[i+iStartIntervals];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Rx Consumed Packets");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_bars(fGraphXStart, y, hGraph, fWidthGraph, uTmp, NULL, iCountIntervals, 1);
   y += height_text_small;
   }

   //--------------------------------------------
   /*
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uOutputedVideoPackets[i+iStartIntervals];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Outputed clean video packets");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_bars(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, NULL, iCountIntervals, 1);
   y += height_text_small;
   */

   //--------------------------------------------
   /*
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uOutputedVideoPacketsSingleECUsed[i+iStartIntervals];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Outputed video with single EC used");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_values(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, iCountIntervals);
   y += height_text_small;

   //--------------------------------------------
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uOutputedVideoPacketsTwoECUsed[i+iStartIntervals];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Outputed video with two EC used");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_values(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, iCountIntervals);
   y += height_text_small;

   //--------------------------------------------
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uOutputedVideoPacketsMultipleECUsed[i+iStartIntervals];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Outputed video with more than two EC used");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_values(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, iCountIntervals);
   y += height_text_small;
   */

   // ----------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_MIN_MAX_ACK_TIME )
   {
   controller_runtime_info_vehicle* pRTInfoVehicle = controller_rt_info_get_vehicle_info(pCRTInfo, pActiveModel->uVehicleId);
   for( int i=0; i<iCountIntervals; i++ )
   {
      iTmp1[i] = pRTInfoVehicle->uMinAckTime[i+iStartIntervals];  
      iTmp2[i] = pRTInfoVehicle->uMaxAckTime[i+iStartIntervals];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Min/Max Ack Time (ms)");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_lines(fGraphXStart, y, hGraphSmall*1.3, fWidthGraph, NULL, iTmp1, iTmp2, NULL, iCountIntervals);
   y += height_text_small;
   }

   // ----------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_RX_VIDEO_MAX_EC_USED )
   {
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uOutputedVideoPacketsMaxECUsed[i+iStartIntervals];  
   }
   sprintf(szBuff, "Outputed video EC max packets used ( %d / %d )", pActiveModel->video_link_profiles[pActiveModel->video_params.user_selected_video_link_profile].block_packets, pActiveModel->video_link_profiles[pActiveModel->video_params.user_selected_video_link_profile].block_fecs);
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, szBuff);
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_values(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, iCountIntervals);
   y += height_text_small;
   }

   //--------------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_RX_VIDEO_UNRECOVERABLE_BLOCKS )
   {
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uOutputedVideoPacketsSkippedBlocks[i+iStartIntervals];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Unrecoverable Video Blocks skipped");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_values(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, iCountIntervals);
   y += height_text_small;
   }

   //--------------------------------------------
   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_VIDEO_RETRANSMISSIONS )
   {
      for( int i=0; i<iCountIntervals; i++ )
      {
         uTmp[i] = pCRTInfoVehicle->uCountReqRetransmissions[i+iStartIntervals];
      }
      g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Requested Retransmissions");
      y += height_text*1.3;
      y += _osd_render_debug_stats_graph_bars(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, NULL, iCountIntervals, 0);
      y += height_text_small;

      for( int i=0; i<iCountIntervals; i++ )
      {
         uTmp[i] = pCRTInfoVehicle->uCountAckRetransmissions[i+iStartIntervals];
      }
      g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Ack Retransmissions");
      y += height_text*1.3;
      y += _osd_render_debug_stats_graph_bars(fGraphXStart, y, hGraphSmall, fWidthGraph, uTmp, NULL, iCountIntervals, 0);
      y += height_text_small;
   }

   //--------------------------------------------
   /*
   for( int i=0; i<iCountIntervals; i++ )
   {
      uTmp[i] = pCRTInfo->uRecvVideoDataPackets[i+iStartIntervals];
      uTmp2[i] = pCRTInfo->uRecvVideoECPackets[i+iStartIntervals];
   }
   g_pRenderEngine->drawText(xPos, y, s_idFontStats, "Rx Video Decode Data/EC");
   y += height_text*1.3;
   y += _osd_render_debug_stats_graph_bars(fGraphXStart, y, hGraph, fWidthGraph, uTmp, uTmp2, iCountIntervals, 1);
   y += height_text_small;
   /**/
   //------------------------------------------------


   // ----------------------------------------------
   float fStrokeSize = 2.0;
   g_pRenderEngine->setStrokeSize(fStrokeSize);
   float yMid = 0.5*(y+yTop);
   xPosSlice = fGraphXStart + dx;

   if ( pP->uDebugStatsFlags & CTRL_RT_DEBUG_INFO_FLAG_SHOW_VIDEO_PROFILE_CHANGES )
   {
   for( int i=0; i<iCountIntervals; i++ )
   {
      u32 uFlags = pCRTInfo->uFlagsAdaptiveVideo[i+iStartIntervals];
      if ( 0 == uFlags )
      {
         xPosSlice += fWidthBar;
         continue;
      }

      if ( uFlags & CTRL_RT_INFO_FLAG_VIDEO_PROF_SWITCHED_USER_SELECTABLE )
      {
         g_pRenderEngine->setStroke(80, 80, 250, fStrokeSize);
         g_pRenderEngine->setFill(0, 0, 0, 0);
         g_pRenderEngine->drawRect(xPosSlice - fWidthPixel, yTop, fWidthBar + 2.0*fWidthPixel, y - yTop);
      }
      if ( uFlags & CTRL_RT_INFO_FLAG_VIDEO_PROF_SWITCHED_LOWER )
      {
         g_pRenderEngine->setStroke(250, 80, 80, fStrokeSize);
         g_pRenderEngine->setFill(0, 0, 0, 0);
         g_pRenderEngine->drawRect(xPosSlice - fWidthPixel, yTop, fWidthBar + 2.0*fWidthPixel, y - yTop);
      }
      if ( uFlags & CTRL_RT_INFO_FLAG_VIDEO_PROF_SWITCHED_HIGHER )
      {
         g_pRenderEngine->setStroke(80, 250, 80, fStrokeSize);
         g_pRenderEngine->setFill(0, 0, 0, 0);
         g_pRenderEngine->drawRect(xPosSlice - fWidthPixel, yTop, fWidthBar + 2.0*fWidthPixel, y - yTop);
      }

      if ( uFlags & CTRL_RT_INFO_FLAG_VIDEO_PROF_SWITCH_REQ_BY_USER )
      {
         g_pRenderEngine->setStroke(80, 80, 250, fStrokeSize);
         g_pRenderEngine->setFill(0, 0, 0, 0);
         g_pRenderEngine->drawLine(xPosSlice - fWidthPixel, yMid, xPosSlice - fWidthPixel, y);
         g_pRenderEngine->drawLine(xPosSlice, yMid, xPosSlice, y);
      }
      if ( uFlags & CTRL_RT_INFO_FLAG_VIDEO_PROF_SWITCH_REQ_BY_ADAPTIVE_LOWER )
      {
         g_pRenderEngine->setStroke(250, 80, 80, fStrokeSize);
         g_pRenderEngine->setFill(0, 0, 0, 0);
         g_pRenderEngine->drawLine(xPosSlice - fWidthPixel, yMid, xPosSlice - fWidthPixel, y);
         g_pRenderEngine->drawLine(xPosSlice, yMid, xPosSlice, y);
      }
      if ( uFlags & CTRL_RT_INFO_FLAG_VIDEO_PROF_SWITCH_REQ_BY_ADAPTIVE_HIGHER )
      {
         g_pRenderEngine->setStroke(80, 250, 80, fStrokeSize);
         g_pRenderEngine->setFill(0, 0, 0, 0);
         g_pRenderEngine->drawLine(xPosSlice - fWidthPixel, yMid, xPosSlice - fWidthPixel, y);
         g_pRenderEngine->drawLine(xPosSlice, yMid, xPosSlice, y);
      }

      if ( uFlags & CTRL_RT_INFO_FLAG_RECV_ACK )
      {
         g_pRenderEngine->setStroke(50, 50, 250, fStrokeSize);
         g_pRenderEngine->setFill(0, 0, 0, 0);
         g_pRenderEngine->drawLine(xPosSlice + fWidthPixel, yMid, xPosSlice + fWidthPixel, y);
         g_pRenderEngine->drawLine(xPosSlice + fWidthPixel*2, yMid, xPosSlice + fWidthPixel*2, y);
      }

      xPosSlice += fWidthBar;
   }
   }
   
   float xLine = fGraphXStart + pCRTInfo->iCurrentIndex * fWidthBar;
   g_pRenderEngine->setStrokeSize(OSD_STRIKE_WIDTH);
   g_pRenderEngine->setStroke(255,255,100, OSD_STRIKE_WIDTH);
   g_pRenderEngine->drawLine(xLine, yPos, xLine, yPos+height);
   //g_pRenderEngine->drawLine(xLine+g_pRenderEngine->getPixelWidth(), yPos, xLine + g_pRenderEngine->getPixelWidth(), yPos+height);
   osd_set_colors();
}