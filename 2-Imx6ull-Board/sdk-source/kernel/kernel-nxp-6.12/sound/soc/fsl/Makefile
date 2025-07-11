# SPDX-License-Identifier: GPL-2.0
# P1022 DS Machine Support
snd-soc-p1022-ds-y := p1022_ds.o
obj-$(CONFIG_SND_SOC_P1022_DS) += snd-soc-p1022-ds.o

# P1022 RDK Machine Support
snd-soc-p1022-rdk-y := p1022_rdk.o
obj-$(CONFIG_SND_SOC_P1022_RDK) += snd-soc-p1022-rdk.o

# Freescale SSI/DMA/SAI/SPDIF Support
snd-soc-fsl-audmix-y := fsl_audmix.o
snd-soc-fsl-asoc-card-y := fsl-asoc-card.o
snd-soc-fsl-asrc-y := fsl_asrc.o fsl_asrc_dma.o fsl_asrc_m2m.o
snd-soc-fsl-lpc3xxx-y := lpc3xxx-pcm.o lpc3xxx-i2s.o
snd-soc-fsl-sai-y := fsl_sai.o fsl_sai_sysfs.o
snd-soc-fsl-ssi-y := fsl_ssi.o
snd-soc-fsl-ssi-$(CONFIG_DEBUG_FS) += fsl_ssi_dbg.o
snd-soc-fsl-spdif-y := fsl_spdif.o
snd-soc-fsl-esai-y := fsl_esai.o
snd-soc-fsl-micfil-y := fsl_micfil.o
snd-soc-fsl-utils-y := fsl_utils.o
snd-soc-fsl-dma-y := fsl_dma.o
snd-soc-fsl-mqs-y := fsl_mqs.o
snd-soc-fsl-easrc-y := fsl_easrc.o
snd-soc-fsl-xcvr-y := fsl_xcvr.o
snd-soc-fsl-aud2htx-y := fsl_aud2htx.o
snd-soc-fsl-rpmsg-y := fsl_rpmsg.o
snd-soc-fsl-qmc-audio-y := fsl_qmc_audio.o

obj-$(CONFIG_SND_SOC_FSL_AUDMIX) += snd-soc-fsl-audmix.o
obj-$(CONFIG_SND_SOC_FSL_ASOC_CARD) += snd-soc-fsl-asoc-card.o
snd-soc-fsl-hdmi-y := fsl_hdmi.o
obj-$(CONFIG_SND_SOC_FSL_ASRC) += snd-soc-fsl-asrc.o
obj-$(CONFIG_SND_SOC_FSL_LPC3XXX) += snd-soc-fsl-lpc3xxx.o
obj-$(CONFIG_SND_SOC_FSL_SAI) += snd-soc-fsl-sai.o
obj-$(CONFIG_SND_SOC_FSL_SSI) += snd-soc-fsl-ssi.o
obj-$(CONFIG_SND_SOC_FSL_SPDIF) += snd-soc-fsl-spdif.o
obj-$(CONFIG_SND_SOC_FSL_ESAI) += snd-soc-fsl-esai.o
obj-$(CONFIG_SND_SOC_FSL_MICFIL) += snd-soc-fsl-micfil.o
obj-$(CONFIG_SND_SOC_FSL_UTILS) += snd-soc-fsl-utils.o
obj-$(CONFIG_SND_SOC_FSL_MQS) += snd-soc-fsl-mqs.o
obj-$(CONFIG_SND_SOC_FSL_EASRC) += snd-soc-fsl-easrc.o
obj-$(CONFIG_SND_SOC_FSL_HDMI) += snd-soc-fsl-hdmi.o
obj-$(CONFIG_SND_SOC_POWERPC_DMA) += snd-soc-fsl-dma.o
obj-$(CONFIG_SND_SOC_FSL_XCVR) += snd-soc-fsl-xcvr.o
obj-$(CONFIG_SND_SOC_FSL_AUD2HTX) += snd-soc-fsl-aud2htx.o
obj-$(CONFIG_SND_SOC_FSL_RPMSG) += snd-soc-fsl-rpmsg.o
obj-$(CONFIG_SND_SOC_POWERPC_QMC_AUDIO) += snd-soc-fsl-qmc-audio.o

# MPC5200 Platform Support
obj-$(CONFIG_SND_MPC52xx_DMA) += mpc5200_dma.o
obj-$(CONFIG_SND_SOC_MPC5200_I2S) += mpc5200_psc_i2s.o
obj-$(CONFIG_SND_SOC_MPC5200_AC97) += mpc5200_psc_ac97.o

# MPC5200 Machine Support
obj-$(CONFIG_SND_MPC52xx_SOC_PCM030) += pcm030-audio-fabric.o
obj-$(CONFIG_SND_MPC52xx_SOC_EFIKA) += efika-audio-fabric.o

# i.MX Platform Support
snd-soc-imx-audmux-y := imx-audmux.o
obj-$(CONFIG_SND_SOC_IMX_AUDMUX) += snd-soc-imx-audmux.o

obj-$(CONFIG_SND_SOC_IMX_PCM_FIQ) += imx-pcm-fiq.o
obj-$(CONFIG_SND_SOC_IMX_PCM_DMA) += imx-pcm-dma.o
obj-$(CONFIG_SND_SOC_IMX_AUDIO_RPMSG) += imx-audio-rpmsg.o
obj-$(CONFIG_SND_SOC_IMX_PCM_RPMSG) += imx-pcm-rpmsg.o
obj-$(CONFIG_SND_SOC_IMX_HDMI_DMA) += imx-hdmi-dma.o hdmi_pcm.o

# i.MX Machine Support
snd-soc-eukrea-tlv320-y := eukrea-tlv320.o
snd-soc-imx-es8328-y := imx-es8328.o
snd-soc-imx-sgtl5000-y := imx-sgtl5000.o
snd-soc-imx-audmix-y := imx-audmix.o
snd-soc-imx-hdmi-y := imx-hdmi.o
snd-soc-imx-rpmsg-y := imx-rpmsg.o
snd-soc-imx-card-y := imx-card.o
snd-soc-imx-pcm512x-y:= imx-pcm512x.o
snd-soc-imx6qdl-hdmi-y := imx6qdl-hdmi.o

obj-$(CONFIG_SND_SOC_EUKREA_TLV320) += snd-soc-eukrea-tlv320.o
obj-$(CONFIG_SND_SOC_IMX_ES8328) += snd-soc-imx-es8328.o
obj-$(CONFIG_SND_SOC_IMX_SGTL5000) += snd-soc-imx-sgtl5000.o
obj-$(CONFIG_SND_SOC_IMX_AUDMIX) += snd-soc-imx-audmix.o
obj-$(CONFIG_SND_SOC_IMX_HDMI) += snd-soc-imx-hdmi.o
obj-$(CONFIG_SND_SOC_IMX_RPMSG) += snd-soc-imx-rpmsg.o
obj-$(CONFIG_SND_SOC_IMX_CARD) += snd-soc-imx-card.o
obj-$(CONFIG_SND_SOC_IMX_PCM512X) += snd-soc-imx-pcm512x.o
obj-$(CONFIG_SND_SOC_IMX6QDL_HDMI) += snd-soc-imx6qdl-hdmi.o

AFLAGS_hdmi_pcm.o := -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp
