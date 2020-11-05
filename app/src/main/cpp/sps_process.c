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

typedef struct H264SPSParser {
    GetBitContext gb;
    SetBitContext sb;
    uint8_t *get_buffer;
    uint8_t *set_buffer;
    int get_size;
    int set_size;
} H264SPSParser;

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

typedef struct FRAMERATEINFO {
    int frame_rate;
    unsigned int num_units_in_tick;
    unsigned int time_scale;
    unsigned int fixed_frame_rate_flag;
} FRAMERATEINFO;


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

static int swap(int value) {
    return ((value & 0x000000FF) << 24) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0xFF000000) >> 24);
}

static unsigned int ReadBit(GetBitContext *gb) {
    uint8_t nValue;
    int nIndex, nOffset;

    assert(gb->index <= gb->size_in_bits);

    nIndex = gb->index / 8;
    nOffset = gb->index % 8 + 1;
    nValue = *(gb->buffer + nIndex);

    gb->index++;

    return (nValue >> (8 - nOffset)) & 0x01;
}

static unsigned int ReadBits(GetBitContext *gb, int n) {
    int r = 0, i = 0;
    for (i = 0; i < n; i++) {
        r |= (ReadBit(gb) << (n - i - 1));
    }
    return r;
}

static unsigned int ReadUE(GetBitContext *gb) {
    int r = 0, i = 0;

    while ((ReadBit(gb) == 0) && (i < 32)) {
        i++;
    }
    r = ReadBits(gb, i);
    r += (1 << i) - 1;

    return r;
}

static unsigned int ReadSE(GetBitContext *gb) {
    int r = 0;

    r = ReadUE(gb);
    if (r & 0x01) {
        r = (r + 1) / 2;
    } else {
        r = -(r / 2);
    }

    return r;
}


