#ifndef __H264H__
#define __H264H__

typedef struct
{
  byte                  zaehler;
  byte                  nenner;
} tFraction;

// HRD parameters (Appendix E.1.2)
typedef struct
{
  int cpb_cnt_minus1;
  int bit_rate_scale;
  int cpb_size_scale;
    int bit_rate_value_minus1[32]; // up to cpb_cnt_minus1, which is <= 31
    int cpb_size_value_minus1[32];
    bool cbr_flag[32];
  int initial_cpb_removal_delay_length_minus1;
  int cpb_removal_delay_length_minus1;
  int dpb_output_delay_length_minus1;
  int time_offset_length;
} hrd_t;

// VUI Parameters (Appendix E.1.1)
typedef struct
{
  bool aspect_ratio_info_present_flag;
    int aspect_ratio_idc;
    int sar_width;
    int sar_height;
  bool overscan_info_present_flag;
    bool overscan_appropriate_flag;
  bool video_signal_type_present_flag;
    int video_format;
    bool video_full_range_flag;
    bool colour_description_present_flag;
    int colour_primaries;
    int transfer_characteristics;
    int matrix_coefficients;
  bool chroma_loc_info_present_flag;
    int chroma_sample_loc_type_top_field;
    int chroma_sample_loc_type_bottom_field;
  bool timing_info_present_flag;
    int num_units_in_tick;
    int time_scale;
    bool fixed_frame_rate_flag;
/*  bool nal_hrd_parameters_present_flag;
    hrd_t nal_hrd_parameters;
  bool vcl_hrd_parameters_present_flag;
    hrd_t vcl_hrd_parameters;
    bool low_delay_hrd_flag;
  bool pic_struct_present_flag;
  bool bitstream_restriction_flag;
    bool motion_vectors_over_pic_boundaries_flag;
    int max_bytes_per_pic_denom;
    int max_bits_per_mb_denom;
    int log2_max_mv_length_horizontal;
    int log2_max_mv_length_vertical;
    int num_reorder_frames;
    int max_dec_frame_buffering; */
} vui_t;

// Sequence Parameter Set (7.3.2.1)
typedef struct
{
  int profile_idc;
  bool constraint_set0_flag;
  bool constraint_set1_flag;
  bool constraint_set2_flag;
  bool constraint_set3_flag;
  bool constraint_set4_flag;
  bool constraint_set5_flag;
  int reserved_zero_2bits;
  int level_idc;
  int seq_parameter_set_id;
  int chroma_format_idc;
  int residual_colour_transform_flag;
  int bit_depth_luma_minus8;
  int bit_depth_chroma_minus8;
  int qpprime_y_zero_transform_bypass_flag;
  bool seq_scaling_matrix_present_flag;
    int seq_scaling_list_present_flag[8];
//    int* ScalingList4x4[6];
//    bool UseDefaultScalingMatrix4x4Flag[6];
//    int* ScalingList8x8[2];
//    bool UseDefaultScalingMatrix8x8Flag[2];
  int log2_max_frame_num_minus4;
  int pic_order_cnt_type;
    int log2_max_pic_order_cnt_lsb_minus4;
    bool delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int num_ref_frames_in_pic_order_cnt_cycle;
//    int offset_for_ref_frame[256];
  int max_num_ref_frames;
  int gaps_in_frame_num_value_allowed_flag;
  int pic_width_in_mbs_minus1;
  int pic_height_in_map_units_minus1;
  bool frame_mbs_only_flag;
  bool mb_adaptive_frame_field_flag;
  bool direct_8x8_inference_flag;
  bool frame_cropping_flag;
    int frame_crop_left_offset;
    int frame_crop_right_offset;
    int frame_crop_top_offset;
    int frame_crop_bottom_offset;
  bool vui_parameters_present_flag;
  vui_t vui;
} sps_t;


int EBSPtoRBSP(byte *streamBuffer, int maxLen);
bool ParseSPS(const unsigned char *pStart, unsigned short nLen, int *const VidHeight, int *const VidWidth, double *const VidFPS, double *const VidDAR);

#endif
