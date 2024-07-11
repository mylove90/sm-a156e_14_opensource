// SPDX-License-Identifier: GPL-2.0
/*
 *  mt6835-mt6368.c  --  mt6835 mt6368 ALSA SoC machine driver
 *
 *  Copyright (c) 2021 MediaTek Inc.
 *  Author: Shu-wei Hsu <shu-wei.Hsu@mediatek.com>
 */

#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "../common/mtk-afe-platform-driver.h"
#include "mt6835-afe-common.h"
#include "mt6835-afe-clk.h"
#include "mt6835-afe-gpio.h"
#include "../../codecs/mt6368.h"
#if IS_ENABLED(CONFIG_SND_SOC_MT6368_ACCDET)
#include "../../codecs/mt6368-accdet.h"
#endif
#include "../common/mtk-sp-spk-amp.h"
/*
 * if need additional control for the ext spk amp that is connected
 * after Lineout Buffer / HP Buffer on the codec, put the control in
 * mt6835_mt6368_spk_amp_event()
 */
#define EXT_SPK_AMP_W_NAME "Ext_Speaker_Amp"

static const char *const mt6835_spk_type_str[] = {MTK_SPK_NOT_SMARTPA_STR,
						  MTK_SPK_RICHTEK_RT5509_STR,
						  MTK_SPK_MEDIATEK_MT6660_STR,
						  MTK_SPK_RICHTEK_RT5512_STR,
						  MTK_SPK_GOODIX_TFA98XX_STR};
static const char *const
	mt6835_spk_i2s_type_str[] = {MTK_SPK_I2S_0_STR,
				     MTK_SPK_I2S_1_STR,
				     MTK_SPK_I2S_2_STR,
				     MTK_SPK_I2S_3_STR,
				     MTK_SPK_I2S_5_STR,
				     MTK_SPK_I2S_6_STR,
				     MTK_SPK_I2S_7_STR,
				     MTK_SPK_I2S_8_STR,
				     MTK_SPK_I2S_9_STR,
				     MTK_SPK_TINYCONN_I2S_0_STR};

static const struct soc_enum mt6835_spk_type_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mt6835_spk_type_str),
			    mt6835_spk_type_str),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mt6835_spk_i2s_type_str),
			    mt6835_spk_i2s_type_str),
};

static int mt6835_spk_type_get(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	int idx = mtk_spk_get_type();

	pr_debug("%s() = %d\n", __func__, idx);
	ucontrol->value.integer.value[0] = idx;
	return 0;
}

static int mt6835_spk_i2s_out_type_get(struct snd_kcontrol *kcontrol,
				       struct snd_ctl_elem_value *ucontrol)
{
	int idx = mtk_spk_get_i2s_out_type();

	pr_debug("%s() = %d\n", __func__, idx);
	ucontrol->value.integer.value[0] = idx;
	return 0;
}

static int mt6835_spk_i2s_in_type_get(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_value *ucontrol)
{
	int idx = mtk_spk_get_i2s_in_type();

	pr_debug("%s() = %d\n", __func__, idx);
	ucontrol->value.integer.value[0] = idx;
	return 0;
}

static int mt6835_mt6368_spk_amp_event(struct snd_soc_dapm_widget *w,
					struct snd_kcontrol *kcontrol,
					int event)
{
	struct snd_soc_dapm_context *dapm = w->dapm;
	struct snd_soc_card *card = dapm->card;

	dev_info(card->dev, "%s(), event %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/* spk amp on control */
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/* spk amp off control */
		break;
	default:
		break;
	}

	return 0;
};

static const struct snd_soc_dapm_widget mt6835_mt6368_widgets[] = {
	SND_SOC_DAPM_SPK(EXT_SPK_AMP_W_NAME, mt6835_mt6368_spk_amp_event),
};

static const struct snd_soc_dapm_route mt6835_mt6368_routes[] = {
	{EXT_SPK_AMP_W_NAME, NULL, "LINEOUT L"},
	{EXT_SPK_AMP_W_NAME, NULL, "Headphone L Ext Spk Amp"},
	{EXT_SPK_AMP_W_NAME, NULL, "Headphone R Ext Spk Amp"},
};

static const struct snd_kcontrol_new mt6835_mt6368_controls[] = {
	SOC_DAPM_PIN_SWITCH(EXT_SPK_AMP_W_NAME),
	SOC_ENUM_EXT("MTK_SPK_TYPE_GET", mt6835_spk_type_enum[0],
		     mt6835_spk_type_get, NULL),
	SOC_ENUM_EXT("MTK_SPK_I2S_OUT_TYPE_GET", mt6835_spk_type_enum[1],
		     mt6835_spk_i2s_out_type_get, NULL),
	SOC_ENUM_EXT("MTK_SPK_I2S_IN_TYPE_GET", mt6835_spk_type_enum[1],
		     mt6835_spk_i2s_in_type_get, NULL),
};

