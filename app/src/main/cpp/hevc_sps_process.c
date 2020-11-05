//
// Created by Administrator on 2019/3/28.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

typedef struct GetBitContext {
    const uint8_t *buffer, *buffer_end;
    int index;
    int size_in_bits;
} GetBitContext;

typedef struct SetBitContext {
   	uint8_t *buffer, *buffer_end;
    int index;
    int size_in_bits;
} SetBitContext;


typedef struct H265SPSParser{
    GetBitContext gb;
    SetBitContext sb;
    uint8_t *get_buffer;
    uint8_t *set_buffer;
    int get_size;
    int set_size;
} H265SPSParser;

enum {
    FPS_UNKNOW,
    FPS_15,
    FPS_24,
    FPS_25,
    FPS_2997,
    FPS_30,
    FPS_5994,
    FPS_60,
    FPS_MAX
};

typedef struct FRAMERATEINFO{
    int frame_rate;
    unsigned int num_units_in_tick;
    unsigned int time_scale;
    unsigned int fixed_frame_rate_flag;
}FRAMERATEINFO;


static FRAMERATEINFO framerate_table[] =
{
    {FPS_UNKNOW, 1000, 30000, 0},
    {FPS_15, 1000, 15000, 0},
    {FPS_24, 1000, 24000, 0},
    {FPS_25, 1000, 25000, 0},
    {FPS_2997, 1001, 30000, 0},
    {FPS_30, 1000, 30000, 0},
    {FPS_5994, 1001, 60000, 0},
    {FPS_60, 1000, 60000, 0},
    {FPS_MAX, 0, 0, 0},
};

static int swap(int value)
{
     return ((value & 0x000000FF) << 24) |
               ((value & 0x0000FF00) << 8) |
               ((value & 0x00FF0000) >> 8) |
               ((value & 0xFF000000) >> 24) ;
}

static unsigned int ReadBit(GetBitContext *gb)
{
    uint8_t nValue;
    int nIndex, nOffset;

    assert(gb->index <= gb->size_in_bits);

    nIndex = gb->index / 8;
    nOffset = gb->index % 8 + 1;
    nValue = *(gb->buffer + nIndex);

    gb->index++;

    return (nValue >> (8 - nOffset) ) & 0x01;
}

static unsigned int ReadBits(GetBitContext *gb, int n)
{
    int r = 0, i = 0;
    for( i = 0; i < n; i++){
        r |= ( ReadBit(gb) << (n - i - 1) );
    }
    return r;
}

static unsigned int ReadUE(GetBitContext *gb)
{
    int r = 0, i = 0;

    while( (ReadBit(gb) == 0) && (i < 32)){
        i++;
    }
    r = ReadBits(gb, i);
    r += (1 << i) - 1;

    return r;
}

static unsigned int ReadSE(GetBitContext *gb)
{
    int r = 0;

    r = ReadUE(gb);
    if (r & 0x01){
        r = (r+1)/2;
    }
    else{
        r = -(r/2);
    }

    return r;
}


static unsigned int WriteBit(SetBitContext *sb, unsigned char data)
{
    uint8_t nValue;
    int nIndex, nOffset;

    assert(sb->index <= sb->size_in_bits);

    nIndex = sb->index / 8;
    nOffset = sb->index % 8;

    nValue = *(sb->buffer + nIndex);
	nValue &= (~(0x1 << (7 - nOffset)));
	nValue |= (data << (7 - nOffset));

	sb->buffer[nIndex] = nValue;

    sb->index++;

	return 1;
}

static unsigned int WriteBits(SetBitContext *sb, unsigned char* data, int n)
{
    int r = 1, i = 0;
    for( i = 0; i < n; i++){
		r |= WriteBit(sb, ((data[i/8] >> (7-(i % 8))) & 0x01));
    }
    return r;
}

static unsigned int WriteBits2(SetBitContext *sb, unsigned char* data, int start, int n)
{
    int r = 1, i = 0;

	if(start){
    	for( i = 0; i < start; i++){
			r |= WriteBit(sb, ((data[0] >> (start-i-1)) & 0x01));
	    }
		r |= WriteBits(sb, data+1, n-start);
	}else{
		r |= WriteBits(sb, data, n-start);
	}

    return r;
}

static unsigned int WriteBitsEnd(SetBitContext *sb)
{
	int r = 1, i = 0;
	if(sb->index % 8){
		char data = 0x00;
		r = WriteBits(sb, &data, 8 - (sb->index % 8));
	}

    return r;
}

