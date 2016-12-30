/*
 *
 * Copyright (c) 2016 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
/*
 * sunxi mipi protocol low-level interface
 * Author:raymonxiu
 */

#include "protocol_reg_i.h"
#include "protocol.h"
#include "protocol_reg.h"

MIPI_CSI2_CTL_t *mipi_csi2_ctl[MAX_MIPI_PTL];
MIPI_CSI2_CFG_t *mipi_csi2_cfg[MAX_MIPI_PTL];
MIPI_CSI2_VCDT_RX_t *mipi_csi2_vcdt_rx[MAX_MIPI_PTL];
MIPI_CSI2_RX_PKT_NUM_t *mipi_csi2_rx_pkt_num[MAX_MIPI_PTL];

MIPI_CSI2_CH_CFG_t *mipi_csi2_ch_cfg[MAX_MIPI_PTL];
MIPI_CSI2_CH_INT_EN_t *mipi_csi2_ch_int_en[MAX_MIPI_PTL];
MIPI_CSI2_CH_INT_PD_t *mipi_csi2_ch_int_pd[MAX_MIPI_PTL];
MIPI_CSI2_CH_DT_TRM_t *mipi_csi2_ch_dt_trm[MAX_MIPI_PTL];
MIPI_CSI2_CH_CUR_PH_t *mipi_csi2_ch_cur_ph[MAX_MIPI_PTL];
MIPI_CSI2_CH_ECC_t *mipi_csi2_ch_ecc[MAX_MIPI_PTL];
MIPI_CSI2_CH_CKS_t *mipi_csi2_ch_cks[MAX_MIPI_PTL];
MIPI_CSI2_CH_FRAME_NUM_t *mipi_csi2_ch_frame_num[MAX_MIPI_PTL];
MIPI_CSI2_CH_LINE_NUM_t *mipi_csi2_ch_line_num[MAX_MIPI_PTL];

int ptcl_reg_map(unsigned int sel, unsigned long addr_base)
{
	if (sel > MAX_MIPI_PTL)
		return -1;

	mipi_csi2_ctl[sel] =
	    (MIPI_CSI2_CTL_t *) (addr_base + MIPI_CSI2_CTL_OFF);
	mipi_csi2_cfg[sel] =
	    (MIPI_CSI2_CFG_t *) (addr_base + MIPI_CSI2_CFG_OFF);
	mipi_csi2_vcdt_rx[sel] =
	    (MIPI_CSI2_VCDT_RX_t *) (addr_base + MIPI_CSI2_VCDT_RX_OFF);
	mipi_csi2_rx_pkt_num[sel] =
	    (MIPI_CSI2_RX_PKT_NUM_t *) (addr_base + MIPI_CSI2_RX_PKT_NUM_OFF);
	mipi_csi2_ch_cfg[sel] =
	    (MIPI_CSI2_CH_CFG_t *) (addr_base + MIPI_CSI2_CH_CFG_OFF);
	mipi_csi2_ch_int_en[sel] =
	    (MIPI_CSI2_CH_INT_EN_t *) (addr_base + MIPI_CSI2_CH_INT_EN_OFF);
	mipi_csi2_ch_int_pd[sel] =
	    (MIPI_CSI2_CH_INT_PD_t *) (addr_base + MIPI_CSI2_CH_INT_PD_OFF);
	mipi_csi2_ch_dt_trm[sel] =
	    (MIPI_CSI2_CH_DT_TRM_t *) (addr_base + MIPI_CSI2_CH_DT_TRM_OFF);
	mipi_csi2_ch_cur_ph[sel] =
	    (MIPI_CSI2_CH_CUR_PH_t *) (addr_base + MIPI_CSI2_CH_CUR_PH_OFF);
	mipi_csi2_ch_ecc[sel] =
	    (MIPI_CSI2_CH_ECC_t *) (addr_base + MIPI_CSI2_CH_ECC_OFF);
	mipi_csi2_ch_cks[sel] =
	    (MIPI_CSI2_CH_CKS_t *) (addr_base + MIPI_CSI2_CH_CKS_OFF);
	mipi_csi2_ch_frame_num[sel] =
	    (MIPI_CSI2_CH_FRAME_NUM_t *) (addr_base +
					  MIPI_CSI2_CH_FRAME_NUM_OFF);
	mipi_csi2_ch_line_num[sel] =
	    (MIPI_CSI2_CH_LINE_NUM_t *) (addr_base + MIPI_CSI2_CH_LINE_NUM_OFF);

	return 0;
}

void ptcl_enable(unsigned int sel)
{
	mipi_csi2_ctl[sel]->bits.rst = 1;
	mipi_csi2_ctl[sel]->bits.en = 1;
}

void ptcl_disable(unsigned int sel)
{
	mipi_csi2_ctl[sel]->bits.en = 0;
	mipi_csi2_ctl[sel]->bits.rst = 0;
}

void ptcl_set_data_lane(unsigned int sel, unsigned char lane_num)
{
	mipi_csi2_cfg[sel]->bits.dl_cfg = lane_num - 1;
}

