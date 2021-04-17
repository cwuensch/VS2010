#define _LARGEFILE_SOURCE   1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS  64
#ifdef _MSC_VER
  #define inline
#endif

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "type.h"
#include "H264.h"

// Table E-1.
static const tFraction sar_idc[] = {
  {   0,  0 },  // Unspecified (never written here).
  {   1,  1 }, {  12, 11 }, {  10, 11 }, {  16, 11 },
  {  40, 33 }, {  24, 11 }, {  20, 11 }, {  32, 11 },
  {  80, 33 }, {  18, 11 }, {  15, 11 }, {  64, 33 },
  { 160, 99 }, {   4,  3 }, {   3,  2 }, {   2,  1 },
};

static const unsigned char *m_pStart;
static unsigned short m_nLength;
static int m_nCurrentBit;


// from https://stackoverflow.com/questions/12018535/get-the-width-height-of-the-video-from-h-264-nalu

static unsigned int ReadBit()
{
  int nIndex = m_nCurrentBit / 8;
  int nOffset = m_nCurrentBit % 8 + 1;
  assert(m_nCurrentBit <= m_nLength * 8);

  m_nCurrentBit ++;
  return (m_pStart[nIndex] >> (8-nOffset)) & 0x01;
}

static unsigned int ReadBits(int n)
{
  int r = 0;
  int i;
  for (i = 0; i < n; i++)
  {
    r |= ( ReadBit() << ( n - i - 1 ) );
  }
  return r;
}

static unsigned int ReadExponentialGolombCode()
{
  int r = 0;
  int i = 0;

  while( (ReadBit() == 0) && (i < 32) )
    i++;

  r = ReadBits(i);
  r += (1 << i) - 1;
  return r;
}

static unsigned int ReadSE() 
{
  int r = ReadExponentialGolombCode();
  if (r & 0x01)
    r = (r+1)/2;
  else
    r = -(r/2);
  return r;
}

/*static void ReadHRD(hrd_t *hrd)
{
  int i;
  hrd->cpb_cnt_minus1 = ReadExponentialGolombCode();
  hrd->bit_rate_scale = ReadBits(4);
  hrd->cpb_size_scale = ReadBits(4);
  for (i = 0; i <= hrd->cpb_cnt_minus1; i++ )
  {
    hrd->bit_rate_value_minus1[i] = ReadExponentialGolombCode();
    hrd->cpb_size_value_minus1[i] = ReadExponentialGolombCode();
    hrd->cbr_flag[i] = ReadBit();
  }
  hrd->initial_cpb_removal_delay_length_minus1 = ReadBits(5);
  hrd->cpb_removal_delay_length_minus1 = ReadBits(5);
  hrd->dpb_output_delay_length_minus1 = ReadBits(5);
  hrd->time_offset_length = ReadBits(5);
} */


int EBSPtoRBSP(byte *streamBuffer, int maxLen)
{
  int i=0, j=0, count=0;

  for (i = 0; i < maxLen; i++)
  { //starting from begin_bytepos to avoid header information
    //in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any byte-aligned position
    if (count == 2 && streamBuffer[i] < 0x03)  // Ende ist erreicht
      return j - 2;
    if (count == 2 && streamBuffer[i] == 0x03)
    {
      i++;
      count = 0;
    }
    streamBuffer[j] = streamBuffer[i];
    if(streamBuffer[i] == 0x00)
      count++;
    else
      count = 0;
    j++;
  }
  return -1;
}