unsigned int dump_hevc_sps_information(GetBitContext *gb)
{
	//seq_parameter_set_rbsp()
	int sps_video_parameter_set_id = 0;
	int sps_max_sub_layers_minus1 = 0;
	int sps_temporal_id_nesting_flag = 0;


	sps_video_parameter_set_id = ReadBits(gb, 4);
	sps_max_sub_layers_minus1 = ReadBits(gb, 3);
	sps_temporal_id_nesting_flag = ReadBit(gb);

	printf("sps_video_parameter_set_id: %d\n", sps_video_parameter_set_id);
	printf("sps_max_sub_layers_minus1: %d\n", sps_max_sub_layers_minus1);
	printf("sps_temporal_id_nesting_flag: %d\n", sps_temporal_id_nesting_flag);

	//profile_tier_level(sps_max_sub_layers_minus1)
	{

		int general_profile_space = 0;
		int general_tier_flag = 0;
		int general_profile_idc = 0;
		int general_profile_compatibility_flag[32] = {0};
		int general_progressive_source_flag = 0;
		int general_interlaced_source_flag = 0;
		int general_non_packed_constraint_flag = 0;
		int general_frame_only_constraint_flag = 0;
		int general_reserved_zero_44bits = 0;
		int general_level_idc = 0;

		int sub_layer_profile_present_flag[32] = {0};
		int sub_layer_level_present_flag[32] = {0};
		int reserved_zero_2bits[32] = {0};
		int sub_layer_profile_space[32] = {0};
		int sub_layer_tier_flag[32] = {0};
		int sub_layer_profile_idc[32] = {0};
		int sub_layer_profile_compatibility_flag[32][32] = {0};
		int sub_layer_progressive_source_flag[32] = {0};
		int sub_layer_interlaced_source_flag[32] = {0};
		int sub_layer_non_packed_constraint_flag[32] = {0};
		int sub_layer_frame_only_constraint_flag[32] = {0};
		int sub_layer_reserved_zero_44bits[32] = {0};
		int sub_layer_level_idc[32] = {0};

		int i = 0;
		int j = 0;

		general_profile_space = ReadBits(gb, 2);
		general_tier_flag = ReadBit(gb);
		general_profile_idc = ReadBits(gb, 5);
		for(j = 0; j < 32; j++)
			general_profile_compatibility_flag[j] = ReadBit(gb);
		general_progressive_source_flag = ReadBit(gb);
		general_interlaced_source_flag = ReadBit(gb);
		general_non_packed_constraint_flag = ReadBit(gb);
		general_frame_only_constraint_flag = ReadBit(gb);
		general_reserved_zero_44bits = ReadBits(gb, 44);
		general_level_idc = ReadBits(gb, 8);

		for(i = 0; i < sps_max_sub_layers_minus1; i++) {
			sub_layer_profile_present_flag[i] = ReadBit(gb);
			sub_layer_level_present_flag[i] = ReadBit(gb);
		}
		if(sps_max_sub_layers_minus1 > 0)
			for( i = sps_max_sub_layers_minus1; i < 8; i++)
				reserved_zero_2bits[i] = ReadBits(gb, 2);

		for(i = 0; i < sps_max_sub_layers_minus1; i++) {
			if(sub_layer_profile_present_flag[i]) {
				sub_layer_profile_space[i] = ReadBits(gb, 2);
				sub_layer_tier_flag[i] = ReadBit(gb);
				sub_layer_profile_idc[i] = ReadBits(gb, 5);

				for(j=0; j<32; j++)
					sub_layer_profile_compatibility_flag[i][j] = ReadBit(gb);
				sub_layer_progressive_source_flag[i] = ReadBit(gb);
				sub_layer_interlaced_source_flag[i] = ReadBit(gb);
				sub_layer_non_packed_constraint_flag[i] = ReadBit(gb);
				sub_layer_frame_only_constraint_flag[i] = ReadBit(gb);
				sub_layer_reserved_zero_44bits[i] = ReadBits(gb, 44);

			}
			if(sub_layer_level_present_flag[i])
				sub_layer_level_idc[i] = ReadBits(gb, 8);
		}

		printf("general_profile_space: %d\n", general_profile_space);
		printf("general_tier_flag: %d\n", general_tier_flag);
		printf("general_profile_idc: %d\n", general_profile_idc);
		for(i=0;i<32;i++)
			printf("general_profile_compatibility_flag[%d]: %d\n", i, general_profile_compatibility_flag[i]);
		printf("general_progressive_source_flag: %d\n", general_progressive_source_flag);
		printf("general_interlaced_source_flag: %d\n", general_interlaced_source_flag);
		printf("general_non_packed_constraint_flag: %d\n", general_non_packed_constraint_flag);
		printf("general_frame_only_constraint_flag: %d\n", general_frame_only_constraint_flag);
		printf("general_reserved_zero_44bits: %d\n", general_reserved_zero_44bits);
		printf("general_level_idc: %d\n", general_level_idc);

	}

	int separate_colour_plane_flag = 0;


	int sps_seq_parameter_set_id = ReadUE(gb);
	int chroma_format_idc = ReadUE(gb);

	printf("sps_seq_parameter_set_id: %d\n", sps_seq_parameter_set_id);
	printf("chroma_format_idc: %d\n", chroma_format_idc);


	if(chroma_format_idc == 3)
		separate_colour_plane_flag = ReadBit(gb);

	int pic_width_in_luma_samples = ReadUE(gb);
	int pic_height_in_luma_samples = ReadUE(gb);
	int conformance_window_flag = ReadBit(gb);

	printf("pic_width_in_luma_samples: %d\n", pic_width_in_luma_samples);
	printf("pic_height_in_luma_samples: %d\n", pic_height_in_luma_samples);
	printf("conformance_window_flag: %d\n", conformance_window_flag);

	int conf_win_left_offset = 0;
	int conf_win_right_offset = 0;
	int conf_win_top_offset = 0;
	int conf_win_bottom_offset = 0;

	if(conformance_window_flag){
		conf_win_left_offset = ReadUE(gb);
		conf_win_right_offset = ReadUE(gb);
		conf_win_top_offset = ReadUE(gb);
		conf_win_bottom_offset = ReadUE(gb);
	}

	int bit_depth_luma_minus8 = ReadUE(gb);
	int bit_depth_chroma_minus8 = ReadUE(gb);
	int log2_max_pic_order_cnt_lsb_minus4 = ReadUE(gb);
	int sps_sub_layer_ordering_info_present_flag = ReadBit(gb);

	printf("bit_depth_luma_minus8: %d\n", bit_depth_luma_minus8);
	printf("bit_depth_chroma_minus8: %d\n", bit_depth_chroma_minus8);
	printf("log2_max_pic_order_cnt_lsb_minus4: %d\n", log2_max_pic_order_cnt_lsb_minus4);
	printf("sps_sub_layer_ordering_info_present_flag: %d\n", sps_sub_layer_ordering_info_present_flag);


	int sps_max_dec_pic_buffering_minus1[32] = {0};
	int sps_max_num_reorder_pics[32] = {0};
	int sps_max_latency_increase_plus1[32] = {0};

	int i = 0;
	int j = 0;
	for(i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1);
		i <= sps_max_sub_layers_minus1; i++) {

		sps_max_dec_pic_buffering_minus1[i] = ReadUE(gb);
		sps_max_num_reorder_pics[i] = ReadUE(gb);
		sps_max_latency_increase_plus1[i] = ReadUE(gb);

	}

	int log2_min_luma_coding_block_size_minus3 = ReadUE(gb);
	int log2_diff_max_min_luma_coding_block_size = ReadUE(gb);
	int log2_min_transform_block_size_minus2 = ReadUE(gb);
	int log2_diff_max_min_transform_block_size = ReadUE(gb);
	int max_transform_hierarchy_depth_inter = ReadUE(gb);
	int max_transform_hierarchy_depth_intra = ReadUE(gb);
	int scaling_list_enabled_flag = ReadBit(gb);
	printf("scaling_list_enabled_flag: %d\n", scaling_list_enabled_flag);


	if(scaling_list_enabled_flag) {
		int scaling_list_pred_mode_flag[32][32] = {0};
		int scaling_list_pred_matrix_id_delta[32][32] = {0};
		int scaling_list_dc_coef_minus8[32][32] = {0};
		int scaling_list_delta_coef = 0;

		int sps_scaling_list_data_present_flag = ReadBit(gb);
		printf("sps_scaling_list_data_present_flag: %d\n", sps_scaling_list_data_present_flag);
		if(sps_scaling_list_data_present_flag) {
			//scaling_list_data()
			{
				int sizeId = 0;
				int matrixId = 0;

				for(sizeId = 0; sizeId < 4; sizeId++) {
					for(matrixId = 0; matrixId < ((sizeId==3)?2:6); matrixId++)	{
						scaling_list_pred_mode_flag[sizeId][matrixId] = ReadBit(gb);
						if(!scaling_list_pred_mode_flag[sizeId][matrixId])
							scaling_list_pred_matrix_id_delta[sizeId][matrixId] = ReadUE(gb);
						else {
							int nextCoef = 8;
							//int coefNum = Min( 64, (1 << (4+(sizeId << 1))));
							int coefNum = 64 > (1 << (4+(sizeId << 1))) ? (1 << (4+(sizeId << 1))) : 64;
							int ScalingList[32][32][32] = {0};
							if(sizeId > 1) {
								scaling_list_dc_coef_minus8[sizeId-2][matrixId] = ReadSE(gb);
								nextCoef = scaling_list_dc_coef_minus8[sizeId-2][matrixId] + 8;
							}
							for(i=0; i<coefNum; i++) {
								scaling_list_delta_coef = ReadSE(gb);
								nextCoef = (nextCoef + scaling_list_delta_coef+256) % 256;
								ScalingList[sizeId][matrixId][i] = nextCoef;
							}
						}
					}
				}
			}
		}
	}

	int amp_enabled_flag = ReadBit(gb);
	int sample_adaptive_offset_enabled_flag = ReadBit(gb);
	int pcm_enabled_flag = ReadBit(gb);
	printf("pcm_enabled_flag: %d\n", pcm_enabled_flag);

	if(pcm_enabled_flag) {
		int pcm_sample_bit_depth_luma_minus1 = ReadBits(gb, 4);
		int pcm_sample_bot_depth_chroma_minus1 = ReadBits(gb, 4);
		int log2_min_pcm_luma_coding_block_size_minus3 = ReadUE(gb);
		int log2_diff_max_min_pcm_luma_coding_block_size = ReadUE(gb);
		int pcm_loop_filter_disabled_flag = ReadBit(gb);
	}

	int num_short_term_ref_pic_sets = ReadUE(gb);
	printf("num_short_term_ref_pic_sets: %d\n", num_short_term_ref_pic_sets);
	printf("gb->index: %d\n", gb->index);
	int NumDeltaPocs[32] = {0};
	int stRpsIdx = 0;
	for(stRpsIdx = 0; stRpsIdx < num_short_term_ref_pic_sets; stRpsIdx++) {
		//short_term_ref_pic_set(i)
		{
			int inter_ref_pic_set_prediction_flag = 0;
			int used_by_curr_pic_flag[32] = {0};
			int use_delta_flag[32] = {0};
			int delta_idx_minus1 = 0;
			int i, j;

			//if(i != 0)
			if(stRpsIdx != 0)
				inter_ref_pic_set_prediction_flag = ReadBit(gb);
			else
				inter_ref_pic_set_prediction_flag = 0;

			//printf("inter_ref_pic_set_prediction_flag: %d, i: %d\n", inter_ref_pic_set_prediction_flag, stRpsIdx);

			if(inter_ref_pic_set_prediction_flag) {
				if(stRpsIdx == num_short_term_ref_pic_sets)
					delta_idx_minus1 = ReadUE(gb);
				int delta_rps_sign = ReadBit(gb);
				int abs_delta_rps_minus1 = ReadUE(gb);
				//for(j =0; j<=NumDeltaPocs[i]; j++) {
				for(j =0; j<=0; j++) {
					used_by_curr_pic_flag[j] = ReadBit(gb);
					if(!used_by_curr_pic_flag[j])
						use_delta_flag[j] = ReadBit(gb);
				}

			}else{
				int num_negative_pics = ReadUE(gb);
				int num_positive_pics = ReadUE(gb);

				int delta_poc_s0_minus1[32] = {0};
				int used_by_curr_pic_s0_flag[32] = {0};
				int delta_poc_s1_minus1[32] = {0};
				int used_by_curr_pic_s1_flag[32] = {0};

				//printf("num_negative_pics: %d\n", num_negative_pics);
				//printf("num_positive_pics: %d\n", num_positive_pics);

				for(i=0; i< num_negative_pics; i++) {
					delta_poc_s0_minus1[i] = ReadUE(gb);
					used_by_curr_pic_s0_flag[i] = ReadBit(gb);
				}
				for(i=0; i< num_positive_pics; i++) {
					delta_poc_s1_minus1[i] = ReadUE(gb);
					used_by_curr_pic_s1_flag[i] = ReadBit(gb);
				}
			}
		}
	}

	int long_term_ref_pics_present_flag = ReadBit(gb);
	int num_long_term_ref_pics_sps = 0;
	int lt_ref_pic_poc_lsb_sps[32] = {0};
	int used_by_curr_pic_lt_sps_flag[32] = {0};
	printf("long_term_ref_pics_present_flag: %d\n", long_term_ref_pics_present_flag);

	if(long_term_ref_pics_present_flag) {
		num_long_term_ref_pics_sps = ReadUE(gb);
		for(i = 0; i < num_long_term_ref_pics_sps; i++) {
			lt_ref_pic_poc_lsb_sps[i] = ReadUE(gb);
			used_by_curr_pic_lt_sps_flag[i] = ReadBit(gb);
		}
	}

	int sps_temporal_mvp_enabled_flag = ReadBit(gb);
	int strong_intra_smoothing_enabled_flag = ReadBit(gb);
	int vui_parameters_present_flag = ReadBit(gb);

	printf("vui_parameters_present_flag: %d\n", vui_parameters_present_flag);
	if(vui_parameters_present_flag) {
		//vui_parameters()
		{
    		int aspect_ratio_info_present_flag = ReadBit(gb);
	    	if(aspect_ratio_info_present_flag)
	    	{
	    		int aspect_ratio_idc = ReadBits(gb, 8);

	    		if(aspect_ratio_idc == 255)
	    		{
	    			int sar_width = ReadBits(gb, 16);
	    			int sar_height = ReadBits(gb, 16);
	    		}

	    	}

	    	int overscan_info_present_flag = ReadBit(gb);
	    	if(overscan_info_present_flag)
	    	{
	    		int overscan_appropriate_flag = ReadBit(gb);
	    	}

	    	int video_signal_type_present_flag = ReadBit(gb);
	    	if(video_signal_type_present_flag)
	    	{
	    		int video_format = ReadBits(gb, 3);
	    		int video_full_range_flag = ReadBit(gb);
	    		int colour_description_present_flag = ReadBit(gb);
	    		if(colour_description_present_flag)
	    		{
	    			int colour_primaries = ReadBits(gb, 8);
	    			int transfer_characteristics = ReadBits(gb, 8);
    				int matrix_coeffs = ReadBits(gb, 8);
	    		}
	    	}

	    	int chroma_loc_info_present_flag = ReadBit(gb);
	    	if(chroma_loc_info_present_flag)
	    	{
	    		int chroma_sample_loc_type_top_field = ReadUE(gb);
	    		int chroma_sample_loc_type_bottom_field = ReadUE(gb);
	    	}

			int neutral_chroma_indocation_flag = ReadBit(gb);
			int filed_seq_flag = ReadBit(gb);
			int frame_filed_info_present_flag = ReadBit(gb);
			int default_display_window_flag = ReadBit(gb);
			if(default_display_window_flag) {
				int def_disp_win_left_offset = ReadUE(gb);
				int def_disp_win_right_offset = ReadUE(gb);
				int def_disp_win_top_offset = ReadUE(gb);
				int def_disp_win_bottom_offset = ReadUE(gb);
			}

	    	int vui_timing_info_present_flag = ReadBit(gb);
			//printf("m_nCurrentBit:%d\n", m_nCurrentBit);
	    	if(vui_timing_info_present_flag) {
	    		int vui_num_units_in_tick = ReadBits(gb, 32);
	    		int vui_time_scale = ReadBits(gb, 32);
				int vui_poc_proportional_to_timing_flag = ReadBit(gb);
				int vui_num_ticks_poc_diff_one_minus1 = 0;

				if(vui_poc_proportional_to_timing_flag)
					vui_num_ticks_poc_diff_one_minus1 = ReadUE(gb);
				int vui_hdr_parameters_present_flag = ReadBit(gb);
				if(vui_hdr_parameters_present_flag)
					//hrd_parameters(1, sps_max_sub_layers_minus1)
					{

					}
	    	}
			int bitstream_restriction_flag = ReadBit(gb);
			if(bitstream_restriction_flag) {
				int tiles_fixed_structure_flag = ReadBit(gb);
				int motion_vectors_over_pic_boundaries_flag = ReadBit(gb);
				int restricted_ref_pic_lists_flag = ReadBit(gb);
				int min_spatial_segmentation_id = ReadUE(gb);
				int max_bytes_per_pic_denom = ReadUE(gb);
				int max_bits_per_min_cu_denom = ReadUE(gb);
				int log2_max_mv_length_horizontal = ReadUE(gb);
				int log2_max_mv_length_vertical = ReadUE(gb);
			}

   /*
			printf("sapect_ratio_info_present_flag:%d\n", sapect_ratio_info_present_flag);
			printf("sapect_ratio_idc:%d\n", sapect_ratio_idc);
			printf("sar_width:%d\n", sar_width);
			printf("sar_height:%d\n", sar_height);
			printf("overscan_info_present_flag:%d\n", overscan_info_present_flag);
			printf("overscan_appropriate_flag:%d\n", overscan_appropriate_flag);
			printf("video_signal_type_present_flag:%d\n", video_signal_type_present_flag);
			printf("video_format:%d\n", video_format);
			printf("video_full_range_flag:%d\n", video_full_range_flag);
			printf("colour_description_present_flag:%d\n", colour_description_present_flag);
			printf("colour_primaries:%d\n", colour_primaries);
			printf("transfer_characteristics:%d\n", transfer_characteristics);
			printf("matrix_coefficients:%d\n", matrix_coefficients);
			printf("chroma_loc_info_present_flag:%d\n", chroma_loc_info_present_flag);


			printf("chroma_sample_loc_type_top_field:%d\n", chroma_sample_loc_type_top_field);
			printf("chroma_sample_loc_type_bottom_field:%d\n", chroma_sample_loc_type_bottom_field);
			printf("timing_info_present_flag:%d\n", timing_info_present_flag);
			printf("num_units_in_tick:%d\n", num_units_in_tick);
			printf("time_scale:%d\n", time_scale);
			printf("fixed_frame_rate_flag:%d\n", fixed_frame_rate_flag);
			printf("nal_hrd_parameters_present_flag:%d\n", nal_hrd_parameters_present_flag);
			printf("vcl_hrd_parameters_present_flag:%d\n", vcl_hrd_parameters_present_flag);
			printf("low_delay_hrd_flag:%d\n", low_delay_hrd_flag);
			printf("pic_struct_present_flag:%d\n", pic_struct_present_flag);
			printf("bitstream_restriction_flag:%d\n", bitstream_restriction_flag);
			printf("motion_vectors_over_pic_boundaries_flag:%d\n", motion_vectors_over_pic_boundaries_flag);
			printf("max_bytes_per_pic_denom:%d\n", max_bytes_per_pic_denom);
			printf("max_bits_per_mb_denom:%d\n", max_bits_per_mb_denom);

			printf("log2_max_mv_length_horizontal:%d\n", log2_max_mv_length_horizontal);
			printf("log2_max_mv_length_vertical:%d\n", log2_max_mv_length_vertical);
			printf("num_reorder_frames:%d\n", num_reorder_frames);
			printf("max_dec_frame_buffering:%d\n", max_dec_frame_buffering);
	*/
		}
	}