unsigned char ptcl_get_data_lane(unsigned int sel)
{
	return mipi_csi2_cfg[sel]->bits.dl_cfg + 1;
}

void ptcl_set_pl_bit_order(unsigned int sel, enum bit_order pl_bit_ord)
{
	mipi_csi2_cfg[sel]->bits.pl_bit_ord = pl_bit_ord;
}

enum bit_order ptcl_get_pl_bit_order(unsigned int sel)
{
	return (enum bit_order)mipi_csi2_cfg[sel]->bits.pl_bit_ord;
}

void ptcl_set_ph_bit_order(unsigned int sel, enum bit_order ph_bit_ord)
{
	mipi_csi2_cfg[sel]->bits.ph_bit_ord = ph_bit_ord;
}

enum bit_order ptcl_get_ph_bit_order(unsigned int sel)
{
	return (enum bit_order)mipi_csi2_cfg[sel]->bits.ph_bit_ord;
}

void ptcl_set_ph_byte_order(unsigned int sel, enum byte_order ph_byte_order)
{
	mipi_csi2_cfg[sel]->bits.ph_byte_ord = ph_byte_order;
}

enum byte_order ptcl_get_ph_byte_order(unsigned int sel)
{
	return (enum byte_order)mipi_csi2_cfg[sel]->bits.ph_byte_ord;
}

void ptcl_set_total_ch(unsigned int sel, unsigned char ch_num)
{
	mipi_csi2_cfg[sel]->bits.ch_mode = ch_num - 1;
}

unsigned char ptcl_get_total_ch(unsigned int sel)
{
	return mipi_csi2_cfg[sel]->bits.ch_mode;
}

void ptcl_set_vc(unsigned int sel, unsigned char ch, unsigned char vc)
{
	switch (ch) {
	case 0:
		mipi_csi2_vcdt_rx[sel]->bits.ch0_vc = vc;
		break;
	case 1:
		mipi_csi2_vcdt_rx[sel]->bits.ch1_vc = vc;
		break;
	case 2:
		mipi_csi2_vcdt_rx[sel]->bits.ch2_vc = vc;
		break;
	case 3:
		mipi_csi2_vcdt_rx[sel]->bits.ch3_vc = vc;
		break;
	default:
		break;
	}
}

unsigned char ptcl_get_vc(unsigned int sel, unsigned char ch)
{
	switch (ch) {
	case 0:
		return mipi_csi2_vcdt_rx[sel]->bits.ch0_vc;
	case 1:
		return mipi_csi2_vcdt_rx[sel]->bits.ch1_vc;
	case 2:
		return mipi_csi2_vcdt_rx[sel]->bits.ch2_vc;
	case 3:
		return mipi_csi2_vcdt_rx[sel]->bits.ch2_vc;
	default:
		return -1;
	}
}

void ptcl_set_dt(unsigned int sel, unsigned char ch, enum pkt_fmt dt)
{
	switch (ch) {
	case 0:
		mipi_csi2_vcdt_rx[sel]->bits.ch0_dt = dt;
		break;
	case 1:
		mipi_csi2_vcdt_rx[sel]->bits.ch1_dt = dt;
		break;
	case 2:
		mipi_csi2_vcdt_rx[sel]->bits.ch2_dt = dt;
		break;
	case 3:
		mipi_csi2_vcdt_rx[sel]->bits.ch3_dt = dt;
		break;
	default:
		break;
	}
}

enum pkt_fmt ptcl_get_dt(unsigned int sel, unsigned char ch)
{
	switch (ch) {
	case 0:
		return (enum pkt_fmt)mipi_csi2_vcdt_rx[sel]->bits.ch0_dt;
	case 1:
		return (enum pkt_fmt)mipi_csi2_vcdt_rx[sel]->bits.ch1_dt;
	case 2:
		return (enum pkt_fmt)mipi_csi2_vcdt_rx[sel]->bits.ch2_dt;
	case 3:
		return (enum pkt_fmt)mipi_csi2_vcdt_rx[sel]->bits.ch3_dt;
	default:
		return -1;
	}
}

void ptcl_set_src_type(unsigned int sel, unsigned char ch,
		       enum source_type src_type)
{
	(mipi_csi2_ch_cfg[sel] + ch * MIPI_CSI2_CH_OFF)->bits.src_sel =
	    src_type;
}

enum source_type ptcl_get_src_type(unsigned int sel, unsigned char ch)
{
	return (enum byte_order)(mipi_csi2_ch_cfg[sel] +
				 ch * MIPI_CSI2_CH_OFF)->bits.src_sel;
}

void ptcl_set_line_sync(unsigned int sel, unsigned char ch,
			enum line_sync ls_mode)
{
	(mipi_csi2_ch_cfg[sel] + ch * MIPI_CSI2_CH_OFF)->bits.line_sync =
	    ls_mode;
}

enum line_sync ptcl_get_line_sync(unsigned int sel, unsigned char ch)
{
	return (enum line_sync)(mipi_csi2_ch_cfg[sel] +
				ch * MIPI_CSI2_CH_OFF)->bits.line_sync;
}