static unsigned int WriteBit(SetBitContext *sb, unsigned char data) {
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

static unsigned int WriteBits(SetBitContext *sb, unsigned char *data, int n) {
    int r = 1, i = 0;
    for (i = 0; i < n; i++) {
        r |= WriteBit(sb, ((data[i / 8] >> (7 - (i % 8))) & 0x01));
    }
    return r;
}

static unsigned int WriteBits2(SetBitContext *sb, unsigned char *data, int start, int n) {
    int r = 1, i = 0;

    if (start) {
        for (i = 0; i < start; i++) {
            r |= WriteBit(sb, ((data[0] >> (start - i - 1)) & 0x01));
        }
        r |= WriteBits(sb, data + 1, n - start);
    } else {
        r |= WriteBits(sb, data, n - start);
    }

    return r;
}

static unsigned int WriteBitsEnd(SetBitContext *sb) {
    int r = 1, i = 0;
    if (sb->index % 8) {
        char data = 0x00;
        r = WriteBits(sb, &data, 8 - (sb->index % 8));
    }

    return r;
}

/*
unsigned int find_timing_info_present_flag(GetBitContext *gb)
{
    int frame_crop_left_offset=0;
    int frame_crop_right_offset=0;
    int frame_crop_top_offset=0;
    int frame_crop_bottom_offset=0;

    int profile_idc = ReadBits(gb, 8);
    int constraint_set0_flag = ReadBit(gb);
    int constraint_set1_flag = ReadBit(gb);
    int constraint_set2_flag = ReadBit(gb);
    int constraint_set3_flag = ReadBit(gb);
    int constraint_set4_flag = ReadBit(gb);
    int constraint_set5_flag = ReadBit(gb);
    int reserved_zero_2bits  = ReadBits(gb, 2);
    int level_idc = ReadBits(gb, 8);
    int seq_parameter_set_id = ReadUE(gb);


    if( profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 ||
        profile_idc == 44 || profile_idc == 83 ||
        profile_idc == 86 || profile_idc == 118 )
    {
        int chroma_format_idc = ReadUE(gb);

        if( chroma_format_idc == 3 )
        {
            int residual_colour_transform_flag = ReadBit(gb);
        }
        int bit_depth_luma_minus8 = ReadUE(gb);
        int bit_depth_chroma_minus8 = ReadUE(gb);
        int qpprime_y_zero_transform_bypass_flag = ReadBit(gb);
        int seq_scaling_matrix_present_flag = ReadBit(gb);

        if (seq_scaling_matrix_present_flag)
        {
            int i=0;
            for ( i = 0; i < 8; i++)
            {
                int seq_scaling_list_present_flag = ReadBit(gb);
                if (seq_scaling_list_present_flag)
                {
                    int sizeOfScalingList = (i < 6) ? 16 : 64;
                    int lastScale = 8;
                    int nextScale = 8;
                    int j=0;
                    for ( j = 0; j < sizeOfScalingList; j++)
                    {
                        if (nextScale != 0)
                        {
                            int delta_scale = ReadSE(gb);
                            nextScale = (lastScale + delta_scale + 256) % 256;
                        }
                        lastScale = (nextScale == 0) ? lastScale : nextScale;
                    }
                }
            }
        }
    }

    int log2_max_frame_num_minus4 = ReadUE(gb);
    int pic_order_cnt_type = ReadUE(gb);
    if( pic_order_cnt_type == 0 )
    {
        int log2_max_pic_order_cnt_lsb_minus4 = ReadUE(gb);
    }
    else if( pic_order_cnt_type == 1 )
    {
        int delta_pic_order_always_zero_flag = ReadBit(gb);
        int offset_for_non_ref_pic = ReadSE(gb);
        int offset_for_top_to_bottom_field = ReadSE(gb);
        int num_ref_frames_in_pic_order_cnt_cycle = ReadUE(gb);
        int i;
        for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
        {
            ReadSE(gb);
            //sps->offset_for_ref_frame[ i ] = ReadSE(gb);
        }
    }
    int max_num_ref_frames = ReadUE(gb);
    int gaps_in_frame_num_value_allowed_flag = ReadBit(gb);
    int pic_width_in_mbs_minus1 = ReadUE(gb);
    int pic_height_in_map_units_minus1 = ReadUE(gb);
    int frame_mbs_only_flag = ReadBit(gb);
    if( !frame_mbs_only_flag )
    {
        int mb_adaptive_frame_field_flag = ReadBit(gb);
    }
    int direct_8x8_inference_flag = ReadBit(gb);
    int frame_cropping_flag = ReadBit(gb);
    if( frame_cropping_flag )
    {
        frame_crop_left_offset = ReadUE(gb);
        frame_crop_right_offset = ReadUE(gb);
        frame_crop_top_offset = ReadUE(gb);
        frame_crop_bottom_offset = ReadUE(gb);
    }
    int vui_parameters_present_flag = ReadBit(gb);


    int sapect_ratio_info_present_flag = 0;
    int sapect_ratio_idc = 0;
    int sar_width = 0;
    int sar_height = 0;
    int overscan_info_present_flag = 0;
    int overscan_appropriate_flag = 0;
    int video_signal_type_present_flag = 0;
    int video_format = 0;
    int video_full_range_flag = 0;
    int colour_description_present_flag = 0;
    int colour_primaries = 0;
    int transfer_characteristics = 0;
    int matrix_coefficients = 0;
    int chroma_loc_info_present_flag = 0;

    int chroma_sample_loc_type_top_field = 0;
    int chroma_sample_loc_type_bottom_field  = 0;
    int timing_info_present_flag = 0;
    unsigned int num_units_in_tick = 0;
    unsigned int time_scale = 0;
    int fixed_frame_rate_flag = 0;
    int nal_hrd_parameters_present_flag = 0;
    int vcl_hrd_parameters_present_flag = 0;
    int low_delay_hrd_flag = 0 ;
    int pic_struct_present_flag = 0;
    int bitstream_restriction_flag = 0;
    int motion_vectors_over_pic_boundaries_flag = 0;
    int max_bytes_per_pic_denom = 0;
    int max_bits_per_mb_denom = 0;
    int log2_max_mv_length_horizontal = 0;
    int log2_max_mv_length_vertical = 0;
    int num_reorder_frames = 0;
    int max_dec_frame_buffering = 0;


    if(vui_parameters_present_flag)
    {
    	sapect_ratio_info_present_flag = ReadBit(gb);
    	if(sapect_ratio_info_present_flag)
    	{
    		sapect_ratio_idc = ReadBits(gb, 8);

    		if(sapect_ratio_idc == 255)
    		{
    			sar_width = ReadBits(gb, 16);
    			sar_height = ReadBits(gb, 16);
    		}

    	}

    	overscan_info_present_flag = ReadBit(gb);
    	if(overscan_info_present_flag)
    	{
    		overscan_appropriate_flag = ReadBit(gb);
    	}

    	video_signal_type_present_flag = ReadBit(gb);
    	if(video_signal_type_present_flag)
    	{
    		video_format = ReadBits(gb, 3);
    		video_full_range_flag = ReadBit(gb);
    		colour_description_present_flag = ReadBit(gb);
    		if(colour_description_present_flag)
    		{
    			colour_primaries = ReadBits(gb, 8);
    			transfer_characteristics = ReadBits(gb, 8);
    			matrix_coefficients = ReadBits(gb, 8);
    		}
    	}

    	chroma_loc_info_present_flag = ReadBit(gb);
    	if(chroma_loc_info_present_flag)
    	{
    		chroma_sample_loc_type_top_field = ReadUE(gb);
    		chroma_sample_loc_type_bottom_field = ReadUE(gb);
    	}

    	timing_info_present_flag = ReadBit(gb);
	}

	return timing_info_present_flag;

}
*/

unsigned int find_vui_parameters_present_flag(GetBitContext *gb) {
    int frame_crop_left_offset = 0;
    int frame_crop_right_offset = 0;
    int frame_crop_top_offset = 0;
    int frame_crop_bottom_offset = 0;

    int profile_idc = ReadBits(gb, 8);
    int constraint_set0_flag = ReadBit(gb);
    int constraint_set1_flag = ReadBit(gb);
    int constraint_set2_flag = ReadBit(gb);
    int constraint_set3_flag = ReadBit(gb);
    int constraint_set4_flag = ReadBit(gb);
    int constraint_set5_flag = ReadBit(gb);
    int reserved_zero_2bits = ReadBits(gb, 2);
    int level_idc = ReadBits(gb, 8);
    int seq_parameter_set_id = ReadUE(gb);


    if (profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 ||
        profile_idc == 44 || profile_idc == 83 ||
        profile_idc == 86 || profile_idc == 118) {
        int chroma_format_idc = ReadUE(gb);

        if (chroma_format_idc == 3) {
            int residual_colour_transform_flag = ReadBit(gb);
        }
        int bit_depth_luma_minus8 = ReadUE(gb);
        int bit_depth_chroma_minus8 = ReadUE(gb);
        int qpprime_y_zero_transform_bypass_flag = ReadBit(gb);
        int seq_scaling_matrix_present_flag = ReadBit(gb);

        if (seq_scaling_matrix_present_flag) {
            int i = 0;
            for (i = 0; i < 8; i++) {
                int seq_scaling_list_present_flag = ReadBit(gb);
                if (seq_scaling_list_present_flag) {
                    int sizeOfScalingList = (i < 6) ? 16 : 64;
                    int lastScale = 8;
                    int nextScale = 8;
                    int j = 0;
                    for (j = 0; j < sizeOfScalingList; j++) {
                        if (nextScale != 0) {
                            int delta_scale = ReadSE(gb);
                            nextScale = (lastScale + delta_scale + 256) % 256;
                        }
                        lastScale = (nextScale == 0) ? lastScale : nextScale;
                    }
                }
            }
        }
    }

    int log2_max_frame_num_minus4 = ReadUE(gb);
    int pic_order_cnt_type = ReadUE(gb);
    if (pic_order_cnt_type == 0) {
        int log2_max_pic_order_cnt_lsb_minus4 = ReadUE(gb);
    } else if (pic_order_cnt_type == 1) {
        int delta_pic_order_always_zero_flag = ReadBit(gb);
        int offset_for_non_ref_pic = ReadSE(gb);
        int offset_for_top_to_bottom_field = ReadSE(gb);
        int num_ref_frames_in_pic_order_cnt_cycle = ReadUE(gb);
        int i;
        for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            ReadSE(gb);
            //sps->offset_for_ref_frame[ i ] = ReadSE(gb);
        }
    }
    int max_num_ref_frames = ReadUE(gb);
    int gaps_in_frame_num_value_allowed_flag = ReadBit(gb);
    int pic_width_in_mbs_minus1 = ReadUE(gb);
    int pic_height_in_map_units_minus1 = ReadUE(gb);
    int frame_mbs_only_flag = ReadBit(gb);
    if (!frame_mbs_only_flag) {
        int mb_adaptive_frame_field_flag = ReadBit(gb);
    }
    int direct_8x8_inference_flag = ReadBit(gb);
    int frame_cropping_flag = ReadBit(gb);
    if (frame_cropping_flag) {
        frame_crop_left_offset = ReadUE(gb);
        frame_crop_right_offset = ReadUE(gb);
        frame_crop_top_offset = ReadUE(gb);
        frame_crop_bottom_offset = ReadUE(gb);
    }
    int vui_parameters_present_flag = ReadBit(gb);

    return vui_parameters_present_flag;
}