/*
	int sps_extension_flag = ReadBit(gb);
	if(sps_extension_flag) {
		while( more_rbsp_data() ) {
			sps_extension_data_flag = ReadBit(gb);
		}
	}
*/
	//rbsp_trailing_bits()
//	{
//		rbsp_stop_one_bit/*equal to 1*/  f(1)
//		while(!byte_aligned())
//			rbsp_alignment_zero_bit/*equal to 0*/ f(1)
//	}

}


static int hevc_sps_find_vui_parameters_present_flag(GetBitContext *gb)
{
	//seq_parameter_set_rbsp()
	int sps_video_parameter_set_id = 0;
	int sps_max_sub_layers_minus1 = 0;
	int sps_temporal_id_nesting_flag = 0;


	sps_video_parameter_set_id = ReadBits(gb, 4);
	sps_max_sub_layers_minus1 = ReadBits(gb, 3);
	sps_temporal_id_nesting_flag = ReadBit(gb);

	printf("sps_video_parameter_set_id: %d\n", sps_video_parameter_set_id);
	printf("sps_max_sub_layers_minus1: %d\n", sps_max_sub_layers_minus1);
	printf("sps_temporal_id_nesting_flag: %d\n", sps_temporal_id_nesting_flag);

	//profile_tier_level(sps_max_sub_layers_minus1)
	{

		int general_profile_space = 0;
		int general_tier_flag = 0;
		int general_profile_idc = 0;
		int general_profile_compatibility_flag[32] = {0};
		int general_progressive_source_flag = 0;
		int general_interlaced_source_flag = 0;
		int general_non_packed_constraint_flag = 0;
		int general_frame_only_constraint_flag = 0;
		int general_reserved_zero_44bits = 0;
		int general_level_idc = 0;

		int sub_layer_profile_present_flag[32] = {0};
		int sub_layer_level_present_flag[32] = {0};
		int reserved_zero_2bits[32] = {0};
		int sub_layer_profile_space[32] = {0};
		int sub_layer_tier_flag[32] = {0};
		int sub_layer_profile_idc[32] = {0};
		int sub_layer_profile_compatibility_flag[32][32] = {0};
		int sub_layer_progressive_source_flag[32] = {0};
		int sub_layer_interlaced_source_flag[32] = {0};
		int sub_layer_non_packed_constraint_flag[32] = {0};
		int sub_layer_frame_only_constraint_flag[32] = {0};
		int sub_layer_reserved_zero_44bits[32] = {0};
		int sub_layer_level_idc[32] = {0};

		int i = 0;
		int j = 0;

		general_profile_space = ReadBits(gb, 2);
		general_tier_flag = ReadBit(gb);
		general_profile_idc = ReadBits(gb, 5);
		for(j = 0; j < 32; j++)
			general_profile_compatibility_flag[j] = ReadBit(gb);
		general_progressive_source_flag = ReadBit(gb);
		general_interlaced_source_flag = ReadBit(gb);
		general_non_packed_constraint_flag = ReadBit(gb);
		general_frame_only_constraint_flag = ReadBit(gb);
		general_reserved_zero_44bits = ReadBits(gb, 44);
		general_level_idc = ReadBits(gb, 8);

		for(i = 0; i < sps_max_sub_layers_minus1; i++) {
			sub_layer_profile_present_flag[i] = ReadBit(gb);
			sub_layer_level_present_flag[i] = ReadBit(gb);
		}
		if(sps_max_sub_layers_minus1 > 0)
			for( i = sps_max_sub_layers_minus1; i < 8; i++)
				reserved_zero_2bits[i] = ReadBits(gb, 2);

		for(i = 0; i < sps_max_sub_layers_minus1; i++) {
			if(sub_layer_profile_present_flag[i]) {
				sub_layer_profile_space[i] = ReadBits(gb, 2);
				sub_layer_tier_flag[i] = ReadBit(gb);
				sub_layer_profile_idc[i] = ReadBits(gb, 5);

				for(j=0; j<32; j++)
					sub_layer_profile_compatibility_flag[i][j] = ReadBit(gb);
				sub_layer_progressive_source_flag[i] = ReadBit(gb);
				sub_layer_interlaced_source_flag[i] = ReadBit(gb);
				sub_layer_non_packed_constraint_flag[i] = ReadBit(gb);
				sub_layer_frame_only_constraint_flag[i] = ReadBit(gb);
				sub_layer_reserved_zero_44bits[i] = ReadBits(gb, 44);

			}
			if(sub_layer_level_present_flag[i])
				sub_layer_level_idc[i] = ReadBits(gb, 8);
		}

		printf("general_profile_space: %d\n", general_profile_space);
		printf("general_tier_flag: %d\n", general_tier_flag);
		printf("general_profile_idc: %d\n", general_profile_idc);
		for(i=0;i<32;i++)
			printf("general_profile_compatibility_flag[%d]: %d\n", i, general_profile_compatibility_flag[i]);
		printf("general_progressive_source_flag: %d\n", general_progressive_source_flag);
		printf("general_interlaced_source_flag: %d\n", general_interlaced_source_flag);
		printf("general_non_packed_constraint_flag: %d\n", general_non_packed_constraint_flag);
		printf("general_frame_only_constraint_flag: %d\n", general_frame_only_constraint_flag);
		printf("general_reserved_zero_44bits: %d\n", general_reserved_zero_44bits);
		printf("general_level_idc: %d\n", general_level_idc);

	}

	int separate_colour_plane_flag = 0;


	int sps_seq_parameter_set_id = ReadUE(gb);
	int chroma_format_idc = ReadUE(gb);

	printf("sps_seq_parameter_set_id: %d\n", sps_seq_parameter_set_id);
	printf("chroma_format_idc: %d\n", chroma_format_idc);


	if(chroma_format_idc == 3)
		separate_colour_plane_flag = ReadBit(gb);

	int pic_width_in_luma_samples = ReadUE(gb);
	int pic_height_in_luma_samples = ReadUE(gb);
	int conformance_window_flag = ReadBit(gb);

	printf("pic_width_in_luma_samples: %d\n", pic_width_in_luma_samples);
	printf("pic_height_in_luma_samples: %d\n", pic_height_in_luma_samples);
	printf("conformance_window_flag: %d\n", conformance_window_flag);

	int conf_win_left_offset = 0;
	int conf_win_right_offset = 0;
	int conf_win_top_offset = 0;
	int conf_win_bottom_offset = 0;

	if(conformance_window_flag){
		conf_win_left_offset = ReadUE(gb);
		conf_win_right_offset = ReadUE(gb);
		conf_win_top_offset = ReadUE(gb);
		conf_win_bottom_offset = ReadUE(gb);
	}

	int bit_depth_luma_minus8 = ReadUE(gb);
	int bit_depth_chroma_minus8 = ReadUE(gb);
	int log2_max_pic_order_cnt_lsb_minus4 = ReadUE(gb);
	int sps_sub_layer_ordering_info_present_flag = ReadBit(gb);

	printf("bit_depth_luma_minus8: %d\n", bit_depth_luma_minus8);
	printf("bit_depth_chroma_minus8: %d\n", bit_depth_chroma_minus8);
	printf("log2_max_pic_order_cnt_lsb_minus4: %d\n", log2_max_pic_order_cnt_lsb_minus4);
	printf("sps_sub_layer_ordering_info_present_flag: %d\n", sps_sub_layer_ordering_info_present_flag);


	int sps_max_dec_pic_buffering_minus1[32] = {0};
	int sps_max_num_reorder_pics[32] = {0};
	int sps_max_latency_increase_plus1[32] = {0};

	int i = 0;
	int j = 0;
	for(i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1);
		i <= sps_max_sub_layers_minus1; i++) {

		sps_max_dec_pic_buffering_minus1[i] = ReadUE(gb);
		sps_max_num_reorder_pics[i] = ReadUE(gb);
		sps_max_latency_increase_plus1[i] = ReadUE(gb);

	}

	int log2_min_luma_coding_block_size_minus3 = ReadUE(gb);
	int log2_diff_max_min_luma_coding_block_size = ReadUE(gb);
	int log2_min_transform_block_size_minus2 = ReadUE(gb);
	int log2_diff_max_min_transform_block_size = ReadUE(gb);
	int max_transform_hierarchy_depth_inter = ReadUE(gb);
	int max_transform_hierarchy_depth_intra = ReadUE(gb);
	int scaling_list_enabled_flag = ReadBit(gb);
	printf("scaling_list_enabled_flag: %d\n", scaling_list_enabled_flag);


	if(scaling_list_enabled_flag) {
		int scaling_list_pred_mode_flag[32][32] = {0};
		int scaling_list_pred_matrix_id_delta[32][32] = {0};
		int scaling_list_dc_coef_minus8[32][32] = {0};
		int scaling_list_delta_coef = 0;

		int sps_scaling_list_data_present_flag = ReadBit(gb);
		printf("sps_scaling_list_data_present_flag: %d\n", sps_scaling_list_data_present_flag);
		if(sps_scaling_list_data_present_flag) {
			//scaling_list_data()
			{
				int sizeId = 0;
				int matrixId = 0;

				for(sizeId = 0; sizeId < 4; sizeId++) {
					for(matrixId = 0; matrixId < ((sizeId==3)?2:6); matrixId++)	{
						scaling_list_pred_mode_flag[sizeId][matrixId] = ReadBit(gb);
						if(!scaling_list_pred_mode_flag[sizeId][matrixId])
							scaling_list_pred_matrix_id_delta[sizeId][matrixId] = ReadUE(gb);
						else {
							int nextCoef = 8;
							//int coefNum = Min( 64, (1 << (4+(sizeId << 1))));
							int coefNum = 64 > (1 << (4+(sizeId << 1))) ? (1 << (4+(sizeId << 1))) : 64;
							int ScalingList[32][32][32] = {0};
							if(sizeId > 1) {
								scaling_list_dc_coef_minus8[sizeId-2][matrixId] = ReadSE(gb);
								nextCoef = scaling_list_dc_coef_minus8[sizeId-2][matrixId] + 8;
							}
							for(i=0; i<coefNum; i++) {
								scaling_list_delta_coef = ReadSE(gb);
								nextCoef = (nextCoef + scaling_list_delta_coef+256) % 256;
								ScalingList[sizeId][matrixId][i] = nextCoef;
							}
						}
					}
				}
			}
		}
	}

	int amp_enabled_flag = ReadBit(gb);
	int sample_adaptive_offset_enabled_flag = ReadBit(gb);
	int pcm_enabled_flag = ReadBit(gb);
	printf("pcm_enabled_flag: %d\n", pcm_enabled_flag);

	if(pcm_enabled_flag) {
		int pcm_sample_bit_depth_luma_minus1 = ReadBits(gb, 4);
		int pcm_sample_bot_depth_chroma_minus1 = ReadBits(gb, 4);
		int log2_min_pcm_luma_coding_block_size_minus3 = ReadUE(gb);
		int log2_diff_max_min_pcm_luma_coding_block_size = ReadUE(gb);
		int pcm_loop_filter_disabled_flag = ReadBit(gb);
	}

	int num_short_term_ref_pic_sets = ReadUE(gb);
	printf("num_short_term_ref_pic_sets: %d\n", num_short_term_ref_pic_sets);
	printf("gb->index: %d\n", gb->index);
	int NumDeltaPocs[32] = {0};
	int stRpsIdx = 0;
	for(stRpsIdx = 0; stRpsIdx < num_short_term_ref_pic_sets; stRpsIdx++) {
		//short_term_ref_pic_set(i)
		{
			int inter_ref_pic_set_prediction_flag = 0;
			int used_by_curr_pic_flag[32] = {0};
			int use_delta_flag[32] = {0};
			int delta_idx_minus1 = 0;
			int i, j;

			//if(i != 0)
			if(stRpsIdx != 0)
				inter_ref_pic_set_prediction_flag = ReadBit(gb);
			else
				inter_ref_pic_set_prediction_flag = 0;

			//printf("inter_ref_pic_set_prediction_flag: %d, i: %d\n", inter_ref_pic_set_prediction_flag, stRpsIdx);

			if(inter_ref_pic_set_prediction_flag) {
				if(stRpsIdx == num_short_term_ref_pic_sets)
					delta_idx_minus1 = ReadUE(gb);
				int delta_rps_sign = ReadBit(gb);
				int abs_delta_rps_minus1 = ReadUE(gb);
				//for(j =0; j<=NumDeltaPocs[i]; j++) {
				for(j =0; j<=0; j++) {
					used_by_curr_pic_flag[j] = ReadBit(gb);
					if(!used_by_curr_pic_flag[j])
						use_delta_flag[j] = ReadBit(gb);
				}

			}else{
				int num_negative_pics = ReadUE(gb);
				int num_positive_pics = ReadUE(gb);

				int delta_poc_s0_minus1[32] = {0};
				int used_by_curr_pic_s0_flag[32] = {0};
				int delta_poc_s1_minus1[32] = {0};
				int used_by_curr_pic_s1_flag[32] = {0};

				//printf("num_negative_pics: %d\n", num_negative_pics);
				//printf("num_positive_pics: %d\n", num_positive_pics);

				for(i=0; i< num_negative_pics; i++) {
					delta_poc_s0_minus1[i] = ReadUE(gb);
					used_by_curr_pic_s0_flag[i] = ReadBit(gb);
				}
				for(i=0; i< num_positive_pics; i++) {
					delta_poc_s1_minus1[i] = ReadUE(gb);
					used_by_curr_pic_s1_flag[i] = ReadBit(gb);
				}
			}
		}
	}

	int long_term_ref_pics_present_flag = ReadBit(gb);
	int num_long_term_ref_pics_sps = 0;
	int lt_ref_pic_poc_lsb_sps[32] = {0};
	int used_by_curr_pic_lt_sps_flag[32] = {0};
	printf("long_term_ref_pics_present_flag: %d\n", long_term_ref_pics_present_flag);

	if(long_term_ref_pics_present_flag) {
		num_long_term_ref_pics_sps = ReadUE(gb);
		for(i = 0; i < num_long_term_ref_pics_sps; i++) {
			lt_ref_pic_poc_lsb_sps[i] = ReadUE(gb);
			used_by_curr_pic_lt_sps_flag[i] = ReadBit(gb);
		}
	}

	int sps_temporal_mvp_enabled_flag = ReadBit(gb);
	int strong_intra_smoothing_enabled_flag = ReadBit(gb);
	int vui_parameters_present_flag = ReadBit(gb);

	printf("vui_parameters_present_flag: %d\n", vui_parameters_present_flag);

	return vui_parameters_present_flag;

}