/*
 * define mtk_spk_i2s_mck node in dts when need mclk,
 * BE i2s need assign snd_soc_ops = mt6835_mt6368_i2s_ops
 */
static int mt6835_mt6368_i2s_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned int rate = params_rate(params);
	unsigned int mclk_fs_ratio = 128;
	unsigned int mclk_fs = rate * mclk_fs_ratio;
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);

	return snd_soc_dai_set_sysclk(cpu_dai,
				      0, mclk_fs, SND_SOC_CLOCK_OUT);
}

static const struct snd_soc_ops mt6835_mt6368_i2s_ops = {
	.hw_params = mt6835_mt6368_i2s_hw_params,
};

static int mt6835_mt6368_mtkaif_calibration(struct snd_soc_pcm_runtime *rtd)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, AFE_PCM_NAME);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);
	struct mt6835_afe_private *afe_priv = afe->platform_priv;
	struct snd_soc_component *codec_component =
		snd_soc_rtdcom_lookup(rtd, CODEC_MT6368_NAME);
	int phase;
	unsigned int monitor = 0;
	int test_done_1, test_done_2;
	int miso0_need_calib, miso1_need_calib;
	int cycle_1, cycle_2;
	int prev_cycle_1, prev_cycle_2;
	int counter;
	int mtkaif_calib_ok;

	dev_info(afe->dev, "%s(), start\n", __func__);

	pm_runtime_get_sync(afe->dev);

	miso0_need_calib = mt6835_afe_gpio_is_prepared(MT6835_AFE_GPIO_DAT_MISO0_ON);
	miso1_need_calib = mt6835_afe_gpio_is_prepared(MT6835_AFE_GPIO_DAT_MISO1_ON);

	mt6835_afe_gpio_request(afe, true, MT6835_DAI_ADDA, 1);
	mt6835_afe_gpio_request(afe, true, MT6835_DAI_ADDA, 0);

	mt6368_mtkaif_calibration_enable(codec_component);

	/* set clock protocol 2 */
	regmap_update_bits(afe->regmap, AFE_AUD_PAD_TOP, 0xff, 0xb8);
	regmap_update_bits(afe->regmap, AFE_AUD_PAD_TOP, 0xff, 0xb9);

	/* set test type to synchronizer pulse */
	regmap_update_bits(afe_priv->topckgen,
			   CKSYS_AUD_TOP_CFG, 0xffff, 0x4);

	mtkaif_calib_ok = true;
	afe_priv->mtkaif_calibration_num_phase = 42;	/* mt6359: 0 ~ 42 */
	afe_priv->mtkaif_chosen_phase[0] = -1;
	afe_priv->mtkaif_chosen_phase[1] = -1;
	afe_priv->mtkaif_chosen_phase[2] = -1;

	for (phase = 0;
	     phase <= afe_priv->mtkaif_calibration_num_phase &&
	     mtkaif_calib_ok;
	     phase++) {
		mt6368_set_mtkaif_calibration_phase(codec_component,
						    phase, phase, phase);

		regmap_update_bits(afe_priv->topckgen,
				   CKSYS_AUD_TOP_CFG, 0x1, 0x1);

		test_done_1 = miso0_need_calib ? 0 : -1;
		test_done_2 = miso1_need_calib ? 0 : -1;
		cycle_1 = -1;
		cycle_2 = -1;
		counter = 0;
		while (test_done_1 == 0 ||
		       test_done_2 == 0) {
			regmap_read(afe_priv->topckgen,
				    CKSYS_AUD_TOP_MON, &monitor);

			/* get test status */
			if (test_done_1 == 0)
				test_done_1 = (monitor >> 28) & 0x1;
			if (test_done_2 == 0)
				test_done_2 = (monitor >> 29) & 0x1;

			/* get delay cycle */
			if (test_done_1 == 1)
				cycle_1 = monitor & 0xf;
			if (test_done_2 == 1)
				cycle_2 = (monitor >> 4) & 0xf;

			/* handle if never test done */
			if (++counter > 10000) {
				dev_err(afe->dev, "%s(), test fail, cycle_1 %d, cycle_2 %d, monitor 0x%x\n",
					__func__,
					cycle_1, cycle_2, monitor);
				mtkaif_calib_ok = false;
				break;
			}
		}

		if (phase == 0) {
			prev_cycle_1 = cycle_1;
			prev_cycle_2 = cycle_2;
		}

		if (miso0_need_calib &&
		    cycle_1 != prev_cycle_1 &&
		    afe_priv->mtkaif_chosen_phase[0] < 0) {
			afe_priv->mtkaif_chosen_phase[0] = phase - 1;
			afe_priv->mtkaif_phase_cycle[0] = prev_cycle_1;
		}

		if (miso1_need_calib &&
		    cycle_2 != prev_cycle_2 &&
		    afe_priv->mtkaif_chosen_phase[1] < 0) {
			afe_priv->mtkaif_chosen_phase[1] = phase - 1;
			afe_priv->mtkaif_phase_cycle[1] = prev_cycle_2;
		}

		regmap_update_bits(afe_priv->topckgen,
				   CKSYS_AUD_TOP_CFG, 0x1, 0x0);
	}

	mt6368_set_mtkaif_calibration_phase(codec_component,
		(afe_priv->mtkaif_chosen_phase[0] < 0) ?
		0 : afe_priv->mtkaif_chosen_phase[0],
		(afe_priv->mtkaif_chosen_phase[1] < 0) ?
		0 : afe_priv->mtkaif_chosen_phase[1],
		(afe_priv->mtkaif_chosen_phase[2] < 0) ?
		0 : afe_priv->mtkaif_chosen_phase[2]);

	/* disable rx fifo */
	regmap_update_bits(afe->regmap, AFE_AUD_PAD_TOP, 0xff, 0xb8);

	mt6368_mtkaif_calibration_disable(codec_component);

	mt6835_afe_gpio_request(afe, false, MT6835_DAI_ADDA, 1);
	mt6835_afe_gpio_request(afe, false, MT6835_DAI_ADDA, 0);

	/* disable syncword if miso pin not prepared */
	if (!miso0_need_calib)
		regmap_update_bits(afe->regmap, AFE_ADDA_MTKAIF_SYNCWORD_CFG,
				   RG_ADDA_MTKAIF_RX_SYNC_WORD1_DISABLE_MASK_SFT,
				   0x1 << RG_ADDA_MTKAIF_RX_SYNC_WORD1_DISABLE_SFT);
	if (!miso1_need_calib)
		regmap_update_bits(afe->regmap, AFE_ADDA_MTKAIF_SYNCWORD_CFG,
				   RG_ADDA_MTKAIF_RX_SYNC_WORD2_DISABLE_MASK_SFT,
				   0x1 << RG_ADDA_MTKAIF_RX_SYNC_WORD2_DISABLE_SFT);

	pm_runtime_put(afe->dev);

	dev_info(afe->dev, "%s(), mtkaif_chosen_phase[0/1/2]:%d/%d/%d, miso_need_calib[%d/%d/%d]\n",
		 __func__,
		 afe_priv->mtkaif_chosen_phase[0],
		 afe_priv->mtkaif_chosen_phase[1],
		 miso0_need_calib, miso1_need_calib);