void ptcl_int_enable(unsigned int sel, unsigned char ch,
		     enum protocol_int int_flag)
{
	switch (int_flag) {
	case FIFO_OVER_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.fifo_over_int = 1;
		break;
	case FRAME_END_SYNC_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.frame_end_sync_int = 1;
		break;
	case FRAME_START_SYNC_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.frame_start_sync_int = 1;
		break;
	case LINE_END_SYNC_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.line_end_sync_int = 1;
		break;
	case LINE_START_SYNC_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.line_start_sync_int = 1;
		break;
	case PH_UPDATE_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.ph_update_int = 1;
		break;
	case PF_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.pf_int = 1;
		break;
	case EMB_DATA_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.emb_data_int = 1;
		break;
	case FRAME_SYNC_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.frame_sync_err_int = 1;
		break;
	case LINE_SYNC_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.line_sync_err_int = 1;
		break;
	case ECC_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.ecc_err_int = 1;
		break;
	case ECC_WRN_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.ecc_wrn_int = 1;
		break;
	case CHKSUM_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.chksum_err_int = 1;
		break;
	case EOT_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.eot_err_int = 1;
		break;
	default:
		break;
	}
}

void ptcl_int_disable(unsigned int sel, unsigned char ch,
		      enum protocol_int int_flag)
{
	switch (int_flag) {
	case FIFO_OVER_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.fifo_over_int = 0;
		break;
	case FRAME_END_SYNC_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.frame_end_sync_int = 0;
		break;
	case FRAME_START_SYNC_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.frame_start_sync_int = 0;
		break;
	case LINE_END_SYNC_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.line_end_sync_int = 0;
		break;
	case LINE_START_SYNC_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.line_start_sync_int = 0;
		break;
	case PH_UPDATE_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.ph_update_int = 0;
		break;
	case PF_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.pf_int = 0;
		break;
	case EMB_DATA_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.emb_data_int = 0;
		break;
	case FRAME_SYNC_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.frame_sync_err_int = 0;
		break;
	case LINE_SYNC_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.line_sync_err_int = 0;
		break;
	case ECC_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.ecc_err_int = 0;
		break;
	case ECC_WRN_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.ecc_wrn_int = 0;
		break;
	case CHKSUM_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.chksum_err_int = 0;
		break;
	case EOT_ERR_INT:
		(mipi_csi2_ch_int_en[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.eot_err_int = 0;
		break;
	default:
		break;
	}
}

void ptcl_clear_int_status(unsigned int sel, unsigned char ch,
			   enum protocol_int int_flag)
{
	switch (int_flag) {
	case FIFO_OVER_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.fifo_over_pd = 1;
		break;
	case FRAME_END_SYNC_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.frame_end_sync_pd = 1;
		break;
	case FRAME_START_SYNC_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.frame_start_sync_pd = 1;
		break;
	case LINE_END_SYNC_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.line_end_sync_pd = 1;
		break;
	case LINE_START_SYNC_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.line_start_sync_pd = 1;
		break;
	case PH_UPDATE_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.ph_update_pd = 1;
		break;
	case PF_INT:
		(mipi_csi2_ch_int_pd[sel] + ch * MIPI_CSI2_CH_OFF)->bits.pf_pd =
		    1;
		break;
	case EMB_DATA_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.emb_data_pd = 1;
		break;
	case FRAME_SYNC_ERR_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.frame_sync_err_pd = 1;
		break;
	case LINE_SYNC_ERR_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.line_sync_err_pd = 1;
		break;
	case ECC_ERR_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.ecc_err_pd = 1;
		break;
	case ECC_WRN_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.ecc_wrn_pd = 1;
		break;
	case CHKSUM_ERR_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.chksum_err_pd = 1;
		break;
	case EOT_ERR_INT:
		(mipi_csi2_ch_int_pd[sel] +
		 ch * MIPI_CSI2_CH_OFF)->bits.eot_err_pd = 1;
		break;
	default:
		break;
	}
}

unsigned char ptcl_get_int_status(unsigned int sel, unsigned char ch,
				  enum protocol_int int_flag)
{
	switch (int_flag) {
	case FIFO_OVER_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.fifo_over_pd;
	case FRAME_END_SYNC_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.frame_end_sync_pd;
	case FRAME_START_SYNC_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.frame_start_sync_pd;
	case LINE_END_SYNC_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.line_end_sync_pd;
	case LINE_START_SYNC_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.line_start_sync_pd;
	case PH_UPDATE_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.ph_update_pd;
	case PF_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.pf_pd;
	case EMB_DATA_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.emb_data_pd;
	case FRAME_SYNC_ERR_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.frame_sync_err_pd;
	case LINE_SYNC_ERR_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.line_sync_err_pd;
	case ECC_ERR_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.ecc_err_pd;
	case ECC_WRN_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.ecc_wrn_pd;
	case CHKSUM_ERR_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.chksum_err_pd;
	case EOT_ERR_INT:
		return (mipi_csi2_ch_int_pd[sel] +
			ch * MIPI_CSI2_CH_OFF)->bits.eot_err_pd;
	default:
		return -1;
	}
}