static int hevc_sps_find_vui_timing_info_present_flag(GetBitContext *gb)
{
	//printf("vui_parameters_present_flag: %d\n", vui_parameters_present_flag);
	//if(vui_parameters_present_flag)
	{
		//vui_parameters()
		{
    		int aspect_ratio_info_present_flag = ReadBit(gb);
	    	if(aspect_ratio_info_present_flag)
	    	{
	    		int aspect_ratio_idc = ReadBits(gb, 8);

	    		if(aspect_ratio_idc == 255)
	    		{
	    			int sar_width = ReadBits(gb, 16);
	    			int sar_height = ReadBits(gb, 16);
	    		}

	    	}
			printf("spect_ratio_info_present_flag: %d\n", aspect_ratio_info_present_flag);

	    	int overscan_info_present_flag = ReadBit(gb);
	    	if(overscan_info_present_flag)
	    	{
	    		int overscan_appropriate_flag = ReadBit(gb);
	    	}
			printf("overscan_info_present_flag: %d\n", overscan_info_present_flag);

	    	int video_signal_type_present_flag = ReadBit(gb);
	    	if(video_signal_type_present_flag) {
	    		int video_format = ReadBits(gb, 3);
	    		int video_full_range_flag = ReadBit(gb);
	    		int colour_description_present_flag = ReadBit(gb);
	    		if(colour_description_present_flag) {
	    			int colour_primaries = ReadBits(gb, 8);
	    			int transfer_characteristics = ReadBits(gb, 8);
    				int matrix_coeffs = ReadBits(gb, 8);
	    		}
	    	}
			printf("video_signal_type_present_flag: %d\n", video_signal_type_present_flag);

	    	int chroma_loc_info_present_flag = ReadBit(gb);
	    	if(chroma_loc_info_present_flag) {
	    		int chroma_sample_loc_type_top_field = ReadUE(gb);
	    		int chroma_sample_loc_type_bottom_field = ReadUE(gb);
	    	}
			printf("chroma_loc_info_present_flag: %d\n", chroma_loc_info_present_flag);

			int neutral_chroma_indocation_flag = ReadBit(gb);
			int filed_seq_flag = ReadBit(gb);
			int frame_filed_info_present_flag = ReadBit(gb);
			int default_display_window_flag = ReadBit(gb);
			if(default_display_window_flag) {
				int def_disp_win_left_offset = ReadUE(gb);
				int def_disp_win_right_offset = ReadUE(gb);
				int def_disp_win_top_offset = ReadUE(gb);
				int def_disp_win_bottom_offset = ReadUE(gb);
			}
			printf("neutral_chroma_indocation_flag: %d\n", neutral_chroma_indocation_flag);
			printf("filed_seq_flag: %d\n", filed_seq_flag);
			printf("frame_filed_info_present_flag: %d\n", frame_filed_info_present_flag);
			printf("default_display_window_flag: %d\n", default_display_window_flag);

	    	int vui_timing_info_present_flag = ReadBit(gb);


			return vui_timing_info_present_flag;

			printf("vui_timing_info_present_flag: %d\n", vui_timing_info_present_flag);
			//printf("m_nCurrentBit:%d\n", m_nCurrentBit);
	    	if(vui_timing_info_present_flag) {
	    		int vui_num_units_in_tick = ReadBits(gb, 32);
	    		int vui_time_scale = ReadBits(gb, 32);
				int vui_poc_proportional_to_timing_flag = ReadBit(gb);
				int vui_num_ticks_poc_diff_one_minus1 = 0;
				printf("vui_num_units_in_tick: %d\n", vui_num_units_in_tick);
				printf("vui_time_scale: %d\n", vui_time_scale);

				if(vui_poc_proportional_to_timing_flag)
					vui_num_ticks_poc_diff_one_minus1 = ReadUE(gb);
				int vui_hdr_parameters_present_flag = ReadBit(gb);
				if(vui_hdr_parameters_present_flag)
					//hrd_parameters(1, sps_max_sub_layers_minus1)
					{

					}
	    	}
			int bitstream_restriction_flag = ReadBit(gb);
			printf("bitstream_restriction_flag: %d\n", bitstream_restriction_flag);
			if(bitstream_restriction_flag) {
				int tiles_fixed_structure_flag = ReadBit(gb);
				int motion_vectors_over_pic_boundaries_flag = ReadBit(gb);
				int restricted_ref_pic_lists_flag = ReadBit(gb);
				int min_spatial_segmentation_id = ReadUE(gb);
				int max_bytes_per_pic_denom = ReadUE(gb);
				int max_bits_per_min_cu_denom = ReadUE(gb);
				int log2_max_mv_length_horizontal = ReadUE(gb);
				int log2_max_mv_length_vertical = ReadUE(gb);
			}

   /*
			printf("sapect_ratio_info_present_flag:%d\n", sapect_ratio_info_present_flag);
			printf("sapect_ratio_idc:%d\n", sapect_ratio_idc);
			printf("sar_width:%d\n", sar_width);
			printf("sar_height:%d\n", sar_height);
			printf("overscan_info_present_flag:%d\n", overscan_info_present_flag);
			printf("overscan_appropriate_flag:%d\n", overscan_appropriate_flag);
			printf("video_signal_type_present_flag:%d\n", video_signal_type_present_flag);
			printf("video_format:%d\n", video_format);
			printf("video_full_range_flag:%d\n", video_full_range_flag);
			printf("colour_description_present_flag:%d\n", colour_description_present_flag);
			printf("colour_primaries:%d\n", colour_primaries);
			printf("transfer_characteristics:%d\n", transfer_characteristics);
			printf("matrix_coefficients:%d\n", matrix_coefficients);
			printf("chroma_loc_info_present_flag:%d\n", chroma_loc_info_present_flag);


			printf("chroma_sample_loc_type_top_field:%d\n", chroma_sample_loc_type_top_field);
			printf("chroma_sample_loc_type_bottom_field:%d\n", chroma_sample_loc_type_bottom_field);
			printf("timing_info_present_flag:%d\n", timing_info_present_flag);
			printf("num_units_in_tick:%d\n", num_units_in_tick);
			printf("time_scale:%d\n", time_scale);
			printf("fixed_frame_rate_flag:%d\n", fixed_frame_rate_flag);
			printf("nal_hrd_parameters_present_flag:%d\n", nal_hrd_parameters_present_flag);
			printf("vcl_hrd_parameters_present_flag:%d\n", vcl_hrd_parameters_present_flag);
			printf("low_delay_hrd_flag:%d\n", low_delay_hrd_flag);
			printf("pic_struct_present_flag:%d\n", pic_struct_present_flag);
			printf("bitstream_restriction_flag:%d\n", bitstream_restriction_flag);
			printf("motion_vectors_over_pic_boundaries_flag:%d\n", motion_vectors_over_pic_boundaries_flag);
			printf("max_bytes_per_pic_denom:%d\n", max_bytes_per_pic_denom);
			printf("max_bits_per_mb_denom:%d\n", max_bits_per_mb_denom);

			printf("log2_max_mv_length_horizontal:%d\n", log2_max_mv_length_horizontal);
			printf("log2_max_mv_length_vertical:%d\n", log2_max_mv_length_vertical);
			printf("num_reorder_frames:%d\n", num_reorder_frames);
			printf("max_dec_frame_buffering:%d\n", max_dec_frame_buffering);
	*/
		}
	}
/*
	int sps_extension_flag = ReadBit(gb);
	if(sps_extension_flag) {
		while( more_rbsp_data() ) {
			sps_extension_data_flag = ReadBit(gb);
		}
	}
*/
	//rbsp_trailing_bits()
//	{
//		rbsp_stop_one_bit/*equal to 1*/  f(1)
//		while(!byte_aligned())
//			rbsp_alignment_zero_bit/*equal to 0*/ f(1)
//	}

}