unsigned int find_timing_info_present_flag(GetBitContext *gb) {
    int sapect_ratio_info_present_flag = 0;
    int sapect_ratio_idc = 0;
    int sar_width = 0;
    int sar_height = 0;
    int overscan_info_present_flag = 0;
    int overscan_appropriate_flag = 0;
    int video_signal_type_present_flag = 0;
    int video_format = 0;
    int video_full_range_flag = 0;
    int colour_description_present_flag = 0;
    int colour_primaries = 0;
    int transfer_characteristics = 0;
    int matrix_coefficients = 0;
    int chroma_loc_info_present_flag = 0;

    int chroma_sample_loc_type_top_field = 0;
    int chroma_sample_loc_type_bottom_field = 0;
    int timing_info_present_flag = 0;
    unsigned int num_units_in_tick = 0;
    unsigned int time_scale = 0;
    int fixed_frame_rate_flag = 0;
    int nal_hrd_parameters_present_flag = 0;
    int vcl_hrd_parameters_present_flag = 0;
    int low_delay_hrd_flag = 0;
    int pic_struct_present_flag = 0;
    int bitstream_restriction_flag = 0;
    int motion_vectors_over_pic_boundaries_flag = 0;
    int max_bytes_per_pic_denom = 0;
    int max_bits_per_mb_denom = 0;
    int log2_max_mv_length_horizontal = 0;
    int log2_max_mv_length_vertical = 0;
    int num_reorder_frames = 0;
    int max_dec_frame_buffering = 0;


    //if(vui_parameters_present_flag)
    if (1) {
        sapect_ratio_info_present_flag = ReadBit(gb);
        if (sapect_ratio_info_present_flag) {
            sapect_ratio_idc = ReadBits(gb, 8);

            if (sapect_ratio_idc == 255) {
                sar_width = ReadBits(gb, 16);
                sar_height = ReadBits(gb, 16);
            }

        }

        overscan_info_present_flag = ReadBit(gb);
        if (overscan_info_present_flag) {
            overscan_appropriate_flag = ReadBit(gb);
        }

        video_signal_type_present_flag = ReadBit(gb);
        if (video_signal_type_present_flag) {
            video_format = ReadBits(gb, 3);
            video_full_range_flag = ReadBit(gb);
            colour_description_present_flag = ReadBit(gb);
            if (colour_description_present_flag) {
                colour_primaries = ReadBits(gb, 8);
                transfer_characteristics = ReadBits(gb, 8);
                matrix_coefficients = ReadBits(gb, 8);
            }
        }

        chroma_loc_info_present_flag = ReadBit(gb);
        if (chroma_loc_info_present_flag) {
            chroma_sample_loc_type_top_field = ReadUE(gb);
            chroma_sample_loc_type_bottom_field = ReadUE(gb);
        }

        timing_info_present_flag = ReadBit(gb);
    }

    return timing_info_present_flag;

}