#endif
	return 0;
}

static int mt6835_mt6368_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component =
		snd_soc_rtdcom_lookup(rtd, AFE_PCM_NAME);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);
	struct mt6835_afe_private *afe_priv = afe->platform_priv;
	struct snd_soc_component *codec_component =
		snd_soc_rtdcom_lookup(rtd, CODEC_MT6368_NAME);
	struct snd_soc_dapm_context *dapm = &rtd->card->dapm;
	struct mt6368_codec_ops ops;

	/* set dc component callback function for codec */
	ops.enable_dc_compensation = mt6835_enable_dc_compensation;
	ops.set_lch_dc_compensation = mt6835_set_lch_dc_compensation;
	ops.set_rch_dc_compensation = mt6835_set_rch_dc_compensation;
	ops.adda_dl_gain_control = mt6835_adda_dl_gain_control;
	mt6368_set_codec_ops(codec_component, &ops);

	/* set mtkaif protocol */
	mt6368_set_mtkaif_protocol(codec_component,
				   MTKAIF_PROTOCOL_2_CLK_P2);
	afe_priv->mtkaif_protocol = MTKAIF_PROTOCOL_2_CLK_P2;

	/* mtkaif calibration */
	mt6835_mt6368_mtkaif_calibration(rtd);

	/* disable ext amp connection */
	snd_soc_dapm_disable_pin(dapm, EXT_SPK_AMP_W_NAME);
#if IS_ENABLED(CONFIG_SND_SOC_MT6368_ACCDET)
	mt6368_accdet_init(codec_component, rtd->card);
#endif

	return 0;
}