// Rec. ITU-T H.264 (https://www.itu.int/rec/T-REC-H.264-201402-S/en)
// 7.3.2.1.1 Sequence parameter set data syntax
bool ParseSPS(const unsigned char *pStart, unsigned short nLen, int *const VidHeight, int *const VidWidth, double *const VidFPS, double *const VidDAR)
{
  sps_t sps;
  vui_t *vui = &sps.vui;
  int cropUnitX, cropUnitY;

  m_pStart = pStart;
  m_nLength = nLen;
  m_nCurrentBit = 0;

  memset(&sps, 0, sizeof(sps));

  sps.profile_idc = ReadBits(8);
  sps.constraint_set0_flag = ReadBit();
  sps.constraint_set1_flag = ReadBit();
  sps.constraint_set2_flag = ReadBit();
  sps.constraint_set3_flag = ReadBit();
  sps.constraint_set4_flag = ReadBit();
  sps.constraint_set5_flag = ReadBit();
  sps.reserved_zero_2bits  = ReadBits(2);
  sps.level_idc = ReadBits(8);
  sps.seq_parameter_set_id = ReadExponentialGolombCode();

  if( sps.profile_idc == 100 || sps.profile_idc == 110 ||
      sps.profile_idc == 122 || sps.profile_idc == 244 ||
      sps.profile_idc == 44 || sps.profile_idc == 83 ||
      sps.profile_idc == 86 || sps.profile_idc == 118 )
  {
    sps.chroma_format_idc = ReadExponentialGolombCode();
    if( sps.chroma_format_idc == 3 )
    {
      sps.residual_colour_transform_flag = ReadBit();         
    }
    sps.bit_depth_luma_minus8 = ReadExponentialGolombCode();        
    sps.bit_depth_chroma_minus8 = ReadExponentialGolombCode();      
    sps.qpprime_y_zero_transform_bypass_flag = ReadBit();       
    sps.seq_scaling_matrix_present_flag = ReadBit();        

    if (sps.seq_scaling_matrix_present_flag) 
    {
      int i=0;
      for ( i = 0; i < 8; i++) 
      {
        sps.seq_scaling_list_present_flag[i] = ReadBit();
        if (sps.seq_scaling_list_present_flag[i]) 
        {
          int sizeOfScalingList = (i < 6) ? 16 : 64;
          int lastScale = 8;
          int nextScale = 8;
          int j=0;
          for ( j = 0; j < sizeOfScalingList; j++) 
          {
            if (nextScale != 0) 
            {
              int delta_scale = ReadSE();
              nextScale = (lastScale + delta_scale + 256) % 256;
//              if (i < 6)
//                sps.ScalingList4x4[i][j] = nextScale;
//              else
//                sps.ScalingList8x8[i-6][j] = nextScale;
            }
            lastScale = (nextScale == 0) ? lastScale : nextScale;
          }
        }
      }
    }
  }

  sps.log2_max_frame_num_minus4 = ReadExponentialGolombCode();
  sps.pic_order_cnt_type = ReadExponentialGolombCode();
  if (sps.pic_order_cnt_type == 0)
  {
    sps.log2_max_pic_order_cnt_lsb_minus4 = ReadExponentialGolombCode();
  }
  else if( sps.pic_order_cnt_type == 1 )
  {
    int i;
    sps.delta_pic_order_always_zero_flag = ReadBit();
    sps.offset_for_non_ref_pic = ReadSE();
    sps.offset_for_top_to_bottom_field = ReadSE();
    sps.num_ref_frames_in_pic_order_cnt_cycle = ReadExponentialGolombCode();
    for( i = 0; i < sps.num_ref_frames_in_pic_order_cnt_cycle; i++ )
    {
      //sps.offset_for_ref_frame[i] = ReadSE();
      ReadSE();
    }
  }

  sps.max_num_ref_frames = ReadExponentialGolombCode();
  sps.gaps_in_frame_num_value_allowed_flag = ReadBit();
  sps.pic_width_in_mbs_minus1 = ReadExponentialGolombCode();
  sps.pic_height_in_map_units_minus1 = ReadExponentialGolombCode();

  sps.frame_mbs_only_flag = ReadBit();
  if( !sps.frame_mbs_only_flag )
  {
    sps.mb_adaptive_frame_field_flag = ReadBit();
  }
  sps.direct_8x8_inference_flag = ReadBit();

  sps.frame_cropping_flag = ReadBit();
  if (sps.frame_cropping_flag)
  {
    sps.frame_crop_left_offset = ReadExponentialGolombCode();
    sps.frame_crop_right_offset = ReadExponentialGolombCode();
    sps.frame_crop_top_offset = ReadExponentialGolombCode();
    sps.frame_crop_bottom_offset = ReadExponentialGolombCode();
  }

  sps.vui_parameters_present_flag = ReadBit();
  if (sps.vui_parameters_present_flag)
  {
    // E.1.1 VUI parameters syntax
    // from https://github.com/cplussharp/graph-studio-next/blob/master/src/H264StructReader.cpp
    memset(vui,0,sizeof(vui_t));

    vui->aspect_ratio_info_present_flag = ReadBit();
    if (vui->aspect_ratio_info_present_flag)
    {
      vui->aspect_ratio_idc = ReadBits(8);
      if (vui->aspect_ratio_idc == 255)  // Extended_SAR
      {
        vui->sar_width = ReadBits(16);
        vui->sar_height = ReadBits(16);
      }
    }

    vui->overscan_info_present_flag = ReadBit();
    if (vui->overscan_info_present_flag)
      vui->overscan_appropriate_flag = ReadBit();

    vui->video_signal_type_present_flag = ReadBit();
    if (vui->video_signal_type_present_flag)
    {
      vui->video_format = ReadBits(3);
      vui->video_full_range_flag = ReadBit();
      vui->colour_description_present_flag = ReadBit();
      if (vui->colour_description_present_flag)
      {
        vui->colour_primaries = ReadBits(8);
        vui->transfer_characteristics = ReadBits(8);
        vui->matrix_coefficients = ReadBits(8);
      }
    }

    vui->chroma_loc_info_present_flag = ReadBit();
    if (vui->chroma_loc_info_present_flag)
    {
      vui->chroma_sample_loc_type_top_field = ReadExponentialGolombCode();
      vui->chroma_sample_loc_type_bottom_field = ReadExponentialGolombCode();
    }
    
    vui->timing_info_present_flag = ReadBit();
    if (vui->timing_info_present_flag)
    {
      vui->num_units_in_tick = ReadBits(32);
      vui->time_scale = ReadBits(32);
      vui->fixed_frame_rate_flag = ReadBit();
    }
    
/*    vui->nal_hrd_parameters_present_flag = ReadBit();
    if (vui->nal_hrd_parameters_present_flag)
      ReadHRD(&vui->nal_hrd_parameters);

    vui->vcl_hrd_parameters_present_flag = ReadBit();
    if (vui->vcl_hrd_parameters_present_flag)
      ReadHRD(&vui->vcl_hrd_parameters);

    if (vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
      vui->low_delay_hrd_flag = ReadBit();

    vui->pic_struct_present_flag = ReadBit();

    vui->bitstream_restriction_flag = ReadBit();
    if (vui->bitstream_restriction_flag)
    {
      vui->motion_vectors_over_pic_boundaries_flag = ReadBit();
      vui->max_bytes_per_pic_denom = ReadExponentialGolombCode();
      vui->max_bits_per_mb_denom = ReadExponentialGolombCode();
      vui->log2_max_mv_length_horizontal = ReadExponentialGolombCode();
      vui->log2_max_mv_length_vertical = ReadExponentialGolombCode();
      vui->num_reorder_frames = ReadExponentialGolombCode();
      vui->max_dec_frame_buffering = ReadExponentialGolombCode();
    } */
  }
  
#ifdef DEBUG
{
  int i;
  printf("DEBUG SPS: ");
  for (i = 0; i < max((m_nCurrentBit+7)/8, 26); i++)
    printf(" %02X", pStart[i]);
  printf("\n");
}
#endif

  if (sps.chroma_format_idc == 0)
  {
    cropUnitX = 1;
    cropUnitY = 2 - sps.frame_mbs_only_flag;
  }
  else /*if (sps.chroma_format_idc <= 3)*/
  {
    cropUnitX = 2;
    cropUnitY = 2 * ( 2 - sps.frame_mbs_only_flag );
  }

  if (VidWidth)
    *VidWidth = ((sps.pic_width_in_mbs_minus1 +1) * 16) - sps.frame_crop_right_offset * cropUnitX - sps.frame_crop_left_offset * cropUnitX;
  if (VidHeight)
    *VidHeight = ((2 - sps.frame_mbs_only_flag) * (sps.pic_height_in_map_units_minus1 +1) * 16) - (sps.frame_crop_bottom_offset * cropUnitY) - (sps.frame_crop_top_offset * cropUnitY);
  if (VidDAR && sps.vui.aspect_ratio_info_present_flag)
  {
    if (sps.vui.aspect_ratio_idc == 255)
      *VidDAR = ((double)*VidWidth / *VidHeight) * ((double)sps.vui.sar_width / sps.vui.sar_height);
    else
      *VidDAR = ((double)*VidWidth / *VidHeight) * ((double)sar_idc[sps.vui.aspect_ratio_idc].zaehler / sar_idc[sps.vui.aspect_ratio_idc].nenner);
  }
  if (VidFPS && sps.vui.timing_info_present_flag && sps.vui.fixed_frame_rate_flag)
  {
    *VidFPS = (double)sps.vui.time_scale / sps.vui.num_units_in_tick;  //  : (1 + SEI->nuit_field_based_flag);  // Frame-Rate or Field-Rate(!)
//    if (SEI->nuit_field_based_flag)
//    if (*VidHeight >= 1080)
      *VidFPS = *VidFPS / 2;
  }
  return sps.vui_parameters_present_flag;
}