static int remove_h264or5_emulation_bytes(unsigned char* to, unsigned int toMaxSize, unsigned char* from, unsigned int fromSize)
{
  unsigned toSize = 0;
  unsigned i = 0;
  while (i < fromSize && toSize+1 < toMaxSize) {
    if (i+2 < fromSize && from[i] == 0 && from[i+1] == 0 && from[i+2] == 3) {
      to[toSize] = to[toSize+1] = 0;
      toSize += 2;
      i += 3;
    } else {
      to[toSize] = from[i];
      toSize += 1;
      i += 1;
    }
  }

  return toSize;
}

static int add_h264or5_emulation_bytes(unsigned char* to, unsigned int toMaxSize, unsigned char* from, unsigned int fromSize)
{
  unsigned toSize = 0;
  unsigned i = 0;
  while (i < fromSize && toSize+1 < toMaxSize) {
    if (i+2 < fromSize && from[i] == 0 && from[i+1] == 0) {
      to[toSize] = to[toSize+1] = 0;
	  to[toSize+2] = 3;
      toSize += 3;
      i += 2;
    } else {
      to[toSize] = from[i];
      toSize += 1;
      i += 1;
    }
  }

  return toSize;
}


static int init_get_bits(GetBitContext *s, const uint8_t *buffer, int bit_size)
{
    int buffer_size;
    int ret = 0;

    buffer_size = (bit_size + 7) >> 3;

    s->buffer             = buffer;
    s->size_in_bits       = bit_size;
    s->buffer_end         = buffer + buffer_size;
    s->index              = 0;

    return ret;
}