unsigned int dump_sps_information(GetBitContext *gb) {
    int frame_crop_left_offset = 0;
    int frame_crop_right_offset = 0;
    int frame_crop_top_offset = 0;
    int frame_crop_bottom_offset = 0;

    int profile_idc = ReadBits(gb, 8);
    int constraint_set0_flag = ReadBit(gb);
    int constraint_set1_flag = ReadBit(gb);
    int constraint_set2_flag = ReadBit(gb);
    int constraint_set3_flag = ReadBit(gb);
    int constraint_set4_flag = ReadBit(gb);
    int constraint_set5_flag = ReadBit(gb);
    int reserved_zero_2bits = ReadBits(gb, 2);
    int level_idc = ReadBits(gb, 8);
    int seq_parameter_set_id = ReadUE(gb);


    if (profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 ||
        profile_idc == 44 || profile_idc == 83 ||
        profile_idc == 86 || profile_idc == 118) {
        int chroma_format_idc = ReadUE(gb);

        if (chroma_format_idc == 3) {
            int residual_colour_transform_flag = ReadBit(gb);
        }
        int bit_depth_luma_minus8 = ReadUE(gb);
        int bit_depth_chroma_minus8 = ReadUE(gb);
        int qpprime_y_zero_transform_bypass_flag = ReadBit(gb);
        int seq_scaling_matrix_present_flag = ReadBit(gb);

        if (seq_scaling_matrix_present_flag) {
            int i = 0;
            for (i = 0; i < 8; i++) {
                int seq_scaling_list_present_flag = ReadBit(gb);
                if (seq_scaling_list_present_flag) {
                    int sizeOfScalingList = (i < 6) ? 16 : 64;
                    int lastScale = 8;
                    int nextScale = 8;
                    int j = 0;
                    for (j = 0; j < sizeOfScalingList; j++) {
                        if (nextScale != 0) {
                            int delta_scale = ReadSE(gb);
                            nextScale = (lastScale + delta_scale + 256) % 256;
                        }
                        lastScale = (nextScale == 0) ? lastScale : nextScale;
                    }
                }
            }
        }
    }

    int log2_max_frame_num_minus4 = ReadUE(gb);
    int pic_order_cnt_type = ReadUE(gb);
    if (pic_order_cnt_type == 0) {
        int log2_max_pic_order_cnt_lsb_minus4 = ReadUE(gb);
    } else if (pic_order_cnt_type == 1) {
        int delta_pic_order_always_zero_flag = ReadBit(gb);
        int offset_for_non_ref_pic = ReadSE(gb);
        int offset_for_top_to_bottom_field = ReadSE(gb);
        int num_ref_frames_in_pic_order_cnt_cycle = ReadUE(gb);
        int i;
        for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
            ReadSE(gb);
            //sps->offset_for_ref_frame[ i ] = ReadSE(gb);
        }
    }
    int max_num_ref_frames = ReadUE(gb);
    int gaps_in_frame_num_value_allowed_flag = ReadBit(gb);
    int pic_width_in_mbs_minus1 = ReadUE(gb);
    int pic_height_in_map_units_minus1 = ReadUE(gb);
    int frame_mbs_only_flag = ReadBit(gb);
    if (!frame_mbs_only_flag) {
        int mb_adaptive_frame_field_flag = ReadBit(gb);
    }
    int direct_8x8_inference_flag = ReadBit(gb);
    int frame_cropping_flag = ReadBit(gb);
    if (frame_cropping_flag) {
        frame_crop_left_offset = ReadUE(gb);
        frame_crop_right_offset = ReadUE(gb);
        frame_crop_top_offset = ReadUE(gb);
        frame_crop_bottom_offset = ReadUE(gb);
    }
    int vui_parameters_present_flag = ReadBit(gb);