/*void interpret_picture_timing_info( byte* payload, int size, ImageParameters *p_Img )
{
  sps_t *active_sps = p_Img->active_sps;

  int cpb_removal_delay, dpb_output_delay, picture_structure_present_flag, picture_structure;
  int clock_time_stamp_flag;
  int ct_type, nuit_field_based_flag, counting_type, full_timestamp_flag, discontinuity_flag, cnt_dropped_flag, nframes;
  int seconds_value, minutes_value, hours_value, seconds_flag, minutes_flag, hours_flag, time_offset;
  int NumClockTs = 0;
  int i;

  int cpb_removal_len = 24;
  int dpb_output_len  = 24;

  bool CpbDpbDelaysPresentFlag;

  Bitstream* buf;

  if (NULL==active_sps)
  {
    fprintf (stderr, "Warning: no active SPS, timing SEI cannot be parsed\n");
    return;
  }

  buf = malloc(sizeof(Bitstream));
  buf->bitstream_length = size;
  buf->streamBuffer = payload;
  buf->frame_bitoffset = 0;

  p_Dec->UsedBits = 0;


  #ifdef PRINT_PCITURE_TIMING_INFO
    printf("Picture timing SEI message\n");
  #endif

  // CpbDpbDelaysPresentFlag can also be set "by some means not specified in this Recommendation | International Standard"
  CpbDpbDelaysPresentFlag =  (bool) (active_sps->vui_parameters_present_flag
    && (   (active_sps->vui.nal_hrd_parameters_present_flag != 0)
    ||(active_sps->vui.vcl_hrd_parameters_present_flag != 0)));

  if (CpbDpbDelaysPresentFlag )
  {
    if (active_sps->vui_parameters_present_flag)
    {
      if (active_sps->vui.nal_hrd_parameters_present_flag)
      {
      cpb_removal_len = active_sps->vui.nal_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
      dpb_output_len  = active_sps->vui.nal_hrd_parameters.dpb_output_delay_length_minus1  + 1;
      }
      else if (active_sps->vui.vcl_hrd_parameters_present_flag)
      {
        cpb_removal_len = active_sps->vui.vcl_hrd_parameters.cpb_removal_delay_length_minus1 + 1;
        dpb_output_len  = active_sps->vui.vcl_hrd_parameters.dpb_output_delay_length_minus1  + 1;
      }
    }

    if ((active_sps->vui.nal_hrd_parameters_present_flag)||
      (active_sps->vui.vcl_hrd_parameters_present_flag))
    {
      cpb_removal_delay = u_v(cpb_removal_len, "SEI: cpb_removal_delay" , buf);
      dpb_output_delay  = u_v(dpb_output_len,  "SEI: dpb_output_delay"  , buf);
      #ifdef PRINT_PCITURE_TIMING_INFO
        printf("cpb_removal_delay = %d\n",cpb_removal_delay);
        printf("dpb_output_delay  = %d\n",dpb_output_delay);
      #endif
    }
  }

  if (!active_sps->vui_parameters_present_flag)
  {
    picture_structure_present_flag = 0;
  }
  else
  {
    picture_structure_present_flag  =  active_sps->vui.pic_struct_present_flag;
  }

  if (picture_structure_present_flag)
  {
    picture_structure = u_v(4, "SEI: pic_struct" , buf);
    #ifdef PRINT_PCITURE_TIMING_INFO
    printf("picture_structure = %d\n",picture_structure);
    #endif
    switch (picture_structure)
    {
      case 0:
      case 1:
      case 2:
        NumClockTs = 1;
      break;
      case 3:
      case 4:
      case 7:
        NumClockTs = 2;
      break;
      case 5:
      case 6:
      case 8:
        NumClockTs = 3;
      break;
      default:
        error("reserved picture_structure used (can't determine NumClockTs)", 500);
    }
    for (i=0; i<NumClockTs; i++)
    {
      clock_time_stamp_flag = u_1("SEI: clock_time_stamp_flag"  , buf);
      #ifdef PRINT_PCITURE_TIMING_INFO
        printf("clock_time_stamp_flag = %d\n",clock_time_stamp_flag);
      #endif
      if (clock_time_stamp_flag)
      {
        ct_type               = u_v(2, "SEI: ct_type"               , buf);
        nuit_field_based_flag = u_1(   "SEI: nuit_field_based_flag" , buf);
        counting_type         = u_v(5, "SEI: counting_type"         , buf);
        full_timestamp_flag   = u_1(   "SEI: full_timestamp_flag"   , buf);
        discontinuity_flag    = u_1(   "SEI: discontinuity_flag"    , buf);
        cnt_dropped_flag      = u_1(   "SEI: cnt_dropped_flag"      , buf);
        nframes               = u_v(8, "SEI: nframes"               , buf);

        #ifdef PRINT_PCITURE_TIMING_INFO
          printf("ct_type               = %d\n",ct_type);
          printf("nuit_field_based_flag = %d\n",nuit_field_based_flag);
          printf("full_timestamp_flag   = %d\n",full_timestamp_flag);
          printf("discontinuity_flag    = %d\n",discontinuity_flag);
          printf("cnt_dropped_flag      = %d\n",cnt_dropped_flag);
          printf("nframes               = %d\n",nframes);
        #endif
        if (full_timestamp_flag)
        {
          seconds_value         = u_v(6, "SEI: seconds_value"   , buf);
          minutes_value         = u_v(6, "SEI: minutes_value"   , buf);
          hours_value           = u_v(5, "SEI: hours_value"     , buf);
          #ifdef PRINT_PCITURE_TIMING_INFO
            printf("seconds_value = %d\n",seconds_value);
            printf("minutes_value = %d\n",minutes_value);
            printf("hours_value   = %d\n",hours_value);
          #endif
        }
        else
        {
          seconds_flag          = u_1(   "SEI: seconds_flag" , buf);
          #ifdef PRINT_PCITURE_TIMING_INFO
            printf("seconds_flag = %d\n",seconds_flag);
          #endif
          if (seconds_flag)
          {
            seconds_value         = u_v(6, "SEI: seconds_value"   , buf);
            minutes_flag          = u_1(   "SEI: minutes_flag" , buf);
            #ifdef PRINT_PCITURE_TIMING_INFO
              printf("seconds_value = %d\n",seconds_value);
              printf("minutes_flag  = %d\n",minutes_flag);
            #endif
            if(minutes_flag)
            {
              minutes_value         = u_v(6, "SEI: minutes_value"   , buf);
              hours_flag            = u_1(   "SEI: hours_flag" , buf);
              #ifdef PRINT_PCITURE_TIMING_INFO
                printf("minutes_value = %d\n",minutes_value);
                printf("hours_flag    = %d\n",hours_flag);
              #endif
              if(hours_flag)
              {
                hours_value           = u_v(5, "SEI: hours_value"     , buf);
                #ifdef PRINT_PCITURE_TIMING_INFO
                  printf("hours_value   = %d\n",hours_value);
                #endif
              }
            }
          }
        }
        {
          int time_offset_length;
          if (active_sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
            time_offset_length = active_sps->vui_seq_parameters.vcl_hrd_parameters.time_offset_length;
          else if (active_sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
            time_offset_length = active_sps->vui_seq_parameters.nal_hrd_parameters.time_offset_length;
          else
            time_offset_length = 24;
          if (time_offset_length)
            time_offset = i_v(time_offset_length, "SEI: time_offset"   , buf);
          else
            time_offset = 0;
          #ifdef PRINT_PCITURE_TIMING_INFO
            printf("time_offset   = %d\n",time_offset);
          #endif
        }
      }
    }
  }
  free (buf);
}

void InterpretSEIMessage(byte* msg, int size, ImageParameters *p_Img)
{
  int payload_type = 0;
  int payload_size = 0;
  int offset = 1;
  byte tmp_byte;

  do
  {
    // sei_message();
    payload_type = 0;
    tmp_byte = msg[offset++];
    while (tmp_byte == 0xFF)
    {
      payload_type += 255;
      tmp_byte = msg[offset++];
    }
    payload_type += tmp_byte;   // this is the last byte

    payload_size = 0;
    tmp_byte = msg[offset++];
    while (tmp_byte == 0xFF)
    {
      payload_size += 255;
      tmp_byte = msg[offset++];
    }
    payload_size += tmp_byte;   // this is the last byte

    switch ( payload_type )     // sei_payload( type, size );
    {
      case  SEI_PIC_TIMING:
        interpret_picture_timing_info( msg+offset, payload_size, p_Img );
        break;
    }
    offset += payload_size;

  } while( msg[offset] != 0x80 );    // more_rbsp_data()  msg[offset] != 0x80

  // ignore the trailing bits rbsp_trailing_bits();
  assert(msg[offset] == 0x80);      // this is the trailing bits
  assert( offset+1 == size );
} */