static int init_set_bits(SetBitContext *s, uint8_t *buffer, int bit_size)
{
    int buffer_size;
    int ret = 0;

    buffer_size = (bit_size + 7) >> 3;

    s->buffer             = buffer;
    s->size_in_bits       = bit_size;
    s->buffer_end         = buffer + buffer_size;
    s->index              = 0;

    return ret;
}

/*
void *H265SPSParserOpen(){
    H265SPSParser *h;
    h = (H265SPSParser *) malloc (sizeof(H265SPSParser));
    if(h == NULL)
        goto fail;
    h->size = 1024 * 1024;
    h->buffer = (uint8_t *) malloc (h->size * sizeof(uint8_t));
    if(h->buffer == NULL)
        goto fail;

    return h;
fail:
    if(h != NULL){
        if(h->buffer != NULL)
            free(h->buffer);
        free(h);
    }

}

void H265SPSParserClose(void *handle){
    H265SPSParser *h = (H265SPSParser *)handle;
    if(h != NULL){
        if(h->buffer != NULL)
            free(h->buffer);
        free(h);
    }
}

int H265SPSParser(void *handle, uint8_t *pData, int nLen){
    int i = 0;
    H265SPSParser *h = (H265SPSParser *)handle;

    for(i =0 ; i < nLen; i++){
        if( (pData[i] == 0x00) &&
            (pData[i+1] == 0x00) &&
            (pData[i+2] == 0x00) &&
            (pData[i+3] == 0x01) ){
                //if(pData[i+4] == 0x67 || pData[i+4] == 0x27){
				if(((pData[i+4] & 0x7E) >> 1) == 33){

					h->size = removeH264or5EmulationBytes(h->buffer, nLen, pData+ i + 5 + 1, nLen - i - 5 - 1);
					printf("h->size: %d\n", h->size);
					for(i=0; i<h->size;i++)
						printf("%x ", h->buffer[i]);
					printf("\n");
                    init_get_bits(&h->gb, h->buffer, h->size * 8);
					//find_timing_info_present_flag(&h->gb);
					dump_hevc_sps_information(&h->gb);

                    break;
                }
        }
    }
    return 0;
}
*/