//------------------------
/*
    if(vui_parameters_present_flag)
    {
    		vui_parameters();
    }
*/

    int sapect_ratio_info_present_flag = 0;
    int sapect_ratio_idc = 0;
    int sar_width = 0;
    int sar_height = 0;
    int overscan_info_present_flag = 0;
    int overscan_appropriate_flag = 0;
    int video_signal_type_present_flag = 0;
    int video_format = 0;
    int video_full_range_flag = 0;
    int colour_description_present_flag = 0;
    int colour_primaries = 0;
    int transfer_characteristics = 0;
    int matrix_coefficients = 0;
    int chroma_loc_info_present_flag = 0;

    int chroma_sample_loc_type_top_field = 0;
    int chroma_sample_loc_type_bottom_field = 0;
    int timing_info_present_flag = 0;
    unsigned int num_units_in_tick = 0;
    unsigned int time_scale = 0;
    int fixed_frame_rate_flag = 0;
    int nal_hrd_parameters_present_flag = 0;
    int vcl_hrd_parameters_present_flag = 0;
    int low_delay_hrd_flag = 0;
    int pic_struct_present_flag = 0;
    int bitstream_restriction_flag = 0;
    int motion_vectors_over_pic_boundaries_flag = 0;
    int max_bytes_per_pic_denom = 0;
    int max_bits_per_mb_denom = 0;
    int log2_max_mv_length_horizontal = 0;
    int log2_max_mv_length_vertical = 0;
    int num_reorder_frames = 0;
    int max_dec_frame_buffering = 0;


    if (vui_parameters_present_flag) {
        sapect_ratio_info_present_flag = ReadBit(gb);
        if (sapect_ratio_info_present_flag) {
            sapect_ratio_idc = ReadBits(gb, 8);

            if (sapect_ratio_idc == 255) {
                sar_width = ReadBits(gb, 16);
                sar_height = ReadBits(gb, 16);
            }

        }

        overscan_info_present_flag = ReadBit(gb);
        if (overscan_info_present_flag) {
            overscan_appropriate_flag = ReadBit(gb);
        }

        video_signal_type_present_flag = ReadBit(gb);
        if (video_signal_type_present_flag) {
            video_format = ReadBits(gb, 3);
            video_full_range_flag = ReadBit(gb);
            colour_description_present_flag = ReadBit(gb);
            if (colour_description_present_flag) {
                colour_primaries = ReadBits(gb, 8);
                transfer_characteristics = ReadBits(gb, 8);
                matrix_coefficients = ReadBits(gb, 8);
            }
        }

        chroma_loc_info_present_flag = ReadBit(gb);
        if (chroma_loc_info_present_flag) {
            chroma_sample_loc_type_top_field = ReadUE(gb);
            chroma_sample_loc_type_bottom_field = ReadUE(gb);
        }

        timing_info_present_flag = ReadBit(gb);
        //printf("m_nCurrentBit:%d\n", m_nCurrentBit);
        if (timing_info_present_flag) {
            num_units_in_tick = ReadBits(gb, 32);
            time_scale = ReadBits(gb, 32);;
            fixed_frame_rate_flag = ReadBit(gb);
        }

        nal_hrd_parameters_present_flag = ReadBit(gb);


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
        printf("motion_vectors_over_pic_boundaries_flag:%d\n",
               motion_vectors_over_pic_boundaries_flag);
        printf("max_bytes_per_pic_denom:%d\n", max_bytes_per_pic_denom);
        printf("max_bits_per_mb_denom:%d\n", max_bits_per_mb_denom);

        printf("log2_max_mv_length_horizontal:%d\n", log2_max_mv_length_horizontal);
        printf("log2_max_mv_length_vertical:%d\n", log2_max_mv_length_vertical);
        printf("num_reorder_frames:%d\n", num_reorder_frames);
        printf("max_dec_frame_buffering:%d\n", max_dec_frame_buffering);


        int cpb_cnt_minus1 = 0;
        int bit_rate_scale = 0;
        int cpb_size_scale = 0;
        int bit_rate_value_minus1[64] = {0};
        int cpb_size_value_minus1[64] = {0};
        int cbr_flag[64] = {0};


        int initial_cpb_removal_delay_length_minus1 = 0;
        int cpb_removal_delay_length_minus1 = 0;
        int dpb_output_delay_length_minus1 = 0;
        int time_offset_length = 0;

        if (nal_hrd_parameters_present_flag) {
            cpb_cnt_minus1 = ReadUE(gb);
            bit_rate_scale = ReadBits(gb, 4);
            cpb_size_scale = ReadBits(gb, 4);

            int SchedSelIdx;
            for (SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; SchedSelIdx++) {
                bit_rate_value_minus1[SchedSelIdx] = ReadUE(gb);
                cpb_size_value_minus1[SchedSelIdx] = ReadUE(gb);
                cbr_flag[SchedSelIdx] = ReadBit(gb);
            }

            initial_cpb_removal_delay_length_minus1 = ReadBits(gb, 5);
            cpb_removal_delay_length_minus1 = ReadBits(gb, 5);
            dpb_output_delay_length_minus1 = ReadBits(gb, 5);
            time_offset_length = ReadBits(gb, 5);
        }

        /*
        if(nal_hrd_parameters_present_flag)
        {
            hrd_parameters();
        }
        */

        vcl_hrd_parameters_present_flag = ReadBit(gb);

        /*
        if(vcl_hrd_parameters_present_flag)
        {
            hrd_parameters();
        }
        */

        if (vcl_hrd_parameters_present_flag) {
            cpb_cnt_minus1 = ReadUE(gb);
            bit_rate_scale = ReadBits(gb, 4);
            cpb_size_scale = ReadBits(gb, 4);

            int SchedSelIdx;
            for (SchedSelIdx = 0; SchedSelIdx <= cpb_cnt_minus1; SchedSelIdx++) {
                bit_rate_value_minus1[SchedSelIdx] = ReadUE(gb);
                cpb_size_value_minus1[SchedSelIdx] = ReadUE(gb);
                cbr_flag[SchedSelIdx] = ReadBit(gb);
            }

            initial_cpb_removal_delay_length_minus1 = ReadBits(gb, 5);
            cpb_removal_delay_length_minus1 = ReadBits(gb, 5);
            dpb_output_delay_length_minus1 = ReadBits(gb, 5);
            time_offset_length = ReadBits(gb, 5);
        }


        if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
            low_delay_hrd_flag = ReadBit(gb);
        }

        pic_struct_present_flag = ReadBit(gb);
        bitstream_restriction_flag = ReadBit(gb);

        if (bitstream_restriction_flag) {
            motion_vectors_over_pic_boundaries_flag = ReadBit(gb);
            max_bytes_per_pic_denom = ReadUE(gb);
            max_bits_per_mb_denom = ReadUE(gb);
            log2_max_mv_length_horizontal = ReadUE(gb);
            log2_max_mv_length_vertical = ReadUE(gb);
            num_reorder_frames = ReadUE(gb);
            max_dec_frame_buffering = ReadUE(gb);
        }

    }