static int mt6835_i2s_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				      struct snd_pcm_hw_params *params)
{
	dev_info(rtd->dev, "%s(), fix format to 32bit\n", __func__);

	/* fix BE i2s format to 32bit, clean param mask first */
	snd_mask_reset_range(hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT),
			     0, SNDRV_PCM_FORMAT_LAST);

	params_set_format(params, SNDRV_PCM_FORMAT_S32_LE);
	return 0;
}

#if IS_ENABLED(CONFIG_MTK_VOW_SUPPORT) && !defined(SKIP_SB)
static const struct snd_pcm_hardware mt6835_mt6368_vow_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_MMAP_VALID),
	.period_bytes_min = 256,
	.period_bytes_max = 2 * 1024,
	.periods_min = 2,
	.periods_max = 4,
	.buffer_bytes_max = 2 * 2 * 1024,
};

static int mt6835_mt6368_vow_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
			snd_soc_rtdcom_lookup(rtd, AFE_PCM_NAME);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);
	int i;

	dev_info(afe->dev, "%s(), start\n", __func__);
	snd_soc_set_runtime_hwparams(substream, &mt6835_mt6368_vow_hardware);

	mt6835_afe_gpio_request(afe, true, MT6835_DAI_VOW, 0);

	/* ASoC will call pm_runtime_get, but vow don't need */
	for_each_rtd_components(rtd, i, component) {
		pm_runtime_put_autosuspend(component->dev);
	}

	return 0;
}

static void mt6835_mt6368_vow_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_component *component =
			snd_soc_rtdcom_lookup(rtd, AFE_PCM_NAME);
	struct mtk_base_afe *afe = snd_soc_component_get_drvdata(component);
	int i;

	dev_info(afe->dev, "%s(), end\n", __func__);
	mt6835_afe_gpio_request(afe, false, MT6835_DAI_VOW, 0);

	/* restore to fool ASoC */
	for_each_rtd_components(rtd, i, component) {
		pm_runtime_get_sync(component->dev);
	}
}

static const struct snd_soc_ops mt6835_mt6368_vow_ops = {
	.startup = mt6835_mt6368_vow_startup,
	.shutdown = mt6835_mt6368_vow_shutdown,
};
#endif  // #if IS_ENABLED(CONFIG_MTK_VOW_SUPPORT)