void *h265_sps_parser_open(){
    H265SPSParser *h;
    h = (H265SPSParser *) malloc (sizeof(H265SPSParser));
    if(h == NULL)
        goto fail;
    h->get_size = 1024;
    h->set_size = 1024;
    h->get_buffer = (uint8_t *) malloc (h->get_size * sizeof(uint8_t));
    h->set_buffer = (uint8_t *) malloc (h->set_size * sizeof(uint8_t));
    if((h->get_buffer == NULL) || (h->set_buffer == NULL))
        goto fail;

    return h;
fail:
    if(h != NULL){
        if(h->get_buffer != NULL)
            free(h->get_buffer);
        if(h->set_buffer != NULL)
            free(h->set_buffer);
        free(h);
    }

}

void h265_sps_parser_close(void *handle){
    H265SPSParser *h = (H265SPSParser *)handle;
    if(h != NULL){
        if(h->get_buffer != NULL)
            free(h->get_buffer);
        if(h->set_buffer != NULL)
            free(h->set_buffer);
        free(h);
    }
}

#define H265_START_CODE 0x000001
static int h265_find_next_start_code (uint8_t *pBuf,
                    uint32_t bufLen)
{
  uint32_t val;
  uint32_t offset;

  offset = 0;
  if (pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 0 && pBuf[3] == 1) {
    pBuf += 4;
    offset = 4;
  } else if (pBuf[0] == 0 && pBuf[1] == 0 && pBuf[2] == 1) {
    pBuf += 3;
    offset = 3;
  }
  val = 0xffffffff;
  while (offset < bufLen - 3) {
    val <<= 8;
    val |= *pBuf++;
    offset++;
    if (val == H265_START_CODE) {
      return offset - 4;
    }
    if ((val & 0x00ffffff) == H265_START_CODE) {
      return offset - 3;
    }
  }
  return 0;
}