//--------------


    int Width = ((pic_width_in_mbs_minus1 + 1) * 16) - frame_crop_bottom_offset * 2 -
                frame_crop_top_offset * 2;
    int Height = ((2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16) -
                 (frame_crop_right_offset * 2) - (frame_crop_left_offset * 2);

    printf("\n\nWxH = %dx%d\n\n", Width, Height);
    printf("num_units_in_tick:%x\n", num_units_in_tick);
    printf("time_scale:%x\n", time_scale);
}


static int
remove_h264or5_emulation_bytes(unsigned char *to, unsigned int toMaxSize, unsigned char *from,
                               unsigned int fromSize) {
    unsigned toSize = 0;
    unsigned i = 0;
    while (i < fromSize && toSize + 1 < toMaxSize) {
        if (i + 2 < fromSize && from[i] == 0 && from[i + 1] == 0 && from[i + 2] == 3) {
            to[toSize] = to[toSize + 1] = 0;
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

static int
add_h264or5_emulation_bytes(unsigned char *to, unsigned int toMaxSize, unsigned char *from,
                            unsigned int fromSize) {
    unsigned toSize = 0;
    unsigned i = 0;
    while (i < fromSize && toSize + 1 < toMaxSize) {
        if (i + 2 < fromSize && from[i] == 0 && from[i + 1] == 0) {
            to[toSize] = to[toSize + 1] = 0;
            to[toSize + 2] = 3;
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


static int init_get_bits(GetBitContext *s, const uint8_t *buffer, int bit_size) {
    int buffer_size;
    int ret = 0;

    buffer_size = (bit_size + 7) >> 3;

    s->buffer = buffer;
    s->size_in_bits = bit_size;
    s->buffer_end = buffer + buffer_size;
    s->index = 0;

    return ret;
}

static int init_set_bits(SetBitContext *s, uint8_t *buffer, int bit_size) {
    int buffer_size;
    int ret = 0;

    buffer_size = (bit_size + 7) >> 3;

    s->buffer = buffer;
    s->size_in_bits = bit_size;
    s->buffer_end = buffer + buffer_size;
    s->index = 0;

    return ret;
}

void *h264_sps_parser_open() {
    H264SPSParser *h;
    h = (H264SPSParser *) malloc(sizeof(H264SPSParser));
    if (h == NULL)
        goto fail;
    h->get_size = 1024;
    h->set_size = 1024;
    h->get_buffer = (uint8_t *) malloc(h->get_size * sizeof(uint8_t));
    h->set_buffer = (uint8_t *) malloc(h->set_size * sizeof(uint8_t));
    if ((h->get_buffer == NULL) || (h->set_buffer == NULL))
        goto fail;

    return h;
    fail:
    if (h != NULL) {
        if (h->get_buffer != NULL)
            free(h->get_buffer);
        if (h->set_buffer != NULL)
            free(h->set_buffer);
        free(h);
    }

}

void h264_sps_parser_close(void *handle) {
    H264SPSParser *h = (H264SPSParser *) handle;
    if (h != NULL) {
        if (h->get_buffer != NULL)
            free(h->get_buffer);
        if (h->set_buffer != NULL)
            free(h->set_buffer);
        free(h);
    }
}

int
h264_sps_set_timing_flag(void *handle, unsigned char *to, unsigned int *toSize, unsigned char *from,
                         unsigned int fromSize, int frame_rate_index) {

    H264SPSParser *h = (H264SPSParser *) handle;
    int i = 0;

    for (i = 0; i < fromSize; i++) {
        if ((from[i] == 0x00) &&
            (from[i + 1] == 0x00) &&
            (from[i + 2] == 0x00) &&
            (from[i + 3] == 0x01)) {
            if (from[i + 4] == 0x67 || from[i + 4] == 0x27) {

                h->get_size = remove_h264or5_emulation_bytes(h->get_buffer, fromSize, from + i + 5,
                                                             fromSize - i - 5);

                init_get_bits(&h->gb, h->get_buffer, h->get_size * 8);
                init_set_bits(&h->sb, h->set_buffer, h->set_size * 8);

                int vui_parameters_present_flag = find_vui_parameters_present_flag(&h->gb);
                if (vui_parameters_present_flag) {
                    int timing_info_present_flag = find_timing_info_present_flag(&h->gb);
                    if (timing_info_present_flag == 0) {
                        printf("timing flag index: %d\n", h->gb.index);
                        //WriteBits(&h->sb, h->buffer, h->gb.index);
                        WriteBits2(&h->sb, h->get_buffer, 0, h->gb.index - 1);
                        //timing_info_present_flag	u(1)
                        WriteBit(&h->sb, 0x01);
                        //num_units_in_tick u(32)
//                        unsigned char num_units_in_tick[4] = {0x00, 0x00, 0x00, 0x01};
                        unsigned int num_units_in_tick = swap(
                                framerate_table[frame_rate_index].num_units_in_tick);
                        //unsigned int num_units_in_tick = 1;
                        WriteBits(&h->sb, &num_units_in_tick, 32);
                        //time_scale	(u32)
//                        unsigned char time_scale[4] = {0x00, 0x00, 0x00, 0x3C};
                        unsigned int time_scale = swap(
                                framerate_table[frame_rate_index].time_scale * 2);
                        WriteBits(&h->sb, &time_scale, 32);
                        //fixed_frame_rate_flag u(1)

                        unsigned char fixed_frame_rate_flag = framerate_table[frame_rate_index].fixed_frame_rate_flag;
                        WriteBit(&h->sb, fixed_frame_rate_flag);
                        //WriteBit(&h->sb, 0x00);

                        WriteBits2(&h->sb, h->get_buffer + h->gb.index / 8, (8 - (h->gb.index % 8)),
                                   h->get_size * 8 - h->gb.index);
                        WriteBitsEnd(&h->sb);
                        printf("h->sb.index:%d\n", h->sb.index);

                        memcpy(to, from, 5);
                        *toSize = 5;

                        *toSize += add_h264or5_emulation_bytes(to + 5, h->sb.index / 8 * 2,
                                                               h->sb.buffer, h->sb.index / 8);
                    } else {
                        memcpy(to, from, fromSize);
                        *toSize = fromSize;
                    }

                } else {
                    WriteBits2(&h->sb, h->get_buffer, 0, h->gb.index - 1);
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
                    //timing_info_present_flag	u(1)
                    WriteBit(&h->sb, 0x01);
                    //num_units_in_tick u(32)
//                    unsigned char num_units_in_tick[4] = {0x00, 0x00, 0x00, 0x01};
                    unsigned int num_units_in_tick = swap(
                            framerate_table[frame_rate_index].num_units_in_tick);
                    //unsigned int num_units_in_tick = 1;
                    WriteBits(&h->sb, &num_units_in_tick, 32);
                    //time_scale	(u32)
//                    unsigned char time_scale[4] = {0x00, 0x00, 0x00, 0x3C};
                    unsigned int time_scale = swap(
                            framerate_table[frame_rate_index].time_scale * 2);
                    WriteBits(&h->sb, &time_scale, 32);
                    //fixed_frame_rate_flag u(1)

                    unsigned char fixed_frame_rate_flag = framerate_table[frame_rate_index].fixed_frame_rate_flag;
                    WriteBit(&h->sb, fixed_frame_rate_flag);
                    //WriteBit(&h->sb, 0x00);

                    //nal_hrd_parameters_present_flag u(1)
                    WriteBit(&h->sb, 0x00);
                    //vcl_hrd_parameters_present_flag u(1)
                    WriteBit(&h->sb, 0x00);
                    //pic_struct_present_flag u(1)
                    WriteBit(&h->sb, 0x00);
                    //bitstream_restriction_flag u(1)
                    WriteBit(&h->sb, 0x00);

                    WriteBits2(&h->sb, h->get_buffer + h->gb.index / 8, (8 - (h->gb.index % 8)),
                               h->get_size * 8 - h->gb.index);
                    WriteBitsEnd(&h->sb);
                    printf("h->sb.index:%d\n", h->sb.index);

                    memcpy(to, from, 5);
                    *toSize = 5;
                    *toSize += add_h264or5_emulation_bytes(to + 5, h->sb.index / 8 * 2,
                                                           h->sb.buffer, h->sb.index / 8);
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

//int main() {
//    //unsigned char data[100] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xAC, 0xB4, 0x03, 0xC0, 0x11, 0x3F, 0x2C, 0xAC, 0x14, 0x18,
//    //        0x14, 0x1B, 0x42, 0x84, 0xD4, '\0'};
//    unsigned char data[100] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xAC, 0xB4, 0x03,
//                               0xC0, 0x11, 0x3F, 0x2C, 0xA4, 0x14, 0x08, 0x08, 0x1B, 0x42, 0x84,
//                               0xD4, '\0'};
//    unsigned int len = 23;
//
//    //unsigned char data[100] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x28, 0xAC, 0xB4, 0x03, 0xC0, 0x11, 0x3F, 0x2C, 0xAC, 0x14, 0x18,
//    //        0x15, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x1E, 0x0D, 0xA1, 0x42, 0x6A, 0x00, '\0'};
//    //unsigned int len = 34;
//    unsigned char data2[100] = {0};
//    unsigned int len2 = 0;
//
//    int i = 0;
//    for (i = 0; i < len; i++)
//        printf("%x ", data[i]);
//    printf("\n");
//    printf("Buffer size %d\n", len);
//
//
//    void *h = h264_sps_parser_open();
//
//    h264_sps_set_timing_flag(h, data2, &len2, data, len, FPS_30);
//
//    printf("len2:%d\n", len2);
//    for (i = 0; i < len2; i++)
//        printf("%x ", data2[i]);
//    printf("\n");
//
//    h264_sps_parser_close(h);
//
//    return 0;
//}