/* FE */
SND_SOC_DAILINK_DEFS(playback1,
	DAILINK_COMP_ARRAY(COMP_CPU("DL1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(playback12,
	DAILINK_COMP_ARRAY(COMP_CPU("DL12")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(playback2,
	DAILINK_COMP_ARRAY(COMP_CPU("DL2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(playback3,
	DAILINK_COMP_ARRAY(COMP_CPU("DL3")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(playback4,
	DAILINK_COMP_ARRAY(COMP_CPU("DL4")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(playback5,
	DAILINK_COMP_ARRAY(COMP_CPU("DL5")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(playback6,
	DAILINK_COMP_ARRAY(COMP_CPU("DL6")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(playback7,
	DAILINK_COMP_ARRAY(COMP_CPU("DL7")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(playback8,
	DAILINK_COMP_ARRAY(COMP_CPU("DL8")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(playback9,
	DAILINK_COMP_ARRAY(COMP_CPU("DL9")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture1,
	DAILINK_COMP_ARRAY(COMP_CPU("UL1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture2,
	DAILINK_COMP_ARRAY(COMP_CPU("UL2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture3,
	DAILINK_COMP_ARRAY(COMP_CPU("UL3")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture4,
	DAILINK_COMP_ARRAY(COMP_CPU("UL4")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture5,
	DAILINK_COMP_ARRAY(COMP_CPU("UL5")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture6,
	DAILINK_COMP_ARRAY(COMP_CPU("UL6")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture7,
	DAILINK_COMP_ARRAY(COMP_CPU("UL7")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture8,
	DAILINK_COMP_ARRAY(COMP_CPU("UL8")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture9,
	DAILINK_COMP_ARRAY(COMP_CPU("UL9")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture_mono_1,
	DAILINK_COMP_ARRAY(COMP_CPU("UL_MONO_1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture_mono_2,
	DAILINK_COMP_ARRAY(COMP_CPU("UL_MONO_2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(capture_mono_3,
	DAILINK_COMP_ARRAY(COMP_CPU("UL_MONO_3")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

/* hostless */
SND_SOC_DAILINK_DEFS(hostless_lpbk,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless LPBK DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_fm,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless FM DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_fm_record,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_FM_Record DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_adda_dl_hwgain,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_ADDA_DL_HWGain DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_speech,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless Speech DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_sph_echo_ref,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_Sph_Echo_Ref_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_spk_init,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_Spk_Init_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_adda_dl_i2s_out,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_ADDA_DL_I2S_OUT DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_src1,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_SRC_1_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_src2,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_SRC_2_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_src3,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_SRC_3_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_hw_src_1_out,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_HW_SRC_1_OUT_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_hw_src_1_in,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_HW_SRC_1_IN_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_hw_src_2_out,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_HW_SRC_2_OUT_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_hw_src_2_in,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_HW_SRC_2_IN_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_hw_src_3_out,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_HW_SRC_3_OUT_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_hw_src_3_in,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_HW_SRC_3_IN_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_src_bargein,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_SRC_Bargein_DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

/* BE */
SND_SOC_DAILINK_DEFS(adda,
	DAILINK_COMP_ARRAY(COMP_CPU("ADDA")),
	DAILINK_COMP_ARRAY(COMP_CODEC(DEVICE_MT6368_NAME,
				      "mt6368-snd-codec-aif1")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(ap_dmic,
	DAILINK_COMP_ARRAY(COMP_CPU("AP_DMIC")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(i2s0,
	DAILINK_COMP_ARRAY(COMP_CPU("I2S0")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(i2s1,
	DAILINK_COMP_ARRAY(COMP_CPU("I2S1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(i2s2,
	DAILINK_COMP_ARRAY(COMP_CPU("I2S2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(i2s3,
	DAILINK_COMP_ARRAY(COMP_CPU("I2S3")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(i2s5,
	DAILINK_COMP_ARRAY(COMP_CPU("I2S5")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hw_gain1,
	DAILINK_COMP_ARRAY(COMP_CPU("HW Gain 1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hw_gain2,
	DAILINK_COMP_ARRAY(COMP_CPU("HW Gain 2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hw_src1,
	DAILINK_COMP_ARRAY(COMP_CPU("HW_SRC_1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hw_src2,
	DAILINK_COMP_ARRAY(COMP_CPU("HW_SRC_2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hw_src3,
	DAILINK_COMP_ARRAY(COMP_CPU("HW_SRC_3")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(connsys_i2s,
	DAILINK_COMP_ARRAY(COMP_CPU("CONNSYS_I2S")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(pcm1,
	DAILINK_COMP_ARRAY(COMP_CPU("PCM 1")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(pcm2,
	DAILINK_COMP_ARRAY(COMP_CPU("PCM 2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

/* hostless */
SND_SOC_DAILINK_DEFS(hostless_ul1,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_UL1 DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_ul2,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_UL2 DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_ul3,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_UL3 DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_ul6,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless_UL6 DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_hw_gain_aaudio,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless HW Gain AAudio DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
SND_SOC_DAILINK_DEFS(hostless_src_aaudio,
	DAILINK_COMP_ARRAY(COMP_CPU("Hostless SRC AAudio DAI")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
#if IS_ENABLED(CONFIG_SND_SOC_MTK_BTCVSD)
SND_SOC_DAILINK_DEFS(btcvsd,
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("18050000.mtk-btcvsd-snd")));
#endif
#if IS_ENABLED(CONFIG_MTK_VOW_SUPPORT) && !defined(SKIP_SB)
SND_SOC_DAILINK_DEFS(vow,
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_CODEC(DEVICE_MT6368_NAME,
				      "mt6368-snd-codec-vow")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
#endif
#if IS_ENABLED(CONFIG_MTK_ULTRASND_PROXIMITY)
SND_SOC_DAILINK_DEFS(ultra,
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("snd-scp-ultra")));
#endif

#if IS_ENABLED(CONFIG_MTK_SCP_AUDIO)
SND_SOC_DAILINK_DEFS(scpspkprocess,
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_DUMMY()));
#endif

static struct snd_soc_dai_link mt6835_mt6368_dai_links[] = {
	/* Front End DAI links */
	{
		.name = "Playback_1",
		.stream_name = "Playback_1",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback1),
	},
	{
		.name = "Playback_12",
		.stream_name = "Playback_12",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback12),
	},
	{
		.name = "Playback_2",
		.stream_name = "Playback_2",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback2),
	},
	{
		.name = "Playback_3",
		.stream_name = "Playback_3",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback3),
	},
	{
		.name = "Playback_4",
		.stream_name = "Playback_4",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback4),
	},
	{
		.name = "Playback_5",
		.stream_name = "Playback_5",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback5),
	},
	{
		.name = "Playback_6",
		.stream_name = "Playback_6",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback6),
	},
	{
		.name = "Playback_7",
		.stream_name = "Playback_7",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback7),
	},
	{
		.name = "Playback_8",
		.stream_name = "Playback_8",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback8),
	},
	{
		.name = "Playback_9",
		.stream_name = "Playback_9",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		SND_SOC_DAILINK_REG(playback9),
	},
	{
		.name = "Capture_1",
		.stream_name = "Capture_1",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture1),
	},
	{
		.name = "Capture_2",
		.stream_name = "Capture_2",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture2),
	},
	{
		.name = "Capture_3",
		.stream_name = "Capture_3",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture3),
	},
	{
		.name = "Capture_4",
		.stream_name = "Capture_4",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture4),
	},
	{
		.name = "Capture_5",
		.stream_name = "Capture_5",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture5),
	},
	{
		.name = "Capture_6",
		.stream_name = "Capture_6",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture6),
	},
	{
		.name = "Capture_7",
		.stream_name = "Capture_7",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture7),
	},
	{
		.name = "Capture_8",
		.stream_name = "Capture_8",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture8),
	},
	{
		.name = "Capture_9",
		.stream_name = "Capture_9",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture9),
	},
	{
		.name = "Capture_Mono_1",
		.stream_name = "Capture_Mono_1",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture_mono_1),
	},
	{
		.name = "Capture_Mono_2",
		.stream_name = "Capture_Mono_2",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture_mono_2),
	},
	{
		.name = "Capture_Mono_3",
		.stream_name = "Capture_Mono_3",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		SND_SOC_DAILINK_REG(capture_mono_3),
	},
	{
		.name = "Hostless_LPBK",
		.stream_name = "Hostless_LPBK",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_lpbk),
	},
	{
		.name = "Hostless_FM",
		.stream_name = "Hostless_FM",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_fm),
	},
	{
		.name = "Hostless_FM_Record",
		.stream_name = "Hostless_FM_Record",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_fm_record),
	},
	{
		.name = "Hostless_ADDA_DL_HWGain",
		.stream_name = "Hostless_ADDA_DL_HWGain",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_adda_dl_hwgain),
	},
	{
		.name = "Hostless_Speech",
		.stream_name = "Hostless_Speech",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_speech),
	},
	{
		.name = "Hostless_Sph_Echo_Ref",
		.stream_name = "Hostless_Sph_Echo_Ref",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_sph_echo_ref),
	},
	{
		.name = "Hostless_Spk_Init",
		.stream_name = "Hostless_Spk_Init",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_spk_init),
	},
	{
		.name = "Hostless_ADDA_DL_I2S_OUT",
		.stream_name = "Hostless_ADDA_DL_I2S_OUT",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_adda_dl_i2s_out),
	},
	{
		.name = "Hostless_SRC_1",
		.stream_name = "Hostless_SRC_1",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_src1),
	},
	{
		.name = "Hostless_SRC_2",
		.stream_name = "Hostless_SRC_2",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_src2),
	},
	{
		.name = "Hostless_SRC_3",
		.stream_name = "Hostless_SRC_3",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_src3),
	},
	{
		.name = "Hostless_HW_SRC_1_OUT",
		.stream_name = "Hostless_HW_SRC_1_OUT",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_hw_src_1_out),
	},
	{
		.name = "Hostless_HW_SRC_1_IN",
		.stream_name = "Hostless_HW_SRC_1_IN",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_hw_src_1_in),
	},
	{
		.name = "Hostless_HW_SRC_2_OUT",
		.stream_name = "Hostless_HW_SRC_2_OUT",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_hw_src_2_out),
	},
	{
		.name = "Hostless_HW_SRC_2_IN",
		.stream_name = "Hostless_HW_SRC_2_IN",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_hw_src_2_in),
	},
	{
		.name = "Hostless_HW_SRC_3_OUT",
		.stream_name = "Hostless_HW_SRC_3_OUT",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_hw_src_3_out),
	},
	{
		.name = "Hostless_HW_SRC_3_IN",
		.stream_name = "Hostless_HW_SRC_3_IN",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_hw_src_3_in),
	},
	{
		.name = "Hostless_SRC_Bargein",
		.stream_name = "Hostless_SRC_Bargein",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_src_bargein),
	},
	/* Back End DAI links */
	{
		.name = "Primary Codec",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		.init = mt6835_mt6368_init,
		SND_SOC_DAILINK_REG(adda),
	},
	{
		.name = "AP_DMIC",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(ap_dmic),
	},
	{
		.name = "I2S0",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS
			| SND_SOC_DAIFMT_GATED,
		.ops = &mt6835_mt6368_i2s_ops,
		.no_pcm = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = mt6835_i2s_hw_params_fixup,
		SND_SOC_DAILINK_REG(i2s0),
	},
	{
		.name = "I2S1",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS
			| SND_SOC_DAIFMT_GATED,
		.ops = &mt6835_mt6368_i2s_ops,
		.no_pcm = 1,
		.dpcm_playback = 1,
		.ignore_suspend = 1,
		.be_hw_params_fixup = mt6835_i2s_hw_params_fixup,
		SND_SOC_DAILINK_REG(i2s1),
	},
	{
		.name = "I2S2",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS
			| SND_SOC_DAIFMT_GATED,
		.ops = &mt6835_mt6368_i2s_ops,
		.no_pcm = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		.be_hw_params_fixup = mt6835_i2s_hw_params_fixup,
		SND_SOC_DAILINK_REG(i2s2),
	},
	{
		.name = "I2S3",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS
			| SND_SOC_DAIFMT_GATED,
		.ops = &mt6835_mt6368_i2s_ops,
		.no_pcm = 1,
		.dpcm_playback = 1,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_hw_params_fixup = mt6835_i2s_hw_params_fixup,
		SND_SOC_DAILINK_REG(i2s3),
	},
	{
		.name = "I2S5",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS
			| SND_SOC_DAIFMT_GATED,
		.ops = &mt6835_mt6368_i2s_ops,
		.no_pcm = 1,
		.dpcm_playback = 1,
		.ignore_suspend = 1,
		.be_hw_params_fixup = mt6835_i2s_hw_params_fixup,
		SND_SOC_DAILINK_REG(i2s5),
	},
	{
		.name = "HW Gain 1",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hw_gain1),
	},
	{
		.name = "HW Gain 2",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hw_gain2),
	},
	{
		.name = "HW_SRC_1",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hw_src1),
	},
	{
		.name = "HW_SRC_2",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hw_src2),
	},
	{
		.name = "HW_SRC_3",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hw_src3),
	},
	{
		.name = "CONNSYS_I2S",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(connsys_i2s),
	},
	{
		.name = "PCM 1",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(pcm1),
	},
	{
		.name = "PCM 2",
		.no_pcm = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(pcm2),
	},
	/* dummy BE for ul memif to record from dl memif */
	{
		.name = "Hostless_UL1",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_ul1),
	},
	{
		.name = "Hostless_UL2",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_ul2),
	},
	{
		.name = "Hostless_UL3",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_ul3),
	},
	{
		.name = "Hostless_UL6",
		.no_pcm = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_ul6),
	},
	{
		.name = "Hostless_HW_Gain_AAudio",
		.stream_name = "Hostless_HW_Gain_AAudio",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_hw_gain_aaudio),
	},
	{
		.name = "Hostless_SRC_AAudio",
		.stream_name = "Hostless_SRC_AAudio",
		.trigger = {SND_SOC_DPCM_TRIGGER_PRE,
			    SND_SOC_DPCM_TRIGGER_PRE},
		.dynamic = 1,
		.dpcm_playback = 1,
		.dpcm_capture = 1,
		.ignore_suspend = 1,
		SND_SOC_DAILINK_REG(hostless_src_aaudio),
	},
	/* BTCVSD */
#if IS_ENABLED(CONFIG_SND_SOC_MTK_BTCVSD)
	{
		.name = "BTCVSD",
		.stream_name = "BTCVSD",
		SND_SOC_DAILINK_REG(btcvsd),
	},
#endif
	/* VoW */
#if IS_ENABLED(CONFIG_MTK_VOW_SUPPORT) && !defined(SKIP_SB)
	{
		.name = "VOW_Capture",
		.stream_name = "VOW_Capture",
		.ignore_suspend = 1,
		.ops = &mt6835_mt6368_vow_ops,
		SND_SOC_DAILINK_REG(vow),
	},
#endif
#if IS_ENABLED(CONFIG_MTK_ULTRASND_PROXIMITY)
	{
		.name = "SCP_ULTRA_Playback",
		.stream_name = "SCP_ULTRA_Playback",
		SND_SOC_DAILINK_REG(ultra),
	},
#endif
#if IS_ENABLED(CONFIG_MTK_SCP_AUDIO)
	{
		.name = "SCP_SPK_Process",
		.stream_name = "SCP_SPK_Process",
		SND_SOC_DAILINK_REG(scpspkprocess),
	},
#endif
};

static struct snd_soc_card mt6835_mt6368_soc_card = {
	.name = "mt6835-mt6368",
	.owner = THIS_MODULE,
	.dai_link = mt6835_mt6368_dai_links,
	.num_links = ARRAY_SIZE(mt6835_mt6368_dai_links),

	.controls = mt6835_mt6368_controls,
	.num_controls = ARRAY_SIZE(mt6835_mt6368_controls),
	.dapm_widgets = mt6835_mt6368_widgets,
	.num_dapm_widgets = ARRAY_SIZE(mt6835_mt6368_widgets),
	.dapm_routes = mt6835_mt6368_routes,
	.num_dapm_routes = ARRAY_SIZE(mt6835_mt6368_routes),
};

#if IS_ENABLED(CONFIG_MTK_SCP_AUDIO)
int mtk_update_scp_audio_info(struct snd_soc_card *card,
			struct platform_device *pdev)
{
	struct snd_soc_dai_link *dai_link;
	int i = 0;

	/* find dai link of SCP_SPK_Process */
	for_each_card_prelinks(card, i, dai_link) {
		if (strcmp(dai_link->name, "SCP_SPK_Process") == 0) {
			dai_link->cpus->name = NULL;
			dai_link->cpus->dai_name = "audio_task_spk_process";
			dai_link->platforms->name = "snd_scp_audio";
			dai_link->platforms->dai_name = NULL;
			dev_info(&pdev->dev, "scp audio updated\n");
		}
	}

	return 0;
}
#endif

static int mt6835_mt6368_dev_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &mt6835_mt6368_soc_card;
	struct device_node *platform_node, *spk_node;
	int ret, i;
	struct snd_soc_dai_link *dai_link;
#if IS_ENABLED(CONFIG_MTK_SCP_AUDIO)
	struct device_node *scp_audio_node;
#endif

	dev_info(&pdev->dev, "%s()\n", __func__);

	/* update speaker type */
	ret = mtk_spk_update_info(card, pdev);
	if (ret) {
		dev_err(&pdev->dev, "%s(), mtk_spk_update_info error\n",
			__func__);
		return -EINVAL;
	}

	/* get platform node */
	platform_node = of_parse_phandle(pdev->dev.of_node,
					 "mediatek,platform", 0);
	if (!platform_node) {
		dev_err(&pdev->dev, "Property 'platform' missing or invalid\n");
		return -EINVAL;
	}

	/* get speaker codec node */
	spk_node = of_get_child_by_name(pdev->dev.of_node,
					"mediatek,speaker-codec");
	if (!spk_node) {
		dev_err(&pdev->dev,
			"spk_node of_get_child_by_name fail\n");
		//return -EINVAL;
	}

	for_each_card_prelinks(card, i, dai_link) {
		if (!dai_link->platforms->name)
			dai_link->platforms->of_node = platform_node;

		if (!strcmp(dai_link->name, "Speaker Codec")) {
			ret = snd_soc_of_get_dai_link_codecs(
						&pdev->dev, spk_node, dai_link);
			if (ret < 0) {
				dev_err(&pdev->dev,
					"Speaker Codec get_dai_link fail: %d\n", ret);
				return -EINVAL;
			}
		} else if (!strcmp(dai_link->name, "Speaker Codec Ref")) {
			ret = snd_soc_of_get_dai_link_codecs(
						&pdev->dev, spk_node, dai_link);
			if (ret < 0) {
				dev_err(&pdev->dev,
					"Speaker Codec Ref get_dai_link fail: %d\n", ret);
				return -EINVAL;
			}
		}
	}

#if IS_ENABLED(CONFIG_MTK_SCP_AUDIO)
	/* get scp audio node */
	scp_audio_node = of_parse_phandle(pdev->dev.of_node,
					 "mediatek,scp-audio", 0);
	if (scp_audio_node) {
		dev_err(&pdev->dev, "got scp audio node\n");
		mtk_update_scp_audio_info(card, pdev);
	} else {
		dev_err(&pdev->dev, "can't find scp audio node\n");
	}
#endif

	card->dev = &pdev->dev;
	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret)
		dev_err(&pdev->dev, "%s snd_soc_register_card fail %d\n",
			__func__, ret);
	else
		dev_err(&pdev->dev, "%s snd_soc_register_card pss %d\n",
				__func__, ret);
	return ret;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id mt6835_mt6368_dt_match[] = {
	{.compatible = "mediatek,mt6835-mt6368-sound",},
	{}
};
#endif

static const struct dev_pm_ops mt6835_mt6368_pm_ops = {
	.poweroff = snd_soc_poweroff,
	.restore = snd_soc_resume,
};

static struct platform_driver mt6835_mt6368_driver = {
	.driver = {
		.name = "mt6835-mt6368",
#if IS_ENABLED(CONFIG_OF)
		.of_match_table = mt6835_mt6368_dt_match,
#endif
		.pm = &mt6835_mt6368_pm_ops,
	},
	.probe = mt6835_mt6368_dev_probe,
};

module_platform_driver(mt6835_mt6368_driver);

/* Module information */
MODULE_DESCRIPTION("MT6835 mt6368 ALSA SoC machine driver");
MODULE_AUTHOR("Shane Chien <shane.chien@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("mt6835 mt6368 soc card");