int h265_sps_set_timing_flag(void *handle, unsigned char* to, unsigned int* toSize, unsigned char *from, unsigned int fromSize, int frame_rate_index){

    H265SPSParser *h = (H265SPSParser *)handle;
	unsigned char vps[100] = {0};
	unsigned char sps[100] = {0};
	unsigned char pps[100] = {0};
	int i = 0;

	int vps_length = h265_find_next_start_code(from, fromSize);
	int sps_length = h265_find_next_start_code(from + vps_length, fromSize - vps_length);
	int pps_length = fromSize - vps_length - sps_length;

	printf("vps_length: %d, sps_length: %d, pps_length: %d\n", vps_length, sps_length, pps_length);

	memcpy(vps, from, vps_length);
	memcpy(sps, from + vps_length, sps_length);
	memcpy(pps, from + vps_length + sps_length, pps_length);

	printf("vps:\n");
	for(i=0;i<vps_length;i++) {
		printf("0x%x ,", vps[i]);
	}
	printf("\n");

	printf("sps:\n");
	for(i=0;i<sps_length;i++) {
		printf("0x%x ,", sps[i]);
	}
	printf("\n");

	printf("pps:\n");
	for(i=0;i<pps_length;i++) {
		printf("0x%x ,", pps[i]);
	}
	printf("\n");

    for(i =0 ; i < sps_length; i++){
        if( (sps[i] == 0x00) &&
            (sps[i+1] == 0x00) &&
            (sps[i+2] == 0x00) &&
            (sps[i+3] == 0x01) ){
                //if(from[i+4] == 0x67 || from[i+4] == 0x27){
                if(((sps[i+4] & 0x7E) >> 1) == 33){

					h->get_size = remove_h264or5_emulation_bytes(h->get_buffer, sps_length, sps+i+5+1, sps_length-i-5-1);

                    init_get_bits(&h->gb, h->get_buffer, h->get_size * 8);
                    init_set_bits(&h->sb, h->set_buffer, h->set_size * 8);

					int vui_parameters_present_flag = hevc_sps_find_vui_parameters_present_flag(&h->gb);
					printf("======vui_parameters_present_flag: %d\n", vui_parameters_present_flag);
					if(vui_parameters_present_flag)
					{
						int vui_timing_info_present_flag = hevc_sps_find_vui_timing_info_present_flag(&h->gb);
						printf("======vui_timing_info_present_flag: %d\n", vui_timing_info_present_flag);
						if(vui_timing_info_present_flag == 0)
						{
							printf("timing flag index: %d\n", h->gb.index);
							//WriteBits(&h->sb, h->buffer, h->gb.index);
							WriteBits2(&h->sb, h->get_buffer, 0, h->gb.index-1);
							//vui_timing_info_present_flag	u(1)
							WriteBit(&h->sb, 0x01);
							//vui_num_units_in_tick u(32)
							//unsigned char num_units_in_tick[4] = {0x00, 0x00, 0x00, 0x01};
							//unsigned char num_units_in_tick[4] = {0x00, 0x00, 0x03, 0xE8};
							unsigned int vui_num_units_in_tick = swap(framerate_table[frame_rate_index].num_units_in_tick);
							//unsigned int num_units_in_tick = 1;
							WriteBits(&h->sb, &vui_num_units_in_tick, 32);
							//vui_time_scale	(u32)
							//unsigned char time_scale[4] = {0x00, 0x00, 0x00, 0x3C};
							//unsigned char time_scale[4] = {0x00, 0x00, 0x75, 0x30};
							unsigned int vui_time_scale = swap(framerate_table[frame_rate_index].time_scale);
							WriteBits(&h->sb, &vui_time_scale, 32);
							//vui_proc_proportional_to_timing_flag u(1)

							unsigned char vui_proc_proportional_to_timing_flag = framerate_table[frame_rate_index].fixed_frame_rate_flag;
							WriteBit(&h->sb, vui_proc_proportional_to_timing_flag);
							//WriteBit(&h->sb, 0x00);

							//vui_hrd_parameters_present_flag	u(1)
							WriteBit(&h->sb, 0x00);

							WriteBits2(&h->sb, h->get_buffer+h->gb.index/8, (8-(h->gb.index%8)), h->get_size * 8 - h->gb.index);
							WriteBitsEnd(&h->sb);
							printf("h->sb.index:%d\n", h->sb.index);

							memcpy(to, vps, vps_length);
							memcpy(to + vps_length, sps, 6);
							*toSize = vps_length + 6;

							*toSize += add_h264or5_emulation_bytes(to + *toSize, h->sb.index/8*2, h->sb.buffer, h->sb.index/8);
							memcpy(to + *toSize, pps, pps_length);
							*toSize += pps_length;
						}
						else
						{
							memcpy(to, vps, vps_length);
							memcpy(to + vps_length, sps, sps_length);
							memcpy(to + vps_length + sps_length, pps, pps_length);
							*toSize = vps_length + sps_length + pps_length;
						}

					}
					else
					{
						WriteBits2(&h->sb, h->get_buffer, 0, h->gb.index-1);
						//vui_parameters_present_flag	u(1)
						WriteBit(&h->sb, 0x01);
						//aspect_ratio_info_present_flag u(1)
						WriteBit(&h->sb, 0x00);
						//overscan_info_present_flag u(1)
						WriteBit(&h->sb, 0x00);
						//video_signal_type_present_flag u(1)
						WriteBit(&h->sb, 0x00);
						//chroma_loc_info_present_flag	u(1)
						WriteBit(&h->sb, 0x00);

						//neutral_chroma_indication_flag	u(1)
						WriteBit(&h->sb, 0x00);
						//field_seq_flag	u(1)
						WriteBit(&h->sb, 0x00);
						//frame_field_info_present_flag	u(1)
						WriteBit(&h->sb, 0x00);
						//default_display_window_flag	u(1)
						WriteBit(&h->sb, 0x00);


						//vui_timing_info_present_flag	u(1)
						WriteBit(&h->sb, 0x01);
						//vui_num_units_in_tick u(32)
						//unsigned char num_units_in_tick[4] = {0x00, 0x00, 0x00, 0x01};
						//unsigned char num_units_in_tick[4] = {0x00, 0x00, 0x03, 0xE8};
						unsigned int num_units_in_tick = swap(framerate_table[frame_rate_index].num_units_in_tick);
						//unsigned int num_units_in_tick = 1;
						WriteBits(&h->sb, &num_units_in_tick, 32);

						//vui_time_scale	(u32)
						//unsigned char time_scale[4] = {0x00, 0x00, 0x00, 0x3C};
						//unsigned char time_scale[4] = {0x00, 0x00, 0x75, 0x30};
						unsigned int time_scale = swap(framerate_table[frame_rate_index].time_scale);
						WriteBits(&h->sb, &time_scale, 32);

						//vui_poc_proportional_to_timing_flag u(1)
						unsigned char fixed_frame_rate_flag = framerate_table[frame_rate_index].fixed_frame_rate_flag;
						//WriteBit(&h->sb, vui_poc_proportional_to_timing_flag);
						WriteBit(&h->sb, 0x00);

						//vui_hrd_parameters_present_flag	u(1)
						WriteBit(&h->sb, 0x00);

						//bitstream_restriction_flag u(1)
						WriteBit(&h->sb, 0x00);

						WriteBits2(&h->sb, h->get_buffer+h->gb.index/8, (8-(h->gb.index%8)), h->get_size * 8 - h->gb.index);
						WriteBitsEnd(&h->sb);
						printf("h->sb.index:%d\n", h->sb.index);

						memcpy(to, vps, vps_length);
						memcpy(to + vps_length, sps, 6);
						*toSize = vps_length +  6;

						*toSize += add_h264or5_emulation_bytes(to + *toSize, h->sb.index/8*2, h->sb.buffer, h->sb.index/8);
						memcpy(to + *toSize, pps, pps_length);

						*toSize += pps_length;

					}

/*

					int timing_info_present_flag = find_timing_info_present_flag(&h->gb);
					if(timing_info_present_flag == 0)
					{
						printf("timing flag index: %d\n", h->gb.index);
						//WriteBits(&h->sb, h->buffer, h->gb.index);
						WriteBits2(&h->sb, h->get_buffer, 0, h->gb.index-1);
						//timing_info_present_flag	u(1)
						WriteBit(&h->sb, 0x01);
						//num_units_in_tick u(32)
						//unsigned char num_units_in_tick[4] = {0x00, 0x00, 0x00, 0x01};
						unsigned int num_units_in_tick = swap(framerate_table[frame_rate_index].num_units_in_tick);
						//unsigned int num_units_in_tick = 1;
						WriteBits(&h->sb, &num_units_in_tick, 32);
						//time_scale	(u32)
						//unsigned char time_scale[4] = {0x00, 0x00, 0x00, 0x3C};
						unsigned int time_scale = swap(framerate_table[frame_rate_index].time_scale);
						WriteBits(&h->sb, &time_scale, 32);
						//fixed_frame_rate_flag u(1)

						unsigned char fixed_frame_rate_flag = framerate_table[frame_rate_index].fixed_frame_rate_flag;
						WriteBit(&h->sb, fixed_frame_rate_flag);
						//WriteBit(&h->sb, 0x00);

						WriteBits2(&h->sb, h->get_buffer+h->gb.index/8, (8-(h->gb.index%8)), h->get_size * 8 - h->gb.index);
						WriteBitsEnd(&h->sb);
						printf("h->sb.index:%d\n", h->sb.index);

						memcpy(to, from, 5);
						*toSize = 5;

						*toSize += add_h264or5_emulation_bytes(to+5, h->sb.index/8*2, h->sb.buffer, h->sb.index/8);
					}
					else
					{
						memcpy(to, from, fromSize);
						*toSize = fromSize;
					}
*/
                    break;
                }
        }
    }
    return 0;
}



/*
int main()
{
    //unsigned char data[100] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xAC, 0xB4, 0x03, 0xC0, 0x11, 0x3F, 0x2C, 0xAC, 0x14, 0x18,
     //       0x14, 0x1B, 0x42, 0x84, 0xD4, '\0'};

    //unsigned char data[100] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xAC, 0xB4, 0x03, 0xC0, 0x11, 0x3F, 0x2C, 0xAC, 0x14, 0x18,
      //      0x15, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x1E, 0x0D, 0xA1, 0x42, 0x6A, 0x00, '\0'};


	//unsigned int len = 23;
	//unsigned int len = 34;

//	unsigned char data[100] = {0x00,0x00,0x00,0x01,0x42,0x01,0x01,0x01,0x60,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x00,0x03,0x00,0x78,0xa0,0x03,0xc0,0x80,0x10,0xe5,0x8d,0xf9,0x24,0x53,0x77,0xc5,0x55,0x2e,0x57,0x6c,0x80};
//	unsigned int len = 40;

//	unsigned char data[100] = {0x00, 0x00, 0x00, 0x01, 0x42, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x78, 0xa0, 0x03, 0xc0, 0x80, 0x10, 0xe5, 0x8d, 0xf9, 0x24, 0x53, 0x77, 0xc5, 0x55, 0x2e, 0x57, 0x6e, 0x10, 0x00, 0x00, 0x03, 0x3e, 0x80, 0x00, 0x0e, 0xa6, 0x00, 0x20, 0x00};

//	unsigned int len = 50;

	unsigned char data[100] = {0x0, 0x0, 0x0, 0x1, 0x42, 0x1, 0x1, 0x1, 0x60, 0x0, 0x0, 0x3, 0x0, 0x0, 0x3, 0x0, 0x0, 0x3, 0x0, 0x0, 0x3, 0x0, 0x78, 0xa0, 0x3, 0xc0, 0x80, 0x10, 0xe5, 0x8d, 0xf9, 0x24, 0x53, 0x77, 0xc5, 0x55, 0x2e, 0x57, 0x6e, 0x1, 0x0, 0x0, 0x3, 0x3, 0xe8, 0x0, 0x0, 0x3, 0xea, 0x60, 0x8, 0x0};
	unsigned int len = 52;

    unsigned char data2[100] = {0};
    unsigned int len2 = 0;

	int i = 0;
	for(i = 0; i < len; i++)
		printf("%x ", data[i]);


    printf("\n\nBuffer size %d\n", len);


    void* h = h265_sps_parser_open();

    h265_sps_set_timing_flag(h, data2, &len2, data, len, FPS_30);

    printf("len2:%d\n", len2);
    for(i = 0; i < len2; i++)
        printf("0x%x, ", data2[i]);
    printf("\n");

    h265_sps_parser_close(h);


    return 0;
}
*/
