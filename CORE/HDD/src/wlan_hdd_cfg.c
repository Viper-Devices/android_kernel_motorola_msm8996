/*
 * Copyright (c) 2012-2014 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

/**=========================================================================

                       EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header:$   $DateTime: $ $Author: $


  when        who    what, where, why
  --------    ---    --------------------------------------------------------
  07/27/09    kanand Created module.

  ==========================================================================*/

/*--------------------------------------------------------------------------
  Include Files
  ------------------------------------------------------------------------*/


#include <linux/firmware.h>
#include <linux/string.h>
#include <wlan_hdd_includes.h>
#include <wlan_hdd_main.h>
#include <wlan_hdd_assoc.h>
#include <wlan_hdd_cfg.h>
#include <linux/string.h>
#include <vos_types.h>
#include <csrApi.h>
#include <pmcApi.h>
#include <wlan_hdd_misc.h>

#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
static void
cbNotifySetRoamPrefer5GHz(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    sme_UpdateRoamPrefer5GHz(pHddCtx->hHal, pHddCtx->cfg_ini->nRoamPrefer5GHz);
}

static void
cbNotifySetImmediateRoamRssiDiff(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    sme_UpdateImmediateRoamRssiDiff(pHddCtx->hHal,
                                    pHddCtx->cfg_ini->nImmediateRoamRssiDiff,
                                    0);
}

static void
cbNotifySetRoamRssiDiff(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    sme_UpdateRoamRssiDiff(pHddCtx->hHal,
                           0,
                           pHddCtx->cfg_ini->RoamRssiDiff);
}

static void
cbNotifySetFastTransitionEnabled(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    sme_UpdateFastTransitionEnabled(pHddCtx->hHal,
                                    pHddCtx->cfg_ini->isFastTransitionEnabled);
}

static void
cbNotifySetRoamIntraBand(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    sme_setRoamIntraBand(pHddCtx->hHal, pHddCtx->cfg_ini->nRoamIntraBand);
}

static void
cbNotifySetWESMode(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_UpdateWESMode(pHddCtx->hHal,
                      pHddCtx->cfg_ini->isWESModeEnabled,
                      0);
}

static void
cbNotifySetRoamScanNProbes(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    sme_UpdateRoamScanNProbes(pHddCtx->hHal, 0,
                              pHddCtx->cfg_ini->nProbes);
}

static void
cbNotifySetRoamScanHomeAwayTime(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
     sme_UpdateRoamScanHomeAwayTime(pHddCtx->hHal, 0,
                                    pHddCtx->cfg_ini->nRoamScanHomeAwayTime,
                                    eANI_BOOLEAN_TRUE);
}
#endif

#ifdef FEATURE_WLAN_OKC
static void
cbNotifySetOkcFeatureEnabled(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
}
#endif

#ifdef FEATURE_WLAN_LFR
static void
NotifyIsFastRoamIniFeatureEnabled(hdd_context_t *pHddCtx,
                                  unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_UpdateIsFastRoamIniFeatureEnabled(pHddCtx->hHal, 0,
                               pHddCtx->cfg_ini->isFastRoamIniFeatureEnabled );
}

static void
NotifyIsMAWCIniFeatureEnabled(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_UpdateIsMAWCIniFeatureEnabled(pHddCtx->hHal,
                                      pHddCtx->cfg_ini->MAWCEnabled );
}
#endif

#ifdef FEATURE_WLAN_ESE
static void
cbNotifySetEseFeatureEnabled(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_UpdateIsEseFeatureEnabled(pHddCtx->hHal, 0,
                                  pHddCtx->cfg_ini->isEseIniFeatureEnabled );
}
#endif

static void
cbNotifySetFwRssiMonitoring(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_UpdateConfigFwRssiMonitoring(pHddCtx->hHal,
                                     pHddCtx->cfg_ini->fEnableFwRssiMonitoring);
}

#ifdef WLAN_FEATURE_NEIGHBOR_ROAMING
static void cbNotifySetOpportunisticScanThresholdDiff(hdd_context_t *pHddCtx,
                                                      unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_SetRoamOpportunisticScanThresholdDiff(pHddCtx->hHal, 0,
                          pHddCtx->cfg_ini->nOpportunisticThresholdDiff );
}

static void cbNotifySetRoamRescanRssiDiff(hdd_context_t *pHddCtx,
                                          unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_SetRoamRescanRssiDiff(pHddCtx->hHal,
                              0,
                              pHddCtx->cfg_ini->nRoamRescanRssiDiff);
}

static void
cbNotifySetNeighborLookupRssiThreshold(hdd_context_t *pHddCtx,
                                       unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_setNeighborLookupRssiThreshold(pHddCtx->hHal, 0,
                               pHddCtx->cfg_ini->nNeighborLookupRssiThreshold);
}

static void
cbNotifySetNeighborScanPeriod(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_setNeighborScanPeriod(pHddCtx->hHal, 0,
                              pHddCtx->cfg_ini->nNeighborScanPeriod );
}

static void
cbNotifySetNeighborResultsRefreshPeriod(hdd_context_t *pHddCtx,
                                        unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_setNeighborScanRefreshPeriod(pHddCtx->hHal, 0,
                               pHddCtx->cfg_ini->nNeighborResultsRefreshPeriod);
}

static void
cbNotifySetEmptyScanRefreshPeriod(hdd_context_t *pHddCtx,
                                  unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_UpdateEmptyScanRefreshPeriod(pHddCtx->hHal, 0,
                                     pHddCtx->cfg_ini->nEmptyScanRefreshPeriod);
}

static void
cbNotifySetNeighborScanMinChanTime(hdd_context_t *pHddCtx,
                                   unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_setNeighborScanMinChanTime(pHddCtx->hHal,
                                   pHddCtx->cfg_ini->nNeighborScanMinChanTime,
                                   0);
}

static void
cbNotifySetNeighborScanMaxChanTime(hdd_context_t *pHddCtx,
                                   unsigned long NotifyId)
{
    sme_setNeighborScanMaxChanTime(pHddCtx->hHal, 0,
                                   pHddCtx->cfg_ini->nNeighborScanMaxChanTime);
}
static void cbNotifySetRoamBmissFirstBcnt(hdd_context_t *pHddCtx,
                                          unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_SetRoamBmissFirstBcnt(pHddCtx->hHal,
                              0,
                              pHddCtx->cfg_ini->nRoamBmissFirstBcnt);
}

static void cbNotifySetRoamBmissFinalBcnt(hdd_context_t *pHddCtx,
                                          unsigned long NotifyId)
{
    sme_SetRoamBmissFinalBcnt(pHddCtx->hHal, 0,
                              pHddCtx->cfg_ini->nRoamBmissFinalBcnt);
}

static void cbNotifySetRoamBeaconRssiWeight(hdd_context_t *pHddCtx,
                                          unsigned long NotifyId)
{
    sme_SetRoamBeaconRssiWeight(pHddCtx->hHal, 0,
                              pHddCtx->cfg_ini->nRoamBeaconRssiWeight);
}

static void
cbNotifySetDFSScanMode(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    /* At the point this routine is called, the value in the cfg_ini
       table has already been updated */
    sme_UpdateDFSScanMode(pHddCtx->hHal, 0,
                          pHddCtx->cfg_ini->allowDFSChannelRoam);
}

#endif

static void cbNotifySetEnableSSR(hdd_context_t *pHddCtx, unsigned long NotifyId)
{
    sme_UpdateEnableSSR(pHddCtx->hHal, pHddCtx->cfg_ini->enableSSR);
}


static void cbNotify_set_gSapPreferredChanLocation(hdd_context_t *pHddCtx,
                                                   unsigned long NotifyId)
{
    WLANSAP_set_Dfs_Preferred_Channel_location(pHddCtx->hHal,
                              pHddCtx->cfg_ini->gSapPreferredChanLocation);
}


static void chNotify_set_gDisableDfsJapanW53(hdd_context_t *pHddCtx,
                                             unsigned long NotifyId)
{
    WLANSAP_set_Dfs_Restrict_JapanW53(pHddCtx->hHal,
                              pHddCtx->cfg_ini->gDisableDfsJapanW53);
}

#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
static void
cbNotifyUpdateRoamScanOffloadEnabled(hdd_context_t *pHddCtx,
                                     unsigned long NotifyId)
{
    sme_UpdateRoamScanOffloadEnabled(pHddCtx->hHal,
                                    pHddCtx->cfg_ini->isRoamOffloadScanEnabled);
    if (0 == pHddCtx->cfg_ini->isRoamOffloadScanEnabled) {
        pHddCtx->cfg_ini->bFastRoamInConIniFeatureEnabled = 0;
        sme_UpdateEnableFastRoamInConcurrency(pHddCtx->hHal,
                             pHddCtx->cfg_ini->bFastRoamInConIniFeatureEnabled);
    }
}

static void
cbNotifySetEnableFastRoamInConcurrency(hdd_context_t *pHddCtx,
                                       unsigned long NotifyId)
{
    sme_UpdateEnableFastRoamInConcurrency(pHddCtx->hHal,
                             pHddCtx->cfg_ini->bFastRoamInConIniFeatureEnabled);
}

#endif

REG_TABLE_ENTRY g_registry_table[] =
{
   REG_VARIABLE( CFG_RTS_THRESHOLD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, RTSThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RTS_THRESHOLD_DEFAULT,
                 CFG_RTS_THRESHOLD_MIN,
                 CFG_RTS_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_FRAG_THRESHOLD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, FragmentationThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_FRAG_THRESHOLD_DEFAULT,
                 CFG_FRAG_THRESHOLD_MIN,
                 CFG_FRAG_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_OPERATING_CHANNEL_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, OperatingChannel,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_OPERATING_CHANNEL_DEFAULT,
                 CFG_OPERATING_CHANNEL_MIN,
                 CFG_OPERATING_CHANNEL_MAX ),

   REG_VARIABLE( CFG_SHORT_SLOT_TIME_ENABLED_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ShortSlotTimeEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SHORT_SLOT_TIME_ENABLED_DEFAULT,
                 CFG_SHORT_SLOT_TIME_ENABLED_MIN,
                 CFG_SHORT_SLOT_TIME_ENABLED_MAX ),

   REG_VARIABLE( CFG_11D_SUPPORT_ENABLED_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, Is11dSupportEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_11D_SUPPORT_ENABLED_DEFAULT,
                 CFG_11D_SUPPORT_ENABLED_MIN,
                 CFG_11D_SUPPORT_ENABLED_MAX ),

   REG_VARIABLE( CFG_11H_SUPPORT_ENABLED_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, Is11hSupportEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_11H_SUPPORT_ENABLED_DEFAULT,
                 CFG_11H_SUPPORT_ENABLED_MIN,
                 CFG_11H_SUPPORT_ENABLED_MAX ),

   REG_VARIABLE( CFG_ENFORCE_11D_CHANNELS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fEnforce11dChannels,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_ENFORCE_11D_CHANNELS_DEFAULT,
                 CFG_ENFORCE_11D_CHANNELS_MIN,
                 CFG_ENFORCE_11D_CHANNELS_MAX ),

   REG_VARIABLE( CFG_COUNTRY_CODE_PRIORITY_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fSupplicantCountryCodeHasPriority,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_COUNTRY_CODE_PRIORITY_DEFAULT,
                 CFG_COUNTRY_CODE_PRIORITY_MIN,
                 CFG_COUNTRY_CODE_PRIORITY_MAX),

   REG_VARIABLE( CFG_ENFORCE_COUNTRY_CODE_MATCH_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fEnforceCountryCodeMatch,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_ENFORCE_COUNTRY_CODE_MATCH_DEFAULT,
                 CFG_ENFORCE_COUNTRY_CODE_MATCH_MIN,
                 CFG_ENFORCE_COUNTRY_CODE_MATCH_MAX ),

   REG_VARIABLE( CFG_ENFORCE_DEFAULT_DOMAIN_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fEnforceDefaultDomain,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_ENFORCE_DEFAULT_DOMAIN_DEFAULT,
                 CFG_ENFORCE_DEFAULT_DOMAIN_MIN,
                 CFG_ENFORCE_DEFAULT_DOMAIN_MAX ),

   REG_VARIABLE( CFG_HEARTBEAT_THRESH_24_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, HeartbeatThresh24,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_HEARTBEAT_THRESH_24_DEFAULT,
                 CFG_HEARTBEAT_THRESH_24_MIN,
                 CFG_HEARTBEAT_THRESH_24_MAX ),

   REG_VARIABLE_STRING( CFG_POWER_USAGE_NAME, WLAN_PARAM_String,
                        hdd_config_t, PowerUsageControl,
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_POWER_USAGE_DEFAULT ),

   REG_VARIABLE( CFG_ENABLE_SUSPEND_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nEnableSuspend,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_SUSPEND_DEFAULT,
                 CFG_ENABLE_SUSPEND_MIN,
                 CFG_ENABLE_SUSPEND_MAX ),

   REG_VARIABLE( CFG_ENABLE_ENABLE_DRIVER_STOP_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nEnableDriverStop,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_ENABLE_DRIVER_STOP_DEFAULT,
                 CFG_ENABLE_ENABLE_DRIVER_STOP_MIN,
                 CFG_ENABLE_ENABLE_DRIVER_STOP_MAX ),

   REG_VARIABLE( CFG_ENABLE_LOGP_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fIsLogpEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_LOGP_DEFAULT,
                 CFG_ENABLE_LOGP_MIN,
                 CFG_ENABLE_LOGP_MAX ),

   REG_VARIABLE( CFG_ENABLE_IMPS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fIsImpsEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_IMPS_DEFAULT,
                 CFG_ENABLE_IMPS_MIN,
                 CFG_ENABLE_IMPS_MAX ),

   REG_VARIABLE( CFG_IMPS_MINIMUM_SLEEP_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nImpsMinSleepTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IMPS_MINIMUM_SLEEP_TIME_DEFAULT,
                 CFG_IMPS_MINIMUM_SLEEP_TIME_MIN,
                 CFG_IMPS_MINIMUM_SLEEP_TIME_MAX ),

   REG_VARIABLE( CFG_IMPS_MAXIMUM_SLEEP_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nImpsMaxSleepTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IMPS_MAXIMUM_SLEEP_TIME_DEFAULT,
                 CFG_IMPS_MAXIMUM_SLEEP_TIME_MIN,
                 CFG_IMPS_MAXIMUM_SLEEP_TIME_MAX ),

   REG_VARIABLE( CFG_IMPS_MODERATE_SLEEP_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nImpsModSleepTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IMPS_MODERATE_SLEEP_TIME_DEFAULT,
                 CFG_IMPS_MODERATE_SLEEP_TIME_MIN,
                 CFG_IMPS_MODERATE_SLEEP_TIME_MAX ),

   REG_VARIABLE( CFG_ENABLE_BMPS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fIsBmpsEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_BMPS_DEFAULT,
                 CFG_ENABLE_BMPS_MIN,
                 CFG_ENABLE_BMPS_MAX ),

   REG_VARIABLE( CFG_BMPS_MINIMUM_LI_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nBmpsMinListenInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BMPS_MINIMUM_LI_DEFAULT,
                 CFG_BMPS_MINIMUM_LI_MIN,
                 CFG_BMPS_MINIMUM_LI_MAX ),

   REG_VARIABLE( CFG_BMPS_MAXIMUM_LI_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nBmpsMaxListenInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BMPS_MAXIMUM_LI_DEFAULT,
                 CFG_BMPS_MAXIMUM_LI_MIN,
                 CFG_BMPS_MAXIMUM_LI_MAX ),

   REG_VARIABLE( CFG_BMPS_MODERATE_LI_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nBmpsModListenInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BMPS_MODERATE_LI_DEFAULT,
                 CFG_BMPS_MODERATE_LI_MIN,
                 CFG_BMPS_MODERATE_LI_MAX ),

   REG_VARIABLE( CFG_ENABLE_AUTO_BMPS_TIMER_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fIsAutoBmpsTimerEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_AUTO_BMPS_TIMER_DEFAULT,
                 CFG_ENABLE_AUTO_BMPS_TIMER_MIN,
                 CFG_ENABLE_AUTO_BMPS_TIMER_MAX ),

   REG_VARIABLE( CFG_AUTO_BMPS_TIMER_VALUE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nAutoBmpsTimerValue,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AUTO_BMPS_TIMER_VALUE_DEFAULT,
                 CFG_AUTO_BMPS_TIMER_VALUE_MIN,
                 CFG_AUTO_BMPS_TIMER_VALUE_MAX ),

   REG_VARIABLE( CFG_DOT11_MODE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, dot11Mode,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_DOT11_MODE_DEFAULT,
                 CFG_DOT11_MODE_MIN,
                 CFG_DOT11_MODE_MAX ),

   REG_VARIABLE( CFG_SAP_FORCE_11AC_FOR_11N, WLAN_PARAM_Integer,
                 hdd_config_t, apForce11ACFor11n,
#ifndef WLAN_FEATURE_MBSSID
                 VAR_FLAGS_DYNAMIC_CFG |
#endif
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SAP_FORCE_11AC_FOR_11N_DEFAULT,
                 CFG_SAP_FORCE_11AC_FOR_11N_MIN,
                 CFG_SAP_FORCE_11AC_FOR_11N_MAX ),

   REG_VARIABLE( CFG_CHANNEL_BONDING_MODE_24GHZ_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nChannelBondingMode24GHz,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_CHANNEL_BONDING_MODE_DEFAULT,
                 CFG_CHANNEL_BONDING_MODE_MIN,
                 CFG_CHANNEL_BONDING_MODE_MAX),

   REG_VARIABLE( CFG_CHANNEL_BONDING_MODE_5GHZ_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nChannelBondingMode5GHz,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_CHANNEL_BONDING_MODE_DEFAULT,
                 CFG_CHANNEL_BONDING_MODE_MIN,
                 CFG_CHANNEL_BONDING_MODE_MAX),

   REG_VARIABLE( CFG_MAX_RX_AMPDU_FACTOR_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, MaxRxAmpduFactor,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK ,
                 CFG_MAX_RX_AMPDU_FACTOR_DEFAULT,
                 CFG_MAX_RX_AMPDU_FACTOR_MIN,
                 CFG_MAX_RX_AMPDU_FACTOR_MAX),

   REG_VARIABLE( CFG_FIXED_RATE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, TxRate,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_FIXED_RATE_DEFAULT,
                 CFG_FIXED_RATE_MIN,
                 CFG_FIXED_RATE_MAX ),

   REG_VARIABLE( CFG_SHORT_GI_20MHZ_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ShortGI20MhzEnable,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SHORT_GI_20MHZ_DEFAULT,
                 CFG_SHORT_GI_20MHZ_MIN,
                 CFG_SHORT_GI_20MHZ_MAX ),

   REG_VARIABLE( CFG_BLOCK_ACK_AUTO_SETUP_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, BlockAckAutoSetup,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_BLOCK_ACK_AUTO_SETUP_DEFAULT,
                 CFG_BLOCK_ACK_AUTO_SETUP_MIN,
                 CFG_BLOCK_ACK_AUTO_SETUP_MAX ),

   REG_VARIABLE( CFG_SCAN_RESULT_AGE_COUNT_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ScanResultAgeCount,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SCAN_RESULT_AGE_COUNT_DEFAULT,
                 CFG_SCAN_RESULT_AGE_COUNT_MIN,
                 CFG_SCAN_RESULT_AGE_COUNT_MAX ),

   REG_VARIABLE( CFG_SCAN_RESULT_AGE_TIME_NCNPS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nScanAgeTimeNCNPS,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SCAN_RESULT_AGE_TIME_NCNPS_DEFAULT,
                 CFG_SCAN_RESULT_AGE_TIME_NCNPS_MIN,
                 CFG_SCAN_RESULT_AGE_TIME_NCNPS_MAX ),

   REG_VARIABLE( CFG_SCAN_RESULT_AGE_TIME_NCPS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nScanAgeTimeNCPS,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SCAN_RESULT_AGE_TIME_NCPS_DEFAULT,
                 CFG_SCAN_RESULT_AGE_TIME_NCPS_MIN,
                 CFG_SCAN_RESULT_AGE_TIME_NCPS_MAX ),

   REG_VARIABLE( CFG_SCAN_RESULT_AGE_TIME_CNPS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nScanAgeTimeCNPS,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SCAN_RESULT_AGE_TIME_CNPS_DEFAULT,
                 CFG_SCAN_RESULT_AGE_TIME_CNPS_MIN,
                 CFG_SCAN_RESULT_AGE_TIME_CNPS_MAX ),

   REG_VARIABLE( CFG_SCAN_RESULT_AGE_TIME_CPS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nScanAgeTimeCPS,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SCAN_RESULT_AGE_TIME_CPS_DEFAULT,
                 CFG_SCAN_RESULT_AGE_TIME_CPS_MIN,
                 CFG_SCAN_RESULT_AGE_TIME_CPS_MAX ),

   REG_VARIABLE( CFG_RSSI_CATEGORY_GAP_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nRssiCatGap,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RSSI_CATEGORY_GAP_DEFAULT,
                 CFG_RSSI_CATEGORY_GAP_MIN,
                 CFG_RSSI_CATEGORY_GAP_MAX ),

   REG_VARIABLE( CFG_SHORT_PREAMBLE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fIsShortPreamble,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SHORT_PREAMBLE_DEFAULT,
                 CFG_SHORT_PREAMBLE_MIN,
                 CFG_SHORT_PREAMBLE_MAX ),

   REG_VARIABLE_STRING( CFG_IBSS_BSSID_NAME, WLAN_PARAM_MacAddr,
                        hdd_config_t, IbssBssid,
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_IBSS_BSSID_DEFAULT ),

   REG_VARIABLE_STRING( CFG_INTF0_MAC_ADDR_NAME, WLAN_PARAM_MacAddr,
                        hdd_config_t, intfMacAddr[0],
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_INTF0_MAC_ADDR_DEFAULT ),

   REG_VARIABLE_STRING( CFG_INTF1_MAC_ADDR_NAME, WLAN_PARAM_MacAddr,
                        hdd_config_t, intfMacAddr[1],
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_INTF1_MAC_ADDR_DEFAULT ),

   REG_VARIABLE_STRING( CFG_INTF2_MAC_ADDR_NAME, WLAN_PARAM_MacAddr,
                        hdd_config_t, intfMacAddr[2],
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_INTF2_MAC_ADDR_DEFAULT ),

   REG_VARIABLE_STRING( CFG_INTF3_MAC_ADDR_NAME, WLAN_PARAM_MacAddr,
                        hdd_config_t, intfMacAddr[3],
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_INTF3_MAC_ADDR_DEFAULT ),

   REG_VARIABLE( CFG_AP_QOS_UAPSD_MODE_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, apUapsdEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_QOS_UAPSD_MODE_DEFAULT,
                 CFG_AP_QOS_UAPSD_MODE_MIN,
                 CFG_AP_QOS_UAPSD_MODE_MAX ),

   REG_VARIABLE_STRING( CFG_AP_COUNTRY_CODE, WLAN_PARAM_String,
                        hdd_config_t, apCntryCode,
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_AP_COUNTRY_CODE_DEFAULT ),

   REG_VARIABLE( CFG_AP_ENABLE_RANDOM_BSSID_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, apRandomBssidEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_ENABLE_RANDOM_BSSID_DEFAULT,
                 CFG_AP_ENABLE_RANDOM_BSSID_MIN,
                 CFG_AP_ENABLE_RANDOM_BSSID_MAX ),

   REG_VARIABLE( CFG_AP_ENABLE_PROTECTION_MODE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, apProtEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_ENABLE_PROTECTION_MODE_DEFAULT,
                 CFG_AP_ENABLE_PROTECTION_MODE_MIN,
                 CFG_AP_ENABLE_PROTECTION_MODE_MAX ),

   REG_VARIABLE( CFG_AP_PROTECTION_MODE_NAME, WLAN_PARAM_HexInteger,
                 hdd_config_t, apProtection,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_PROTECTION_MODE_DEFAULT,
                 CFG_AP_PROTECTION_MODE_MIN,
                  CFG_AP_PROTECTION_MODE_MAX ),

   REG_VARIABLE( CFG_AP_OBSS_PROTECTION_MODE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, apOBSSProtEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_OBSS_PROTECTION_MODE_DEFAULT,
                 CFG_AP_OBSS_PROTECTION_MODE_MIN,
                 CFG_AP_OBSS_PROTECTION_MODE_MAX ),

   REG_VARIABLE( CFG_AP_STA_SECURITY_SEPERATION_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, apDisableIntraBssFwd,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_STA_SECURITY_SEPERATION_DEFAULT,
                 CFG_AP_STA_SECURITY_SEPERATION_MIN,
                 CFG_AP_STA_SECURITY_SEPERATION_MAX ),

   REG_VARIABLE( CFG_FRAMES_PROCESSING_TH_MODE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, MinFramesProcThres,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_FRAMES_PROCESSING_TH_DEFAULT,
                 CFG_FRAMES_PROCESSING_TH_MIN,
                 CFG_FRAMES_PROCESSING_TH_MAX ),

   REG_VARIABLE( CFG_SAP_CHANNEL_SELECT_START_CHANNEL , WLAN_PARAM_Integer,
                 hdd_config_t, apStartChannelNum,
#ifndef WLAN_FEATURE_MBSSID
                 VAR_FLAGS_DYNAMIC_CFG |
#endif
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SAP_CHANNEL_SELECT_START_CHANNEL_DEFAULT,
                 CFG_SAP_CHANNEL_SELECT_START_CHANNEL_MIN,
                 CFG_SAP_CHANNEL_SELECT_START_CHANNEL_MAX ),

   REG_VARIABLE( CFG_SAP_CHANNEL_SELECT_END_CHANNEL , WLAN_PARAM_Integer,
                 hdd_config_t, apEndChannelNum,
#ifndef WLAN_FEATURE_MBSSID
                 VAR_FLAGS_DYNAMIC_CFG |
#endif
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SAP_CHANNEL_SELECT_END_CHANNEL_DEFAULT,
                 CFG_SAP_CHANNEL_SELECT_END_CHANNEL_MIN,
                 CFG_SAP_CHANNEL_SELECT_END_CHANNEL_MAX ),

   REG_VARIABLE( CFG_SAP_CHANNEL_SELECT_OPERATING_BAND , WLAN_PARAM_Integer,
                 hdd_config_t, apOperatingBand,
#ifndef WLAN_FEATURE_MBSSID
                 VAR_FLAGS_DYNAMIC_CFG |
#endif
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SAP_CHANNEL_SELECT_OPERATING_BAND_DEFAULT,
                 CFG_SAP_CHANNEL_SELECT_OPERATING_BAND_MIN,
                 CFG_SAP_CHANNEL_SELECT_OPERATING_BAND_MAX ),

   REG_VARIABLE( CFG_SAP_AUTO_CHANNEL_SELECTION_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, apAutoChannelSelection,
#ifndef WLAN_FEATURE_MBSSID
                 VAR_FLAGS_DYNAMIC_CFG |
#endif
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SAP_AUTO_CHANNEL_SELECTION_DEFAULT,
                 CFG_SAP_AUTO_CHANNEL_SELECTION_MIN,
                 CFG_SAP_AUTO_CHANNEL_SELECTION_MAX ),

   REG_VARIABLE( CFG_ENABLE_LTE_COEX , WLAN_PARAM_Integer,
                 hdd_config_t, enableLTECoex,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_LTE_COEX_DEFAULT,
                 CFG_ENABLE_LTE_COEX_MIN,
                 CFG_ENABLE_LTE_COEX_MAX ),

   REG_VARIABLE( CFG_AP_KEEP_ALIVE_PERIOD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, apKeepAlivePeriod,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_KEEP_ALIVE_PERIOD_DEFAULT,
                 CFG_AP_KEEP_ALIVE_PERIOD_MIN,
                 CFG_AP_KEEP_ALIVE_PERIOD_MAX),

   REG_VARIABLE( CFG_GO_KEEP_ALIVE_PERIOD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, goKeepAlivePeriod,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_GO_KEEP_ALIVE_PERIOD_DEFAULT,
                 CFG_GO_KEEP_ALIVE_PERIOD_MIN,
                 CFG_GO_KEEP_ALIVE_PERIOD_MAX),

   REG_VARIABLE( CFG_AP_LINK_MONITOR_PERIOD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, apLinkMonitorPeriod,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_LINK_MONITOR_PERIOD_DEFAULT,
                 CFG_AP_LINK_MONITOR_PERIOD_MIN,
                 CFG_AP_LINK_MONITOR_PERIOD_MAX),

   REG_VARIABLE( CFG_GO_LINK_MONITOR_PERIOD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, goLinkMonitorPeriod,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_GO_LINK_MONITOR_PERIOD_DEFAULT,
                 CFG_GO_LINK_MONITOR_PERIOD_MIN,
                 CFG_GO_LINK_MONITOR_PERIOD_MAX),

   REG_VARIABLE( CFG_DISABLE_PACKET_FILTER , WLAN_PARAM_Integer,
                 hdd_config_t, disablePacketFilter,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_DISABLE_PACKET_FILTER_DEFAULT,
                 CFG_DISABLE_PACKET_FILTER_MIN,
                 CFG_DISABLE_PACKET_FILTER_MAX ),

   REG_VARIABLE( CFG_BEACON_INTERVAL_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nBeaconInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_BEACON_INTERVAL_DEFAULT,
                 CFG_BEACON_INTERVAL_MIN,
                 CFG_BEACON_INTERVAL_MAX ),

   REG_VARIABLE( CFG_ENABLE_IDLE_SCAN_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, nEnableIdleScan,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_IDLE_SCAN_DEFAULT,
                 CFG_ENABLE_IDLE_SCAN_MIN,
                 CFG_ENABLE_IDLE_SCAN_MAX ),

   REG_VARIABLE( CFG_ROAMING_TIME_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, nRoamingTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ROAMING_TIME_DEFAULT,
                 CFG_ROAMING_TIME_MIN,
                 CFG_ROAMING_TIME_MAX ),

   REG_VARIABLE( CFG_VCC_RSSI_TRIGGER_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, nVccRssiTrigger,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VCC_RSSI_TRIGGER_DEFAULT,
                 CFG_VCC_RSSI_TRIGGER_MIN,
                 CFG_VCC_RSSI_TRIGGER_MAX ),

   REG_VARIABLE( CFG_VCC_UL_MAC_LOSS_THRESH_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, nVccUlMacLossThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VCC_UL_MAC_LOSS_THRESH_DEFAULT,
                 CFG_VCC_UL_MAC_LOSS_THRESH_MIN,
                 CFG_VCC_UL_MAC_LOSS_THRESH_MAX ),

   REG_VARIABLE( CFG_PASSIVE_MAX_CHANNEL_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nPassiveMaxChnTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_PASSIVE_MAX_CHANNEL_TIME_DEFAULT,
                 CFG_PASSIVE_MAX_CHANNEL_TIME_MIN,
                 CFG_PASSIVE_MAX_CHANNEL_TIME_MAX ),

   REG_VARIABLE( CFG_PASSIVE_MIN_CHANNEL_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nPassiveMinChnTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_PASSIVE_MIN_CHANNEL_TIME_DEFAULT,
                 CFG_PASSIVE_MIN_CHANNEL_TIME_MIN,
                 CFG_PASSIVE_MIN_CHANNEL_TIME_MAX ),

   REG_VARIABLE( CFG_ACTIVE_MAX_CHANNEL_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nActiveMaxChnTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ACTIVE_MAX_CHANNEL_TIME_DEFAULT,
                 CFG_ACTIVE_MAX_CHANNEL_TIME_MIN,
                 CFG_ACTIVE_MAX_CHANNEL_TIME_MAX ),

   REG_VARIABLE( CFG_ACTIVE_MIN_CHANNEL_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nActiveMinChnTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ACTIVE_MIN_CHANNEL_TIME_DEFAULT,
                 CFG_ACTIVE_MIN_CHANNEL_TIME_MIN,
                 CFG_ACTIVE_MIN_CHANNEL_TIME_MAX ),

   REG_VARIABLE( CFG_ACTIVE_MAX_CHANNEL_TIME_BTC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nActiveMaxChnTimeBtc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ACTIVE_MAX_CHANNEL_TIME_BTC_DEFAULT,
                 CFG_ACTIVE_MAX_CHANNEL_TIME_BTC_MIN,
                 CFG_ACTIVE_MAX_CHANNEL_TIME_BTC_MAX ),

   REG_VARIABLE( CFG_ACTIVE_MIN_CHANNEL_TIME_BTC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nActiveMinChnTimeBtc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ACTIVE_MIN_CHANNEL_TIME_BTC_DEFAULT,
                 CFG_ACTIVE_MIN_CHANNEL_TIME_BTC_MIN,
                 CFG_ACTIVE_MIN_CHANNEL_TIME_BTC_MAX ),

   REG_VARIABLE( CFG_RETRY_LIMIT_ZERO_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, retryLimitZero,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RETRY_LIMIT_ZERO_DEFAULT,
                 CFG_RETRY_LIMIT_ZERO_MIN,
                 CFG_RETRY_LIMIT_ZERO_MAX ),

   REG_VARIABLE( CFG_RETRY_LIMIT_ONE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, retryLimitOne,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RETRY_LIMIT_ONE_DEFAULT,
                 CFG_RETRY_LIMIT_ONE_MIN,
                 CFG_RETRY_LIMIT_ONE_MAX ),

   REG_VARIABLE( CFG_RETRY_LIMIT_TWO_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, retryLimitTwo,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RETRY_LIMIT_TWO_DEFAULT,
                 CFG_RETRY_LIMIT_TWO_MIN,
                 CFG_RETRY_LIMIT_TWO_MAX ),

   REG_VARIABLE( CFG_DISABLE_AGG_WITH_BTC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, disableAggWithBtc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_DISABLE_AGG_WITH_BTC_DEFAULT,
                 CFG_DISABLE_AGG_WITH_BTC_MIN,
                 CFG_DISABLE_AGG_WITH_BTC_MAX ),

#ifdef WLAN_AP_STA_CONCURRENCY
   REG_VARIABLE( CFG_PASSIVE_MAX_CHANNEL_TIME_CONC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nPassiveMaxChnTimeConc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_PASSIVE_MAX_CHANNEL_TIME_CONC_DEFAULT,
                 CFG_PASSIVE_MAX_CHANNEL_TIME_CONC_MIN,
                 CFG_PASSIVE_MAX_CHANNEL_TIME_CONC_MAX ),

   REG_VARIABLE( CFG_PASSIVE_MIN_CHANNEL_TIME_CONC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nPassiveMinChnTimeConc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_PASSIVE_MIN_CHANNEL_TIME_CONC_DEFAULT,
                 CFG_PASSIVE_MIN_CHANNEL_TIME_CONC_MIN,
                 CFG_PASSIVE_MIN_CHANNEL_TIME_CONC_MAX ),

   REG_VARIABLE( CFG_ACTIVE_MAX_CHANNEL_TIME_CONC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nActiveMaxChnTimeConc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ACTIVE_MAX_CHANNEL_TIME_CONC_DEFAULT,
                 CFG_ACTIVE_MAX_CHANNEL_TIME_CONC_MIN,
                 CFG_ACTIVE_MAX_CHANNEL_TIME_CONC_MAX ),

   REG_VARIABLE( CFG_ACTIVE_MIN_CHANNEL_TIME_CONC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nActiveMinChnTimeConc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ACTIVE_MIN_CHANNEL_TIME_CONC_DEFAULT,
                 CFG_ACTIVE_MIN_CHANNEL_TIME_CONC_MIN,
                 CFG_ACTIVE_MIN_CHANNEL_TIME_CONC_MAX ),

   REG_VARIABLE( CFG_REST_TIME_CONC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nRestTimeConc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_REST_TIME_CONC_DEFAULT,
                 CFG_REST_TIME_CONC_MIN,
                 CFG_REST_TIME_CONC_MAX ),

   REG_VARIABLE( CFG_NUM_STA_CHAN_COMBINED_CONC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nNumStaChanCombinedConc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_NUM_STA_CHAN_COMBINED_CONC_DEFAULT,
                 CFG_NUM_STA_CHAN_COMBINED_CONC_MIN,
                 CFG_NUM_STA_CHAN_COMBINED_CONC_MAX ),

   REG_VARIABLE( CFG_NUM_P2P_CHAN_COMBINED_CONC_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nNumP2PChanCombinedConc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_NUM_P2P_CHAN_COMBINED_CONC_DEFAULT,
                 CFG_NUM_P2P_CHAN_COMBINED_CONC_MIN,
                 CFG_NUM_P2P_CHAN_COMBINED_CONC_MAX ),
#endif

   REG_VARIABLE( CFG_MAX_PS_POLL_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nMaxPsPoll,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MAX_PS_POLL_DEFAULT,
                 CFG_MAX_PS_POLL_MIN,
                 CFG_MAX_PS_POLL_MAX ),

    REG_VARIABLE( CFG_MAX_TX_POWER_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nTxPowerCap,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MAX_TX_POWER_DEFAULT,
                 CFG_MAX_TX_POWER_MIN,
                 CFG_MAX_TX_POWER_MAX ),

   REG_VARIABLE( CFG_LOW_GAIN_OVERRIDE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fIsLowGainOverride,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LOW_GAIN_OVERRIDE_DEFAULT,
                 CFG_LOW_GAIN_OVERRIDE_MIN,
                 CFG_LOW_GAIN_OVERRIDE_MAX ),

   REG_VARIABLE( CFG_RSSI_FILTER_PERIOD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nRssiFilterPeriod,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RSSI_FILTER_PERIOD_DEFAULT,
                 CFG_RSSI_FILTER_PERIOD_MIN,
                 CFG_RSSI_FILTER_PERIOD_MAX ),

   REG_VARIABLE( CFG_IGNORE_DTIM_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fIgnoreDtim,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IGNORE_DTIM_DEFAULT,
                 CFG_IGNORE_DTIM_MIN,
                 CFG_IGNORE_DTIM_MAX ),

   REG_VARIABLE( CFG_MAX_LI_MODULATED_DTIM_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fMaxLIModulatedDTIM,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MAX_LI_MODULATED_DTIM_DEFAULT,
                 CFG_MAX_LI_MODULATED_DTIM_MIN,
                 CFG_MAX_LI_MODULATED_DTIM_MAX ),

   REG_VARIABLE( CFG_RX_ANT_CONFIGURATION_NAME, WLAN_PARAM_Integer,
                hdd_config_t, nRxAnt,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_RX_ANT_CONFIGURATION_NAME_DEFAULT,
                CFG_RX_ANT_CONFIGURATION_NAME_MIN,
                CFG_RX_ANT_CONFIGURATION_NAME_MAX ),

   REG_VARIABLE( CFG_FW_HEART_BEAT_MONITORING_NAME, WLAN_PARAM_Integer,
                hdd_config_t, fEnableFwHeartBeatMonitoring,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_FW_HEART_BEAT_MONITORING_DEFAULT,
                CFG_FW_HEART_BEAT_MONITORING_MIN,
                CFG_FW_HEART_BEAT_MONITORING_MAX ),

   REG_VARIABLE( CFG_FW_BEACON_FILTERING_NAME, WLAN_PARAM_Integer,
                hdd_config_t, fEnableFwBeaconFiltering,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_FW_BEACON_FILTERING_DEFAULT,
                CFG_FW_BEACON_FILTERING_MIN,
                CFG_FW_BEACON_FILTERING_MAX ),

   REG_DYNAMIC_VARIABLE( CFG_FW_RSSI_MONITORING_NAME, WLAN_PARAM_Integer,
                hdd_config_t, fEnableFwRssiMonitoring,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_FW_RSSI_MONITORING_DEFAULT,
                CFG_FW_RSSI_MONITORING_MIN,
                CFG_FW_RSSI_MONITORING_MAX,
                cbNotifySetFwRssiMonitoring, 0 ),

   REG_VARIABLE( CFG_DATA_INACTIVITY_TIMEOUT_NAME, WLAN_PARAM_Integer,
                hdd_config_t, nDataInactivityTimeout,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_DATA_INACTIVITY_TIMEOUT_DEFAULT,
                CFG_DATA_INACTIVITY_TIMEOUT_MIN,
                CFG_DATA_INACTIVITY_TIMEOUT_MAX ),

   REG_VARIABLE( CFG_NTH_BEACON_FILTER_NAME, WLAN_PARAM_Integer,
                hdd_config_t, nthBeaconFilter,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_NTH_BEACON_FILTER_DEFAULT,
                CFG_NTH_BEACON_FILTER_MIN,
                CFG_NTH_BEACON_FILTER_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_MODE_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, WmmMode,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_MODE_DEFAULT,
                 CFG_QOS_WMM_MODE_MIN,
                 CFG_QOS_WMM_MODE_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_80211E_ENABLED_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, b80211eIsEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_80211E_ENABLED_DEFAULT,
                 CFG_QOS_WMM_80211E_ENABLED_MIN,
                 CFG_QOS_WMM_80211E_ENABLED_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_UAPSD_MASK_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, UapsdMask,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_UAPSD_MASK_DEFAULT,
                 CFG_QOS_WMM_UAPSD_MASK_MIN,
                 CFG_QOS_WMM_UAPSD_MASK_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_UAPSD_VO_SRV_INTV_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraUapsdVoSrvIntv,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_VO_SRV_INTV_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_VO_SRV_INTV_MIN,
                 CFG_QOS_WMM_INFRA_UAPSD_VO_SRV_INTV_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_UAPSD_VO_SUS_INTV_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraUapsdVoSuspIntv,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_VO_SUS_INTV_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_VO_SUS_INTV_MIN,
                 CFG_QOS_WMM_INFRA_UAPSD_VO_SUS_INTV_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_UAPSD_VI_SRV_INTV_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraUapsdViSrvIntv,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_VI_SRV_INTV_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_VI_SRV_INTV_MIN,
                 CFG_QOS_WMM_INFRA_UAPSD_VI_SRV_INTV_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_UAPSD_VI_SUS_INTV_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraUapsdViSuspIntv,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_VI_SUS_INTV_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_VI_SUS_INTV_MIN,
                 CFG_QOS_WMM_INFRA_UAPSD_VI_SUS_INTV_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_UAPSD_BE_SRV_INTV_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraUapsdBeSrvIntv,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_BE_SRV_INTV_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_BE_SRV_INTV_MIN,
                 CFG_QOS_WMM_INFRA_UAPSD_BE_SRV_INTV_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_UAPSD_BE_SUS_INTV_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraUapsdBeSuspIntv,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_BE_SUS_INTV_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_BE_SUS_INTV_MIN,
                 CFG_QOS_WMM_INFRA_UAPSD_BE_SUS_INTV_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_UAPSD_BK_SRV_INTV_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraUapsdBkSrvIntv,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_BK_SRV_INTV_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_BK_SRV_INTV_MIN,
                 CFG_QOS_WMM_INFRA_UAPSD_BK_SRV_INTV_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_UAPSD_BK_SUS_INTV_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraUapsdBkSuspIntv,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_BK_SUS_INTV_DEFAULT,
                 CFG_QOS_WMM_INFRA_UAPSD_BK_SUS_INTV_MIN,
                 CFG_QOS_WMM_INFRA_UAPSD_BK_SUS_INTV_MAX ),

#ifdef FEATURE_WLAN_ESE
   REG_VARIABLE( CFG_QOS_WMM_INFRA_INACTIVITY_INTERVAL_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, InfraInactivityInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_INACTIVITY_INTERVAL_DEFAULT,
                 CFG_QOS_WMM_INFRA_INACTIVITY_INTERVAL_MIN,
                 CFG_QOS_WMM_INFRA_INACTIVITY_INTERVAL_MAX),

   REG_DYNAMIC_VARIABLE( CFG_ESE_FEATURE_ENABLED_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, isEseIniFeatureEnabled,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ESE_FEATURE_ENABLED_DEFAULT,
                         CFG_ESE_FEATURE_ENABLED_MIN,
                         CFG_ESE_FEATURE_ENABLED_MAX,
                         cbNotifySetEseFeatureEnabled, 0 ),
#endif // FEATURE_WLAN_ESE

#ifdef FEATURE_WLAN_LFR
   // flag to turn ON/OFF Legacy Fast Roaming
   REG_DYNAMIC_VARIABLE( CFG_LFR_FEATURE_ENABLED_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, isFastRoamIniFeatureEnabled,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_LFR_FEATURE_ENABLED_DEFAULT,
                         CFG_LFR_FEATURE_ENABLED_MIN,
                         CFG_LFR_FEATURE_ENABLED_MAX,
                         NotifyIsFastRoamIniFeatureEnabled, 0 ),

   /* flag to turn ON/OFF Motion assistance for Legacy Fast Roaming */
   REG_DYNAMIC_VARIABLE( CFG_LFR_MAWC_FEATURE_ENABLED_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, MAWCEnabled,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_LFR_MAWC_FEATURE_ENABLED_DEFAULT,
                         CFG_LFR_MAWC_FEATURE_ENABLED_MIN,
                         CFG_LFR_MAWC_FEATURE_ENABLED_MAX,
                         NotifyIsMAWCIniFeatureEnabled, 0 ),

#endif // FEATURE_WLAN_LFR

#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
   // flag to turn ON/OFF 11r and ESE FastTransition
   REG_DYNAMIC_VARIABLE( CFG_FAST_TRANSITION_ENABLED_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, isFastTransitionEnabled,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_FAST_TRANSITION_ENABLED_NAME_DEFAULT,
                         CFG_FAST_TRANSITION_ENABLED_NAME_MIN,
                         CFG_FAST_TRANSITION_ENABLED_NAME_MAX,
                         cbNotifySetFastTransitionEnabled, 0 ),

   /* Variable to specify the delta/difference between the RSSI of current AP
    * and roamable AP while roaming */
   REG_DYNAMIC_VARIABLE( CFG_ROAM_RSSI_DIFF_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, RoamRssiDiff,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_RSSI_DIFF_DEFAULT,
                         CFG_ROAM_RSSI_DIFF_MIN,
                         CFG_ROAM_RSSI_DIFF_MAX,
                         cbNotifySetRoamRssiDiff, 0),

   REG_DYNAMIC_VARIABLE( CFG_IMMEDIATE_ROAM_RSSI_DIFF_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nImmediateRoamRssiDiff,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_IMMEDIATE_ROAM_RSSI_DIFF_DEFAULT,
                         CFG_IMMEDIATE_ROAM_RSSI_DIFF_MIN,
                         CFG_IMMEDIATE_ROAM_RSSI_DIFF_MAX,
                         cbNotifySetImmediateRoamRssiDiff, 0),

   REG_DYNAMIC_VARIABLE( CFG_ENABLE_WES_MODE_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, isWESModeEnabled,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ENABLE_WES_MODE_NAME_DEFAULT,
                         CFG_ENABLE_WES_MODE_NAME_MIN,
                         CFG_ENABLE_WES_MODE_NAME_MAX,
                         cbNotifySetWESMode, 0),
#endif
#ifdef FEATURE_WLAN_OKC
   REG_DYNAMIC_VARIABLE( CFG_OKC_FEATURE_ENABLED_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, isOkcIniFeatureEnabled,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_OKC_FEATURE_ENABLED_DEFAULT,
                         CFG_OKC_FEATURE_ENABLED_MIN,
                         CFG_OKC_FEATURE_ENABLED_MAX,
                         cbNotifySetOkcFeatureEnabled, 0 ),
#endif
#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
   REG_DYNAMIC_VARIABLE( CFG_ROAM_SCAN_OFFLOAD_ENABLED, WLAN_PARAM_Integer,
                         hdd_config_t, isRoamOffloadScanEnabled,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_SCAN_OFFLOAD_ENABLED_DEFAULT,
                         CFG_ROAM_SCAN_OFFLOAD_ENABLED_MIN,
                         CFG_ROAM_SCAN_OFFLOAD_ENABLED_MAX,
                         cbNotifyUpdateRoamScanOffloadEnabled, 0),
#endif
   REG_VARIABLE( CFG_QOS_WMM_PKT_CLASSIFY_BASIS_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, PktClassificationBasis,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_PKT_CLASSIFY_BASIS_DEFAULT,
                 CFG_QOS_WMM_PKT_CLASSIFY_BASIS_MIN,
                 CFG_QOS_WMM_PKT_CLASSIFY_BASIS_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_DIR_AC_VO_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraDirAcVo,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_DIR_AC_VO_DEFAULT,
                 CFG_QOS_WMM_INFRA_DIR_AC_VO_MIN,
                 CFG_QOS_WMM_INFRA_DIR_AC_VO_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_VO_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraNomMsduSizeAcVo,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_VO_DEFAULT,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_VO_MIN,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_VO_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_VO_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraMeanDataRateAcVo,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_VO_DEFAULT,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_VO_MIN,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_VO_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_VO_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraMinPhyRateAcVo,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_VO_DEFAULT,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_VO_MIN,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_VO_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_SBA_AC_VO_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraSbaAcVo,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_SBA_AC_VO_DEFAULT,
                 CFG_QOS_WMM_INFRA_SBA_AC_VO_MIN,
                 CFG_QOS_WMM_INFRA_SBA_AC_VO_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_DIR_AC_VI_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraDirAcVi,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_DIR_AC_VI_DEFAULT,
                 CFG_QOS_WMM_INFRA_DIR_AC_VI_MIN,
                 CFG_QOS_WMM_INFRA_DIR_AC_VI_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_VI_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraNomMsduSizeAcVi,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_VI_DEFAULT,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_VI_MIN,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_VI_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_VI_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraMeanDataRateAcVi,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_VI_DEFAULT,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_VI_MIN,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_VI_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_VI_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraMinPhyRateAcVi,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_VI_DEFAULT,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_VI_MIN,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_VI_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_SBA_AC_VI_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraSbaAcVi,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_SBA_AC_VI_DEFAULT,
                 CFG_QOS_WMM_INFRA_SBA_AC_VI_MIN,
                 CFG_QOS_WMM_INFRA_SBA_AC_VI_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_DIR_AC_BE_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraDirAcBe,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_DIR_AC_BE_DEFAULT,
                 CFG_QOS_WMM_INFRA_DIR_AC_BE_MIN,
                 CFG_QOS_WMM_INFRA_DIR_AC_BE_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_BE_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraNomMsduSizeAcBe,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_BE_DEFAULT,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_BE_MIN,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_BE_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_BE_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraMeanDataRateAcBe,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_BE_DEFAULT,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_BE_MIN,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_BE_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_BE_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraMinPhyRateAcBe,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_BE_DEFAULT,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_BE_MIN,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_BE_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_SBA_AC_BE_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraSbaAcBe,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_SBA_AC_BE_DEFAULT,
                 CFG_QOS_WMM_INFRA_SBA_AC_BE_MIN,
                 CFG_QOS_WMM_INFRA_SBA_AC_BE_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_DIR_AC_BK_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, InfraDirAcBk,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_DIR_AC_BK_DEFAULT,
                 CFG_QOS_WMM_INFRA_DIR_AC_BK_MIN,
                 CFG_QOS_WMM_INFRA_DIR_AC_BK_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_BK_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraNomMsduSizeAcBk,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_BK_DEFAULT,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_BK_MIN,
                 CFG_QOS_WMM_INFRA_NOM_MSDU_SIZE_AC_BK_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_BK_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraMeanDataRateAcBk,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_BK_DEFAULT,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_BK_MIN,
                 CFG_QOS_WMM_INFRA_MEAN_DATA_RATE_AC_BK_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_BK_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraMinPhyRateAcBk,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_BK_DEFAULT,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_BK_MIN,
                 CFG_QOS_WMM_INFRA_MIN_PHY_RATE_AC_BK_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_INFRA_SBA_AC_BK_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, InfraSbaAcBk,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_INFRA_SBA_AC_BK_DEFAULT,
                 CFG_QOS_WMM_INFRA_SBA_AC_BK_MIN,
                 CFG_QOS_WMM_INFRA_SBA_AC_BK_MAX ),

   REG_VARIABLE( CFG_TL_DELAYED_TRGR_FRM_INT_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, DelayedTriggerFrmInt,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TL_DELAYED_TRGR_FRM_INT_DEFAULT,
                 CFG_TL_DELAYED_TRGR_FRM_INT_MIN,
                 CFG_TL_DELAYED_TRGR_FRM_INT_MAX ),

   REG_VARIABLE_STRING( CFG_WOWL_PATTERN_NAME, WLAN_PARAM_String,
                        hdd_config_t, wowlPattern,
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_WOWL_PATTERN_DEFAULT ),

   REG_VARIABLE( CFG_QOS_IMPLICIT_SETUP_ENABLED_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, bImplicitQosEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_IMPLICIT_SETUP_ENABLED_DEFAULT,
                 CFG_QOS_IMPLICIT_SETUP_ENABLED_MIN,
                 CFG_QOS_IMPLICIT_SETUP_ENABLED_MAX ),

   REG_VARIABLE( CFG_BTC_EXECUTION_MODE_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, btcExecutionMode,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BTC_EXECUTION_MODE_DEFAULT,
                 CFG_BTC_EXECUTION_MODE_MIN,
                 CFG_BTC_EXECUTION_MODE_MAX ),

   REG_VARIABLE( CFG_MWS_COEX_CONFIG1_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, mwsCoexConfig[0],
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_MIN,
                 CFG_MWS_COEX_CONFIGX_MAX ),

   REG_VARIABLE( CFG_MWS_COEX_CONFIG2_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, mwsCoexConfig[1],
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_MIN,
                 CFG_MWS_COEX_CONFIGX_MAX ),

   REG_VARIABLE( CFG_MWS_COEX_CONFIG3_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, mwsCoexConfig[2],
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_MIN,
                 CFG_MWS_COEX_CONFIGX_MAX ),

   REG_VARIABLE( CFG_MWS_COEX_CONFIG4_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, mwsCoexConfig[3],
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_MIN,
                 CFG_MWS_COEX_CONFIGX_MAX ),

   REG_VARIABLE( CFG_MWS_COEX_CONFIG5_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, mwsCoexConfig[4],
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_MIN,
                 CFG_MWS_COEX_CONFIGX_MAX ),

   REG_VARIABLE( CFG_MWS_COEX_CONFIG6_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, mwsCoexConfig[5],
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_DEFAULT,
                 CFG_MWS_COEX_CONFIGX_MIN,
                 CFG_MWS_COEX_CONFIGX_MAX ),

   REG_VARIABLE( CFG_AP_LISTEN_MODE_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, nEnableListenMode,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_LISTEN_MODE_DEFAULT,
                 CFG_AP_LISTEN_MODE_MIN,
                 CFG_AP_LISTEN_MODE_MAX ),

   REG_VARIABLE( CFG_AP_AUTO_SHUT_OFF , WLAN_PARAM_Integer,
                 hdd_config_t, nAPAutoShutOff,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_AUTO_SHUT_OFF_DEFAULT,
                 CFG_AP_AUTO_SHUT_OFF_MIN,
                 CFG_AP_AUTO_SHUT_OFF_MAX ),

#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
   REG_VARIABLE( CFG_WLAN_MCC_TO_SCC_SWITCH_MODE , WLAN_PARAM_Integer,
                 hdd_config_t, WlanMccToSccSwitchMode,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_WLAN_MCC_TO_SCC_SWITCH_MODE_DEFAULT,
                 CFG_WLAN_MCC_TO_SCC_SWITCH_MODE_MIN,
                 CFG_WLAN_MCC_TO_SCC_SWITCH_MODE_MAX ),
#endif
#ifdef FEATURE_WLAN_AUTO_SHUTDOWN
   REG_VARIABLE( CFG_WLAN_AUTO_SHUTDOWN , WLAN_PARAM_Integer,
                 hdd_config_t, WlanAutoShutdown,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_WLAN_AUTO_SHUTDOWN_DEFAULT,
                 CFG_WLAN_AUTO_SHUTDOWN_MIN,
                 CFG_WLAN_AUTO_SHUTDOWN_MAX ),
#endif
#if defined WLAN_FEATURE_VOWIFI
   REG_VARIABLE( CFG_RRM_ENABLE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fRrmEnable,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RRM_ENABLE_DEFAULT,
                 CFG_RRM_ENABLE_MIN,
                 CFG_RRM_ENABLE_MAX ),

   REG_VARIABLE( CFG_RRM_OPERATING_CHAN_MAX_DURATION_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nInChanMeasMaxDuration,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RRM_OPERATING_CHAN_MAX_DURATION_DEFAULT,
                 CFG_RRM_OPERATING_CHAN_MAX_DURATION_MIN,
                 CFG_RRM_OPERATING_CHAN_MAX_DURATION_MAX ),

   REG_VARIABLE( CFG_RRM_NON_OPERATING_CHAN_MAX_DURATION_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nOutChanMeasMaxDuration,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RRM_NON_OPERATING_CHAN_MAX_DURATION_DEFAULT,
                 CFG_RRM_NON_OPERATING_CHAN_MAX_DURATION_MIN,
                 CFG_RRM_NON_OPERATING_CHAN_MAX_DURATION_MAX ),

   REG_VARIABLE( CFG_RRM_MEAS_RANDOMIZATION_INTVL_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nRrmRandnIntvl,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RRM_MEAS_RANDOMIZATION_INTVL_DEFAULT,
                 CFG_RRM_MEAS_RANDOMIZATION_INTVL_MIN,
                 CFG_RRM_MEAS_RANDOMIZATION_INTVL_MAX ),
#endif

#ifdef WLAN_FEATURE_VOWIFI_11R
   REG_VARIABLE( CFG_FT_RESOURCE_REQ_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fFTResourceReqSupported,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_FT_RESOURCE_REQ_DEFAULT,
                 CFG_FT_RESOURCE_REQ_MIN,
                 CFG_FT_RESOURCE_REQ_MAX ),
#endif

#ifdef WLAN_FEATURE_NEIGHBOR_ROAMING
   REG_DYNAMIC_VARIABLE( CFG_NEIGHBOR_SCAN_TIMER_PERIOD_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nNeighborScanPeriod,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_NEIGHBOR_SCAN_TIMER_PERIOD_DEFAULT,
                         CFG_NEIGHBOR_SCAN_TIMER_PERIOD_MIN,
                         CFG_NEIGHBOR_SCAN_TIMER_PERIOD_MAX,
                         cbNotifySetNeighborScanPeriod, 0 ),

   REG_VARIABLE( CFG_NEIGHBOR_REASSOC_RSSI_THRESHOLD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nNeighborReassocRssiThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_NEIGHBOR_REASSOC_RSSI_THRESHOLD_DEFAULT,
                 CFG_NEIGHBOR_REASSOC_RSSI_THRESHOLD_MIN,
                 CFG_NEIGHBOR_REASSOC_RSSI_THRESHOLD_MAX ),

   REG_DYNAMIC_VARIABLE( CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nNeighborLookupRssiThreshold,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_DEFAULT,
                         CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_MIN,
                         CFG_NEIGHBOR_LOOKUP_RSSI_THRESHOLD_MAX,
                         cbNotifySetNeighborLookupRssiThreshold, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_OPPORTUNISTIC_SCAN_THRESHOLD_DIFF_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nOpportunisticThresholdDiff,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_OPPORTUNISTIC_SCAN_THRESHOLD_DIFF_DEFAULT,
                         CFG_OPPORTUNISTIC_SCAN_THRESHOLD_DIFF_MIN,
                         CFG_OPPORTUNISTIC_SCAN_THRESHOLD_DIFF_MAX,
                          cbNotifySetOpportunisticScanThresholdDiff, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_ROAM_RESCAN_RSSI_DIFF_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nRoamRescanRssiDiff,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_RESCAN_RSSI_DIFF_DEFAULT,
                         CFG_ROAM_RESCAN_RSSI_DIFF_MIN,
                         CFG_ROAM_RESCAN_RSSI_DIFF_MAX,
                         cbNotifySetRoamRescanRssiDiff, 0 ),

   REG_VARIABLE_STRING( CFG_NEIGHBOR_SCAN_CHAN_LIST_NAME, WLAN_PARAM_String,
                        hdd_config_t, neighborScanChanList,
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_NEIGHBOR_SCAN_CHAN_LIST_DEFAULT ),

   REG_DYNAMIC_VARIABLE( CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nNeighborScanMinChanTime,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_DEFAULT,
                         CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_MIN,
                         CFG_NEIGHBOR_SCAN_MIN_CHAN_TIME_MAX,
                         cbNotifySetNeighborScanMinChanTime, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nNeighborScanMaxChanTime,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_DEFAULT,
                         CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_MIN,
                         CFG_NEIGHBOR_SCAN_MAX_CHAN_TIME_MAX,
                         cbNotifySetNeighborScanMaxChanTime, 0 ),

   REG_VARIABLE( CFG_11R_NEIGHBOR_REQ_MAX_TRIES_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nMaxNeighborReqTries,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_11R_NEIGHBOR_REQ_MAX_TRIES_DEFAULT,
                 CFG_11R_NEIGHBOR_REQ_MAX_TRIES_MIN,
                 CFG_11R_NEIGHBOR_REQ_MAX_TRIES_MAX),

   REG_DYNAMIC_VARIABLE( CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nNeighborResultsRefreshPeriod,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_DEFAULT,
                         CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_MIN,
                         CFG_NEIGHBOR_SCAN_RESULTS_REFRESH_PERIOD_MAX,
                         cbNotifySetNeighborResultsRefreshPeriod, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_EMPTY_SCAN_REFRESH_PERIOD_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nEmptyScanRefreshPeriod,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_EMPTY_SCAN_REFRESH_PERIOD_DEFAULT,
                         CFG_EMPTY_SCAN_REFRESH_PERIOD_MIN,
                         CFG_EMPTY_SCAN_REFRESH_PERIOD_MAX,
                         cbNotifySetEmptyScanRefreshPeriod, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_ROAM_BMISS_FIRST_BCNT_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nRoamBmissFirstBcnt,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_BMISS_FIRST_BCNT_DEFAULT,
                         CFG_ROAM_BMISS_FIRST_BCNT_MIN,
                         CFG_ROAM_BMISS_FIRST_BCNT_MAX,
                         cbNotifySetRoamBmissFirstBcnt, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_ROAM_BMISS_FINAL_BCNT_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nRoamBmissFinalBcnt,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_BMISS_FINAL_BCNT_DEFAULT,
                         CFG_ROAM_BMISS_FINAL_BCNT_MIN,
                         CFG_ROAM_BMISS_FINAL_BCNT_MAX,
                         cbNotifySetRoamBmissFinalBcnt, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_ROAM_BEACON_RSSI_WEIGHT_NAME, WLAN_PARAM_Integer,
                         hdd_config_t, nRoamBeaconRssiWeight,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_BEACON_RSSI_WEIGHT_DEFAULT,
                         CFG_ROAM_BEACON_RSSI_WEIGHT_MIN,
                         CFG_ROAM_BEACON_RSSI_WEIGHT_MAX,
                         cbNotifySetRoamBeaconRssiWeight, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_ROAMING_DFS_CHANNEL_NAME , WLAN_PARAM_Integer,
                         hdd_config_t, allowDFSChannelRoam,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAMING_DFS_CHANNEL_DEFAULT,
                         CFG_ROAMING_DFS_CHANNEL_MIN,
                         CFG_ROAMING_DFS_CHANNEL_MAX,
                         cbNotifySetDFSScanMode, 0),

#endif /* WLAN_FEATURE_NEIGHBOR_ROAMING */

   REG_VARIABLE( CFG_QOS_WMM_BURST_SIZE_DEFN_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, burstSizeDefinition,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_BURST_SIZE_DEFN_DEFAULT,
                 CFG_QOS_WMM_BURST_SIZE_DEFN_MIN,
                 CFG_QOS_WMM_BURST_SIZE_DEFN_MAX ),

   REG_VARIABLE( CFG_MCAST_BCAST_FILTER_SETTING_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, mcastBcastFilterSetting,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MCAST_BCAST_FILTER_SETTING_DEFAULT,
                 CFG_MCAST_BCAST_FILTER_SETTING_MIN,
                 CFG_MCAST_BCAST_FILTER_SETTING_MAX ),

   REG_VARIABLE( CFG_ENABLE_HOST_ARPOFFLOAD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fhostArpOffload,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_HOST_ARPOFFLOAD_DEFAULT,
                 CFG_ENABLE_HOST_ARPOFFLOAD_MIN,
                 CFG_ENABLE_HOST_ARPOFFLOAD_MAX ),

#ifdef FEATURE_WLAN_RA_FILTERING
   REG_VARIABLE( CFG_RA_FILTER_ENABLE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, IsRArateLimitEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RA_FILTER_ENABLE_DEFAULT,
                 CFG_RA_FILTER_ENABLE_MIN,
                 CFG_RA_FILTER_ENABLE_MAX ),

   REG_VARIABLE( CFG_RA_RATE_LIMIT_INTERVAL_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, RArateLimitInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_RA_RATE_LIMIT_INTERVAL_DEFAULT,
                 CFG_RA_RATE_LIMIT_INTERVAL_MIN,
                 CFG_RA_RATE_LIMIT_INTERVAL_MAX ),
#endif

   REG_VARIABLE( CFG_ENABLE_HOST_SSDP_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ssdp,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_HOST_SSDP_DEFAULT,
                 CFG_ENABLE_HOST_SSDP_MIN,
                 CFG_ENABLE_HOST_SSDP_MAX ),

   REG_VARIABLE( CFG_ENABLE_HOST_NSOFFLOAD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fhostNSOffload,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_HOST_NSOFFLOAD_DEFAULT,
                 CFG_ENABLE_HOST_NSOFFLOAD_MIN,
                 CFG_ENABLE_HOST_NSOFFLOAD_MAX ),

   REG_VARIABLE( CFG_QOS_WMM_TS_INFO_ACK_POLICY_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, tsInfoAckPolicy,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_QOS_WMM_TS_INFO_ACK_POLICY_DEFAULT,
                 CFG_QOS_WMM_TS_INFO_ACK_POLICY_MIN,
                 CFG_QOS_WMM_TS_INFO_ACK_POLICY_MAX ),

    REG_VARIABLE( CFG_SINGLE_TID_RC_NAME, WLAN_PARAM_Integer,
                  hdd_config_t, bSingleTidRc,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                  CFG_SINGLE_TID_RC_DEFAULT,
                  CFG_SINGLE_TID_RC_MIN,
                  CFG_SINGLE_TID_RC_MAX),

    REG_VARIABLE( CFG_DYNAMIC_PSPOLL_VALUE_NAME, WLAN_PARAM_Integer,
                  hdd_config_t, dynamicPsPollValue,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                  CFG_DYNAMIC_PSPOLL_VALUE_DEFAULT,
                  CFG_DYNAMIC_PSPOLL_VALUE_MIN,
                  CFG_DYNAMIC_PSPOLL_VALUE_MAX ),

   REG_VARIABLE( CFG_TELE_BCN_WAKEUP_EN_NAME, WLAN_PARAM_Integer,
                  hdd_config_t, teleBcnWakeupEn,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                  CFG_TELE_BCN_WAKEUP_EN_DEFAULT,
                  CFG_TELE_BCN_WAKEUP_EN_MIN,
                  CFG_TELE_BCN_WAKEUP_EN_MAX ),

    REG_VARIABLE( CFG_INFRA_STA_KEEP_ALIVE_PERIOD_NAME, WLAN_PARAM_Integer,
                  hdd_config_t, infraStaKeepAlivePeriod,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                  CFG_INFRA_STA_KEEP_ALIVE_PERIOD_DEFAULT,
                  CFG_INFRA_STA_KEEP_ALIVE_PERIOD_MIN,
                  CFG_INFRA_STA_KEEP_ALIVE_PERIOD_MAX),

    REG_VARIABLE( CFG_QOS_ADDTS_WHEN_ACM_IS_OFF_NAME , WLAN_PARAM_Integer,
                  hdd_config_t, AddTSWhenACMIsOff,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                  CFG_QOS_ADDTS_WHEN_ACM_IS_OFF_DEFAULT,
                  CFG_QOS_ADDTS_WHEN_ACM_IS_OFF_MIN,
                  CFG_QOS_ADDTS_WHEN_ACM_IS_OFF_MAX ),


    REG_VARIABLE( CFG_VALIDATE_SCAN_LIST_NAME , WLAN_PARAM_Integer,
                  hdd_config_t, fValidateScanList,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                  CFG_VALIDATE_SCAN_LIST_DEFAULT,
                  CFG_VALIDATE_SCAN_LIST_MIN,
                  CFG_VALIDATE_SCAN_LIST_MAX ),

    REG_VARIABLE( CFG_NULLDATA_AP_RESP_TIMEOUT_NAME, WLAN_PARAM_Integer,
                  hdd_config_t, nNullDataApRespTimeout,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                  CFG_NULLDATA_AP_RESP_TIMEOUT_DEFAULT,
                  CFG_NULLDATA_AP_RESP_TIMEOUT_MIN,
                  CFG_NULLDATA_AP_RESP_TIMEOUT_MAX ),

    REG_VARIABLE( CFG_AP_DATA_AVAIL_POLL_PERIOD_NAME, WLAN_PARAM_Integer,
                  hdd_config_t, apDataAvailPollPeriodInMs,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                  CFG_AP_DATA_AVAIL_POLL_PERIOD_DEFAULT,
                  CFG_AP_DATA_AVAIL_POLL_PERIOD_MIN,
                  CFG_AP_DATA_AVAIL_POLL_PERIOD_MAX ),

   REG_VARIABLE( CFG_BAND_CAPABILITY_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nBandCapability,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BAND_CAPABILITY_DEFAULT,
                 CFG_BAND_CAPABILITY_MIN,
                 CFG_BAND_CAPABILITY_MAX ),

   REG_VARIABLE( CFG_ENABLE_BEACON_EARLY_TERMINATION_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableBeaconEarlyTermination,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_BEACON_EARLY_TERMINATION_DEFAULT,
                 CFG_ENABLE_BEACON_EARLY_TERMINATION_MIN,
                 CFG_ENABLE_BEACON_EARLY_TERMINATION_MAX ),

/* CFG_VOS_TRACE_ENABLE Parameters */
   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_TL_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnableTL,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_WDI_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnableWDI,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_HDD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnableHDD,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_SME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnableSME,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_PE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnablePE,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_PMC_NAME,  WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnablePMC,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_WDA_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnableWDA,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_SYS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnableSYS,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_VOSS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnableVOSS,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_SAP_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnableSAP,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_VOS_TRACE_ENABLE_HDD_SAP_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vosTraceEnableHDDSAP,
                 VAR_FLAGS_OPTIONAL,
                 CFG_VOS_TRACE_ENABLE_DEFAULT,
                 CFG_VOS_TRACE_ENABLE_MIN,
                 CFG_VOS_TRACE_ENABLE_MAX ),

   REG_VARIABLE( CFG_TELE_BCN_TRANS_LI_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nTeleBcnTransListenInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TELE_BCN_TRANS_LI_DEFAULT,
                 CFG_TELE_BCN_TRANS_LI_MIN,
                 CFG_TELE_BCN_TRANS_LI_MAX ),

   REG_VARIABLE( CFG_TELE_BCN_TRANS_LI_NUM_IDLE_BCNS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nTeleBcnTransLiNumIdleBeacons,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TELE_BCN_TRANS_LI_NUM_IDLE_BCNS_DEFAULT,
                 CFG_TELE_BCN_TRANS_LI_NUM_IDLE_BCNS_MIN,
                 CFG_TELE_BCN_TRANS_LI_NUM_IDLE_BCNS_MAX ),

   REG_VARIABLE( CFG_TELE_BCN_MAX_LI_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nTeleBcnMaxListenInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TELE_BCN_MAX_LI_DEFAULT,
                 CFG_TELE_BCN_MAX_LI_MIN,
                 CFG_TELE_BCN_MAX_LI_MAX ),

   REG_VARIABLE( CFG_TELE_BCN_MAX_LI_NUM_IDLE_BCNS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nTeleBcnMaxLiNumIdleBeacons,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TELE_BCN_MAX_LI_NUM_IDLE_BCNS_DEFAULT,
                 CFG_TELE_BCN_MAX_LI_NUM_IDLE_BCNS_MIN,
                 CFG_TELE_BCN_MAX_LI_NUM_IDLE_BCNS_MAX ),

   REG_VARIABLE( CFG_BCN_EARLY_TERM_WAKE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, bcnEarlyTermWakeInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BCN_EARLY_TERM_WAKE_DEFAULT,
                 CFG_BCN_EARLY_TERM_WAKE_MIN,
                 CFG_BCN_EARLY_TERM_WAKE_MAX ),

   REG_VARIABLE( CFG_AP_DATA_AVAIL_POLL_PERIOD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, apDataAvailPollPeriodInMs,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AP_DATA_AVAIL_POLL_PERIOD_DEFAULT,
                 CFG_AP_DATA_AVAIL_POLL_PERIOD_MIN,
                 CFG_AP_DATA_AVAIL_POLL_PERIOD_MAX ),

   REG_VARIABLE( CFG_ENABLE_CLOSE_LOOP_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableCloseLoop,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_CLOSE_LOOP_DEFAULT,
                 CFG_ENABLE_CLOSE_LOOP_MIN,
                 CFG_ENABLE_CLOSE_LOOP_MAX ),

   REG_VARIABLE( CFG_ENABLE_BYPASS_11D_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableBypass11d,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_BYPASS_11D_DEFAULT,
                 CFG_ENABLE_BYPASS_11D_MIN,
                 CFG_ENABLE_BYPASS_11D_MAX ),

   REG_VARIABLE( CFG_ENABLE_DFS_CHNL_SCAN_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableDFSChnlScan,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_DFS_CHNL_SCAN_DEFAULT,
                 CFG_ENABLE_DFS_CHNL_SCAN_MIN,
                 CFG_ENABLE_DFS_CHNL_SCAN_MAX ),

   REG_VARIABLE( CFG_ENABLE_DYNAMIC_DTIM_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableDynamicDTIM,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_DYNAMIC_DTIM_DEFAULT,
                 CFG_ENABLE_DYNAMIC_DTIM_MIN,
                 CFG_ENABLE_DYNAMIC_DTIM_MAX ),

   REG_VARIABLE( CFG_ENABLE_AUTOMATIC_TX_POWER_CONTROL_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableAutomaticTxPowerControl,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_AUTOMATIC_TX_POWER_CONTROL_DEFAULT,
                 CFG_ENABLE_AUTOMATIC_TX_POWER_CONTROL_MIN,
                 CFG_ENABLE_AUTOMATIC_TX_POWER_CONTROL_MAX ),

   REG_VARIABLE( CFG_SHORT_GI_40MHZ_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ShortGI40MhzEnable,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SHORT_GI_40MHZ_DEFAULT,
                 CFG_SHORT_GI_40MHZ_MIN,
                 CFG_SHORT_GI_40MHZ_MAX ),

   REG_DYNAMIC_VARIABLE( CFG_REPORT_MAX_LINK_SPEED, WLAN_PARAM_Integer,
                         hdd_config_t, reportMaxLinkSpeed,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_REPORT_MAX_LINK_SPEED_DEFAULT,
                         CFG_REPORT_MAX_LINK_SPEED_MIN,
                         CFG_REPORT_MAX_LINK_SPEED_MAX,
                         NULL, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_LINK_SPEED_RSSI_HIGH, WLAN_PARAM_SignedInteger,
                         hdd_config_t, linkSpeedRssiHigh,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_LINK_SPEED_RSSI_HIGH_DEFAULT,
                         CFG_LINK_SPEED_RSSI_HIGH_MIN,
                         CFG_LINK_SPEED_RSSI_HIGH_MAX,
                         NULL, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_LINK_SPEED_RSSI_MID, WLAN_PARAM_SignedInteger,
                         hdd_config_t, linkSpeedRssiMid,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_LINK_SPEED_RSSI_MID_DEFAULT,
                         CFG_LINK_SPEED_RSSI_MID_MIN,
                         CFG_LINK_SPEED_RSSI_MID_MAX,
                         NULL, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_LINK_SPEED_RSSI_LOW, WLAN_PARAM_SignedInteger,
                         hdd_config_t, linkSpeedRssiLow,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_LINK_SPEED_RSSI_LOW_DEFAULT,
                         CFG_LINK_SPEED_RSSI_LOW_MIN,
                         CFG_LINK_SPEED_RSSI_LOW_MAX,
                         NULL, 0 ),

#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
   REG_DYNAMIC_VARIABLE( CFG_ROAM_PREFER_5GHZ, WLAN_PARAM_Integer,
                         hdd_config_t, nRoamPrefer5GHz,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_PREFER_5GHZ_DEFAULT,
                         CFG_ROAM_PREFER_5GHZ_MIN,
                         CFG_ROAM_PREFER_5GHZ_MAX,
                         cbNotifySetRoamPrefer5GHz, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_ROAM_INTRA_BAND, WLAN_PARAM_Integer,
                         hdd_config_t, nRoamIntraBand,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_INTRA_BAND_DEFAULT,
                         CFG_ROAM_INTRA_BAND_MIN,
                         CFG_ROAM_INTRA_BAND_MAX,
                         cbNotifySetRoamIntraBand, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_ROAM_SCAN_N_PROBES, WLAN_PARAM_Integer,
                         hdd_config_t, nProbes,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_SCAN_N_PROBES_DEFAULT,
                         CFG_ROAM_SCAN_N_PROBES_MIN,
                         CFG_ROAM_SCAN_N_PROBES_MAX,
                         cbNotifySetRoamScanNProbes, 0 ),

   REG_DYNAMIC_VARIABLE( CFG_ROAM_SCAN_HOME_AWAY_TIME, WLAN_PARAM_Integer,
                         hdd_config_t, nRoamScanHomeAwayTime,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ROAM_SCAN_HOME_AWAY_TIME_DEFAULT,
                         CFG_ROAM_SCAN_HOME_AWAY_TIME_MIN,
                         CFG_ROAM_SCAN_HOME_AWAY_TIME_MAX,
                         cbNotifySetRoamScanHomeAwayTime, 0 ),

#endif

   REG_VARIABLE( CFG_P2P_DEVICE_ADDRESS_ADMINISTRATED_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, isP2pDeviceAddrAdministrated,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_P2P_DEVICE_ADDRESS_ADMINISTRATED_DEFAULT,
                 CFG_P2P_DEVICE_ADDRESS_ADMINISTRATED_MIN,
                 CFG_P2P_DEVICE_ADDRESS_ADMINISTRATED_MAX ),

   REG_VARIABLE( CFG_ENABLE_MCC_ENABLED_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableMCC,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_MCC_ENABLED_DEFAULT,
                 CFG_ENABLE_MCC_ENABLED_MIN,
                 CFG_ENABLE_MCC_ENABLED_MAX ),

   REG_VARIABLE( CFG_ALLOW_MCC_GO_DIFF_BI_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, allowMCCGODiffBI,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ALLOW_MCC_GO_DIFF_BI_DEFAULT,
                 CFG_ALLOW_MCC_GO_DIFF_BI_MIN,
                 CFG_ALLOW_MCC_GO_DIFF_BI_MAX ),

   REG_VARIABLE( CFG_THERMAL_MIGRATION_ENABLE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, thermalMitigationEnable,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THERMAL_MIGRATION_ENABLE_DEFAULT,
                 CFG_THERMAL_MIGRATION_ENABLE_MIN,
                 CFG_THERMAL_MIGRATION_ENABLE_MAX ),

   REG_VARIABLE( CFG_THROTTLE_PERIOD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, throttlePeriod,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THROTTLE_PERIOD_DEFAULT,
                 CFG_THROTTLE_PERIOD_MIN,
                 CFG_THROTTLE_PERIOD_MAX ),

   REG_VARIABLE( CFG_ENABLE_MODULATED_DTIM_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableModulatedDTIM,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_MODULATED_DTIM_DEFAULT,
                 CFG_ENABLE_MODULATED_DTIM_MIN,
                 CFG_ENABLE_MODULATED_DTIM_MAX ),

   REG_VARIABLE( CFG_MC_ADDR_LIST_ENABLE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableMCAddrList,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MC_ADDR_LIST_ENABLE_DEFAULT,
                 CFG_MC_ADDR_LIST_ENABLE_MIN,
                 CFG_MC_ADDR_LIST_ENABLE_MAX ),

#ifdef WLAN_FEATURE_11AC
   REG_VARIABLE( CFG_VHT_CHANNEL_WIDTH, WLAN_PARAM_Integer,
                 hdd_config_t, vhtChannelWidth,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_VHT_CHANNEL_WIDTH_DEFAULT,
                 CFG_VHT_CHANNEL_WIDTH_MIN,
                 CFG_VHT_CHANNEL_WIDTH_MAX),

   REG_VARIABLE( CFG_VHT_ENABLE_RX_MCS_8_9, WLAN_PARAM_Integer,
                 hdd_config_t, vhtRxMCS,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_VHT_ENABLE_RX_MCS_8_9_DEFAULT,
                 CFG_VHT_ENABLE_RX_MCS_8_9_MIN,
                 CFG_VHT_ENABLE_RX_MCS_8_9_MAX),

   REG_VARIABLE( CFG_VHT_ENABLE_TX_MCS_8_9, WLAN_PARAM_Integer,
                 hdd_config_t, vhtTxMCS,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_VHT_ENABLE_TX_MCS_8_9_DEFAULT,
                 CFG_VHT_ENABLE_TX_MCS_8_9_MIN,
                 CFG_VHT_ENABLE_TX_MCS_8_9_MAX),

   REG_VARIABLE( CFG_VHT_ENABLE_RX_MCS2x2_8_9, WLAN_PARAM_Integer,
                 hdd_config_t, vhtRxMCS2x2,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_VHT_ENABLE_RX_MCS2x2_8_9_DEFAULT,
                 CFG_VHT_ENABLE_RX_MCS2x2_8_9_MIN,
                 CFG_VHT_ENABLE_RX_MCS2x2_8_9_MAX),

   REG_VARIABLE( CFG_VHT_ENABLE_TX_MCS2x2_8_9, WLAN_PARAM_Integer,
                 hdd_config_t, vhtTxMCS2x2,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_VHT_ENABLE_TX_MCS2x2_8_9_DEFAULT,
                 CFG_VHT_ENABLE_TX_MCS2x2_8_9_MIN,
                 CFG_VHT_ENABLE_TX_MCS2x2_8_9_MAX),

   REG_VARIABLE( CFG_VHT_ENABLE_2x2_CAP_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, enable2x2,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VHT_ENABLE_2x2_CAP_FEATURE_DEFAULT,
                 CFG_VHT_ENABLE_2x2_CAP_FEATURE_MIN,
                 CFG_VHT_ENABLE_2x2_CAP_FEATURE_MAX ),

   REG_VARIABLE( CFG_VHT_ENABLE_MU_BFORMEE_CAP_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, enableMuBformee,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VHT_ENABLE_MU_BFORMEE_CAP_FEATURE_DEFAULT,
                 CFG_VHT_ENABLE_MU_BFORMEE_CAP_FEATURE_MIN,
                 CFG_VHT_ENABLE_MU_BFORMEE_CAP_FEATURE_MAX ),

   REG_VARIABLE( CFG_VHT_ENABLE_PAID_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, enableVhtpAid,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VHT_ENABLE_PAID_FEATURE_DEFAULT,
                 CFG_VHT_ENABLE_PAID_FEATURE_MIN,
                 CFG_VHT_ENABLE_PAID_FEATURE_MAX ),

   REG_VARIABLE( CFG_VHT_ENABLE_GID_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, enableVhtGid,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VHT_ENABLE_GID_FEATURE_DEFAULT,
                 CFG_VHT_ENABLE_GID_FEATURE_MIN,
                 CFG_VHT_ENABLE_GID_FEATURE_MAX ),
#endif

   REG_VARIABLE( CFG_VHT_ENABLE_1x1_TX_CHAINMASK, WLAN_PARAM_Integer,
                 hdd_config_t, txchainmask1x1,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VHT_ENABLE_1x1_TX_CHAINMASK_DEFAULT,
                 CFG_VHT_ENABLE_1x1_TX_CHAINMASK_MIN,
                 CFG_VHT_ENABLE_1x1_TX_CHAINMASK_MAX ),

   REG_VARIABLE( CFG_VHT_ENABLE_1x1_RX_CHAINMASK, WLAN_PARAM_Integer,
                 hdd_config_t, rxchainmask1x1,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VHT_ENABLE_1x1_RX_CHAINMASK_DEFAULT,
                 CFG_VHT_ENABLE_1x1_RX_CHAINMASK_MIN,
                 CFG_VHT_ENABLE_1x1_RX_CHAINMASK_MAX ),

   REG_VARIABLE( CFG_ENABLE_AMPDUPS_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, enableAmpduPs,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_AMPDUPS_FEATURE_DEFAULT,
                 CFG_ENABLE_AMPDUPS_FEATURE_MIN,
                 CFG_ENABLE_AMPDUPS_FEATURE_MAX ),

   REG_VARIABLE( CFG_HT_ENABLE_SMPS_CAP_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, enableHtSmps,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_HT_ENABLE_SMPS_CAP_FEATURE_DEFAULT,
                 CFG_HT_ENABLE_SMPS_CAP_FEATURE_MIN,
                 CFG_HT_ENABLE_SMPS_CAP_FEATURE_MAX ),

   REG_VARIABLE( CFG_HT_SMPS_CAP_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, htSmps,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_HT_SMPS_CAP_FEATURE_DEFAULT,
                 CFG_HT_SMPS_CAP_FEATURE_MIN,
                 CFG_HT_SMPS_CAP_FEATURE_MAX ),

   REG_VARIABLE( CFG_DISABLE_DFS_CH_SWITCH, WLAN_PARAM_Integer,
                 hdd_config_t, disableDFSChSwitch,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_DISABLE_DFS_CH_SWITCH_DEFAULT,
                 CFG_DISABLE_DFS_CH_SWITCH_MIN,
                 CFG_DISABLE_DFS_CH_SWITCH_MAX ),

   REG_VARIABLE( CFG_ENABLE_DFS_MASTER_CAPABILITY, WLAN_PARAM_Integer,
                 hdd_config_t, enableDFSMasterCap,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_DFS_MASTER_CAPABILITY_DEFAULT,
                 CFG_ENABLE_DFS_MASTER_CAPABILITY_MIN,
                 CFG_ENABLE_DFS_MASTER_CAPABILITY_MAX ),

   REG_DYNAMIC_VARIABLE( CFG_SAP_PREFERRED_CHANNEL_LOCATION, WLAN_PARAM_Integer,
                 hdd_config_t, gSapPreferredChanLocation,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SAP_PREFERRED_CHANNEL_LOCATION_DEFAULT,
                 CFG_SAP_PREFERRED_CHANNEL_LOCATION_MIN,
                 CFG_SAP_PREFERRED_CHANNEL_LOCATION_MAX,
                 cbNotify_set_gSapPreferredChanLocation, 0),

   REG_DYNAMIC_VARIABLE( CFG_DISABLE_DFS_JAPAN_W53, WLAN_PARAM_Integer,
                 hdd_config_t, gDisableDfsJapanW53,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_DISABLE_DFS_JAPAN_W53_DEFAULT,
                 CFG_DISABLE_DFS_JAPAN_W53_MIN,
                 CFG_DISABLE_DFS_JAPAN_W53_MAX,
                 chNotify_set_gDisableDfsJapanW53, 0),

   REG_VARIABLE( CFG_ENABLE_FIRST_SCAN_2G_ONLY_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableFirstScan2GOnly,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_FIRST_SCAN_2G_ONLY_DEFAULT,
                 CFG_ENABLE_FIRST_SCAN_2G_ONLY_MIN,
                 CFG_ENABLE_FIRST_SCAN_2G_ONLY_MAX ),

   REG_VARIABLE( CFG_ENABLE_SKIP_DFS_IN_P2P_SEARCH_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, skipDfsChnlInP2pSearch,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_SKIP_DFS_IN_P2P_SEARCH_DEFAULT,
                 CFG_ENABLE_SKIP_DFS_IN_P2P_SEARCH_MIN,
                 CFG_ENABLE_SKIP_DFS_IN_P2P_SEARCH_MAX ),

   REG_VARIABLE( CFG_IGNORE_DYNAMIC_DTIM_IN_P2P_MODE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ignoreDynamicDtimInP2pMode,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IGNORE_DYNAMIC_DTIM_IN_P2P_MODE_DEFAULT,
                 CFG_IGNORE_DYNAMIC_DTIM_IN_P2P_MODE_MIN,
                 CFG_IGNORE_DYNAMIC_DTIM_IN_P2P_MODE_MAX ),

   REG_VARIABLE( CFG_NUM_BUFF_ADVERT_NAME, WLAN_PARAM_Integer,
                 hdd_config_t,numBuffAdvert ,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_NUM_BUFF_ADVERT_DEFAULT,
                 CFG_NUM_BUFF_ADVERT_MIN,
                 CFG_NUM_BUFF_ADVERT_MAX ),

   REG_VARIABLE( CFG_MCC_CONFIG_PARAM_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, configMccParam,
                 VAR_FLAGS_OPTIONAL,
                 CFG_MCC_CONFIG_PARAM_DEFAULT,
                 CFG_MCC_CONFIG_PARAM_MIN,
                 CFG_MCC_CONFIG_PARAM_MAX ),

   REG_VARIABLE( CFG_ENABLE_RX_STBC, WLAN_PARAM_Integer,
                 hdd_config_t, enableRxSTBC,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_RX_STBC_DEFAULT,
                 CFG_ENABLE_RX_STBC_MIN,
                 CFG_ENABLE_RX_STBC_MAX ),

   REG_VARIABLE( CFG_ENABLE_TX_STBC, WLAN_PARAM_Integer,
                 hdd_config_t, enableTxSTBC,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_TX_STBC_DEFAULT,
                 CFG_ENABLE_TX_STBC_MIN,
                 CFG_ENABLE_TX_STBC_MAX ),

   REG_VARIABLE( CFG_ENABLE_RX_LDPC, WLAN_PARAM_Integer,
                 hdd_config_t, enableRxLDPC,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_RX_LDPC_DEFAULT,
                 CFG_ENABLE_RX_LDPC_MIN,
                 CFG_ENABLE_RX_LDPC_MAX ),

   REG_VARIABLE( CFG_PPS_ENABLE_5G_EBT, WLAN_PARAM_Integer,
                 hdd_config_t, enable5gEBT,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_PPS_ENABLE_5G_EBT_FEATURE_DEFAULT,
                 CFG_PPS_ENABLE_5G_EBT_FEATURE_MIN,
                 CFG_PPS_ENABLE_5G_EBT_FEATURE_MAX ),

#ifdef FEATURE_WLAN_TDLS
   REG_VARIABLE( CFG_TDLS_SUPPORT_ENABLE, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableTDLSSupport,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_SUPPORT_ENABLE_DEFAULT,
                 CFG_TDLS_SUPPORT_ENABLE_MIN,
                 CFG_TDLS_SUPPORT_ENABLE_MAX ),

   REG_VARIABLE( CFG_TDLS_IMPLICIT_TRIGGER, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableTDLSImplicitTrigger,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_IMPLICIT_TRIGGER_DEFAULT,
                 CFG_TDLS_IMPLICIT_TRIGGER_MIN,
                 CFG_TDLS_IMPLICIT_TRIGGER_MAX ),

   REG_VARIABLE( CFG_TDLS_TX_STATS_PERIOD, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSTxStatsPeriod,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_TX_STATS_PERIOD_DEFAULT,
                 CFG_TDLS_TX_STATS_PERIOD_MIN,
                 CFG_TDLS_TX_STATS_PERIOD_MAX ),

   REG_VARIABLE( CFG_TDLS_TX_PACKET_THRESHOLD, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSTxPacketThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_TX_PACKET_THRESHOLD_DEFAULT,
                 CFG_TDLS_TX_PACKET_THRESHOLD_MIN,
                 CFG_TDLS_TX_PACKET_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_TDLS_DISCOVERY_PERIOD, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSDiscoveryPeriod,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_DISCOVERY_PERIOD_DEFAULT,
                 CFG_TDLS_DISCOVERY_PERIOD_MIN,
                 CFG_TDLS_DISCOVERY_PERIOD_MAX ),

   REG_VARIABLE( CFG_TDLS_MAX_DISCOVERY_ATTEMPT, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSMaxDiscoveryAttempt,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_MAX_DISCOVERY_ATTEMPT_DEFAULT,
                 CFG_TDLS_MAX_DISCOVERY_ATTEMPT_MIN,
                 CFG_TDLS_MAX_DISCOVERY_ATTEMPT_MAX ),

   REG_VARIABLE( CFG_TDLS_IDLE_TIMEOUT, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSIdleTimeout,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_IDLE_TIMEOUT_DEFAULT,
                 CFG_TDLS_IDLE_TIMEOUT_MIN,
                 CFG_TDLS_IDLE_TIMEOUT_MAX ),

   REG_VARIABLE( CFG_TDLS_IDLE_PACKET_THRESHOLD, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSIdlePacketThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_IDLE_PACKET_THRESHOLD_DEFAULT,
                 CFG_TDLS_IDLE_PACKET_THRESHOLD_MIN,
                 CFG_TDLS_IDLE_PACKET_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_TDLS_RSSI_HYSTERESIS, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSRSSIHysteresis,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_RSSI_HYSTERESIS_DEFAULT,
                 CFG_TDLS_RSSI_HYSTERESIS_MIN,
                 CFG_TDLS_RSSI_HYSTERESIS_MAX ),

   REG_VARIABLE( CFG_TDLS_RSSI_TRIGGER_THRESHOLD, WLAN_PARAM_SignedInteger,
                 hdd_config_t, fTDLSRSSITriggerThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_RSSI_TRIGGER_THRESHOLD_DEFAULT,
                 CFG_TDLS_RSSI_TRIGGER_THRESHOLD_MIN,
                 CFG_TDLS_RSSI_TRIGGER_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_TDLS_RSSI_TEARDOWN_THRESHOLD, WLAN_PARAM_SignedInteger,
                 hdd_config_t, fTDLSRSSITeardownThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_RSSI_TEARDOWN_THRESHOLD_DEFAULT,
                 CFG_TDLS_RSSI_TEARDOWN_THRESHOLD_MIN,
                 CFG_TDLS_RSSI_TEARDOWN_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_TDLS_RSSI_DELTA, WLAN_PARAM_SignedInteger,
                 hdd_config_t, fTDLSRSSIDelta,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_RSSI_DELTA_DEFAULT,
                 CFG_TDLS_RSSI_DELTA_MIN,
                 CFG_TDLS_RSSI_DELTA_MAX ),

   REG_VARIABLE( CFG_TDLS_QOS_WMM_UAPSD_MASK_NAME , WLAN_PARAM_HexInteger,
                 hdd_config_t, fTDLSUapsdMask,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_QOS_WMM_UAPSD_MASK_DEFAULT,
                 CFG_TDLS_QOS_WMM_UAPSD_MASK_MIN,
                 CFG_TDLS_QOS_WMM_UAPSD_MASK_MAX ),

   REG_VARIABLE( CFG_TDLS_BUFFER_STA_SUPPORT_ENABLE, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableTDLSBufferSta,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_BUFFER_STA_SUPPORT_ENABLE_DEFAULT,
                 CFG_TDLS_BUFFER_STA_SUPPORT_ENABLE_MIN,
                 CFG_TDLS_BUFFER_STA_SUPPORT_ENABLE_MAX ),

   REG_VARIABLE( CFG_TDLS_OFF_CHANNEL_SUPPORT_ENABLE, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableTDLSOffChannel,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_OFF_CHANNEL_SUPPORT_ENABLE_DEFAULT,
                 CFG_TDLS_OFF_CHANNEL_SUPPORT_ENABLE_MIN,
                 CFG_TDLS_OFF_CHANNEL_SUPPORT_ENABLE_MAX ),

   REG_VARIABLE( CFG_TDLS_PREFERRED_OFF_CHANNEL_NUM, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSPrefOffChanNum,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_PREFERRED_OFF_CHANNEL_NUM_DEFAULT,
                 CFG_TDLS_PREFERRED_OFF_CHANNEL_NUM_MIN,
                 CFG_TDLS_PREFERRED_OFF_CHANNEL_NUM_MAX ),

   REG_VARIABLE( CFG_TDLS_PREFERRED_OFF_CHANNEL_BW, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSPrefOffChanBandwidth,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_PREFERRED_OFF_CHANNEL_BW_DEFAULT,
                 CFG_TDLS_PREFERRED_OFF_CHANNEL_BW_MIN,
                 CFG_TDLS_PREFERRED_OFF_CHANNEL_BW_MAX ),

   REG_VARIABLE( CFG_TDLS_PUAPSD_INACTIVITY_TIME, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSPuapsdInactivityTimer,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_PUAPSD_INACTIVITY_TIME_DEFAULT,
                 CFG_TDLS_PUAPSD_INACTIVITY_TIME_MIN,
                 CFG_TDLS_PUAPSD_INACTIVITY_TIME_MAX ),

   REG_VARIABLE( CFG_TDLS_PUAPSD_RX_FRAME_THRESHOLD, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSRxFrameThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_PUAPSD_RX_FRAME_THRESHOLD_DEFAULT,
                 CFG_TDLS_PUAPSD_RX_FRAME_THRESHOLD_MIN,
                 CFG_TDLS_PUAPSD_RX_FRAME_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_TDLS_PUAPSD_PEER_TRAFFIC_IND_WINDOW, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSPuapsdPTIWindow,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_PUAPSD_PEER_TRAFFIC_IND_WINDOW_DEFAULT,
                 CFG_TDLS_PUAPSD_PEER_TRAFFIC_IND_WINDOW_MIN,
                 CFG_TDLS_PUAPSD_PEER_TRAFFIC_IND_WINDOW_MAX ),

   REG_VARIABLE( CFG_TDLS_PUAPSD_PEER_TRAFFIC_RSP_TIMEOUT, WLAN_PARAM_Integer,
                 hdd_config_t, fTDLSPuapsdPTRTimeout,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_PUAPSD_PEER_TRAFFIC_RSP_TIMEOUT_DEFAULT,
                 CFG_TDLS_PUAPSD_PEER_TRAFFIC_RSP_TIMEOUT_MIN,
                 CFG_TDLS_PUAPSD_PEER_TRAFFIC_RSP_TIMEOUT_MAX ),

    REG_VARIABLE( CFG_TDLS_EXTERNAL_CONTROL, WLAN_PARAM_Integer,
                  hdd_config_t, fTDLSExternalControl,
                  VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                  CFG_TDLS_EXTERNAL_CONTROL_DEFAULT,
                  CFG_TDLS_EXTERNAL_CONTROL_MIN,
                  CFG_TDLS_EXTERNAL_CONTROL_MAX ),

   REG_VARIABLE( CFG_TDLS_WMM_MODE_ENABLE, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableTDLSWmmMode,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TDLS_WMM_MODE_ENABLE_DEFAULT,
                 CFG_TDLS_WMM_MODE_ENABLE_MIN,
                 CFG_TDLS_WMM_MODE_ENABLE_MAX ),
#endif

#ifdef WLAN_SOFTAP_VSTA_FEATURE
   REG_VARIABLE( CFG_VSTA_SUPPORT_ENABLE, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableVSTASupport,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VSTA_SUPPORT_ENABLE_DEFAULT,
                 CFG_VSTA_SUPPORT_ENABLE_MIN,
                 CFG_VSTA_SUPPORT_ENABLE_MAX ),
#endif
   REG_VARIABLE( CFG_ENABLE_LPWR_IMG_TRANSITION_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableLpwrImgTransition,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_LPWR_IMG_TRANSITION_DEFAULT,
                 CFG_ENABLE_LPWR_IMG_TRANSITION_MIN,
                 CFG_ENABLE_LPWR_IMG_TRANSITION_MAX ),

#ifdef WLAN_ACTIVEMODE_OFFLOAD_FEATURE
   REG_VARIABLE( CFG_ACTIVEMODE_OFFLOAD_ENABLE, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableActiveModeOffload,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ACTIVEMODE_OFFLOAD_ENABLE_DEFAULT,
                 CFG_ACTIVEMODE_OFFLOAD_ENABLE_MIN,
                 CFG_ACTIVEMODE_OFFLOAD_ENABLE_MAX ),
#endif
   REG_VARIABLE( CFG_ENABLE_LPWR_IMG_TRANSITION_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableLpwrImgTransition,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_LPWR_IMG_TRANSITION_DEFAULT,
                 CFG_ENABLE_LPWR_IMG_TRANSITION_MIN,
                 CFG_ENABLE_LPWR_IMG_TRANSITION_MAX ),


   REG_VARIABLE( CFG_SCAN_AGING_PARAM_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, scanAgingTimeout,
                 VAR_FLAGS_OPTIONAL,
                 CFG_SCAN_AGING_PARAM_DEFAULT,
                 CFG_SCAN_AGING_PARAM_MIN,
                 CFG_SCAN_AGING_PARAM_MAX ),

   REG_VARIABLE( CFG_TX_LDPC_ENABLE_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, enableTxLdpc,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_TX_LDPC_ENABLE_FEATURE_DEFAULT,
                 CFG_TX_LDPC_ENABLE_FEATURE_MIN,
                 CFG_TX_LDPC_ENABLE_FEATURE_MAX ),

   REG_VARIABLE( CFG_ENABLE_MCC_ADATIVE_SCHEDULER_ENABLED_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableMCCAdaptiveScheduler,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_MCC_ADATIVE_SCHEDULER_ENABLED_DEFAULT,
                 CFG_ENABLE_MCC_ADATIVE_SCHEDULER_ENABLED_MIN,
                 CFG_ENABLE_MCC_ADATIVE_SCHEDULER_ENABLED_MAX ),

   REG_VARIABLE( CFG_ANDRIOD_POWER_SAVE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, isAndroidPsEn,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ANDRIOD_POWER_SAVE_DEFAULT,
                 CFG_ANDRIOD_POWER_SAVE_MIN,
                 CFG_ANDRIOD_POWER_SAVE_MAX),

   REG_VARIABLE( CFG_IBSS_ADHOC_CHANNEL_5GHZ_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, AdHocChannel5G,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_ADHOC_CHANNEL_5GHZ_DEFAULT,
                 CFG_IBSS_ADHOC_CHANNEL_5GHZ_MIN,
                 CFG_IBSS_ADHOC_CHANNEL_5GHZ_MAX),

   REG_VARIABLE( CFG_IBSS_ADHOC_CHANNEL_24GHZ_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, AdHocChannel24G,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_ADHOC_CHANNEL_24GHZ_DEFAULT,
                 CFG_IBSS_ADHOC_CHANNEL_24GHZ_MIN,
                 CFG_IBSS_ADHOC_CHANNEL_24GHZ_MAX),

#ifdef WLAN_FEATURE_11AC
   REG_VARIABLE( CFG_VHT_SU_BEAMFORMEE_CAP_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, enableTxBF,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VHT_SU_BEAMFORMEE_CAP_FEATURE_DEFAULT,
                 CFG_VHT_SU_BEAMFORMEE_CAP_FEATURE_MIN,
                 CFG_VHT_SU_BEAMFORMEE_CAP_FEATURE_MAX ),

   REG_VARIABLE( CFG_VHT_ENABLE_TXBF_IN_20MHZ, WLAN_PARAM_Integer,
                 hdd_config_t, enableTxBFin20MHz,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VHT_ENABLE_TXBF_IN_20MHZ_DEFAULT,
                 CFG_VHT_ENABLE_TXBF_IN_20MHZ_MIN,
                 CFG_VHT_ENABLE_TXBF_IN_20MHZ_MAX ),

   REG_VARIABLE( CFG_VHT_CSN_BEAMFORMEE_ANT_SUPPORTED, WLAN_PARAM_Integer,
                 hdd_config_t, txBFCsnValue,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_VHT_CSN_BEAMFORMEE_ANT_SUPPORTED_DEFAULT,
                 CFG_VHT_CSN_BEAMFORMEE_ANT_SUPPORTED_MIN,
                 CFG_VHT_CSN_BEAMFORMEE_ANT_SUPPORTED_MAX ),
#endif

   REG_VARIABLE( CFG_SAP_ALLOW_ALL_CHANNEL_PARAM_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, sapAllowAllChannel,
                 VAR_FLAGS_OPTIONAL,
                 CFG_SAP_ALLOW_ALL_CHANNEL_PARAM_DEFAULT,
                 CFG_SAP_ALLOW_ALL_CHANNEL_PARAM_MIN,
                 CFG_SAP_ALLOW_ALL_CHANNEL_PARAM_MAX ),

#ifdef WLAN_FEATURE_11AC
   REG_VARIABLE( CFG_DISABLE_LDPC_WITH_TXBF_AP, WLAN_PARAM_Integer,
                 hdd_config_t, disableLDPCWithTxbfAP,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_DISABLE_LDPC_WITH_TXBF_AP_DEFAULT,
                 CFG_DISABLE_LDPC_WITH_TXBF_AP_MIN,
                 CFG_DISABLE_LDPC_WITH_TXBF_AP_MAX ),
#endif

   REG_VARIABLE_STRING( CFG_LIST_OF_NON_DFS_COUNTRY_CODE, WLAN_PARAM_String,
                        hdd_config_t, listOfNonDfsCountryCode,
                        VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                        (void *)CFG_LIST_OF_NON_DFS_COUNTRY_CODE_DEFAULT),

   REG_DYNAMIC_VARIABLE( CFG_ENABLE_SSR, WLAN_PARAM_Integer,
                         hdd_config_t, enableSSR,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ENABLE_SSR_DEFAULT,
                         CFG_ENABLE_SSR_MIN,
                         CFG_ENABLE_SSR_MAX,
                         cbNotifySetEnableSSR, 0 ),

   REG_VARIABLE( CFG_MAX_MEDIUM_TIME, WLAN_PARAM_Integer,
                 hdd_config_t, cfgMaxMediumTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_MAX_MEDIUM_TIME_STADEFAULT,
                 CFG_MAX_MEDIUM_TIME_STAMIN,
                 CFG_MAX_MEDIUM_TIME_STAMAX ),

#ifdef WLAN_FEATURE_11AC
   REG_VARIABLE( CFG_ENABLE_VHT_FOR_24GHZ_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableVhtFor24GHzBand,
                 VAR_FLAGS_OPTIONAL,
                 CFG_ENABLE_VHT_FOR_24GHZ_DEFAULT,
                 CFG_ENABLE_VHT_FOR_24GHZ_MIN,
                 CFG_ENABLE_VHT_FOR_24GHZ_MAX),
#endif

   REG_VARIABLE( CFG_SCAN_OFFLOAD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fScanOffload,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SCAN_OFFLOAD_DEFAULT,
                 CFG_SCAN_OFFLOAD_DISABLE,
                 CFG_SCAN_OFFLOAD_ENABLE ),

#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
   REG_DYNAMIC_VARIABLE( CFG_ENABLE_FAST_ROAM_IN_CONCURRENCY, WLAN_PARAM_Integer,
                         hdd_config_t, bFastRoamInConIniFeatureEnabled,
                         VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                         CFG_ENABLE_FAST_ROAM_IN_CONCURRENCY_DEFAULT,
                         CFG_ENABLE_FAST_ROAM_IN_CONCURRENCY_MIN,
                         CFG_ENABLE_FAST_ROAM_IN_CONCURRENCY_MAX,
                         cbNotifySetEnableFastRoamInConcurrency, 0 ),
#endif

   REG_VARIABLE( CFG_ENABLE_ADAPT_RX_DRAIN_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableAdaptRxDrain,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK ,
                 CFG_ENABLE_ADAPT_RX_DRAIN_DEFAULT,
                 CFG_ENABLE_ADAPT_RX_DRAIN_MIN,
                 CFG_ENABLE_ADAPT_RX_DRAIN_MAX),

   REG_VARIABLE( CFG_FLEX_CONNECT_POWER_FACTOR_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, flexConnectPowerFactor,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_MINMAX,
                 CFG_FLEX_CONNECT_POWER_FACTOR_DEFAULT,
                 CFG_FLEX_CONNECT_POWER_FACTOR_MIN,
                 CFG_FLEX_CONNECT_POWER_FACTOR_MAX ),

   REG_VARIABLE( CFG_ENABLE_HEART_BEAT_OFFLOAD, WLAN_PARAM_Integer,
                 hdd_config_t, enableIbssHeartBeatOffload,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_HEART_BEAT_OFFLOAD_DEFAULT,
                 CFG_ENABLE_HEART_BEAT_OFFLOAD_MIN,
                 CFG_ENABLE_HEART_BEAT_OFFLOAD_MAX),

   REG_VARIABLE( CFG_ANTENNA_DIVERSITY_PARAM_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, antennaDiversity,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ANTENNA_DIVERSITY_PARAM_DEFAULT,
                 CFG_ANTENNA_DIVERSITY_PARAM_MIN,
                 CFG_ANTENNA_DIVERSITY_PARAM_MAX),

   REG_VARIABLE( CFG_ENABLE_SNR_MONITORING_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fEnableSNRMonitoring,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK ,
                 CFG_ENABLE_SNR_MONITORING_DEFAULT,
                 CFG_ENABLE_SNR_MONITORING_MIN,
                 CFG_ENABLE_SNR_MONITORING_MAX),

#ifdef FEATURE_WLAN_SCAN_PNO
   REG_VARIABLE( CFG_PNO_SCAN_SUPPORT, WLAN_PARAM_Integer,
                 hdd_config_t, configPNOScanSupport,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_PNO_SCAN_SUPPORT_DEFAULT,
                 CFG_PNO_SCAN_SUPPORT_DISABLE,
                 CFG_PNO_SCAN_SUPPORT_ENABLE),

   REG_VARIABLE( CFG_PNO_SCAN_TIMER_REPEAT_VALUE, WLAN_PARAM_Integer,
                 hdd_config_t, configPNOScanTimerRepeatValue,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_PNO_SCAN_TIMER_REPEAT_VALUE_DEFAULT,
                 CFG_PNO_SCAN_TIMER_REPEAT_VALUE_MIN,
                 CFG_PNO_SCAN_TIMER_REPEAT_VALUE_MAX),
#endif
   REG_VARIABLE( CFG_AMSDU_SUPPORT_IN_AMPDU_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, isAmsduSupportInAMPDU,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_AMSDU_SUPPORT_IN_AMPDU_DEFAULT,
                 CFG_AMSDU_SUPPORT_IN_AMPDU_MIN,
                 CFG_AMSDU_SUPPORT_IN_AMPDU_MAX ),

   REG_VARIABLE( CFG_STRICT_5GHZ_PREF_BY_MARGIN , WLAN_PARAM_Integer,
                 hdd_config_t, nSelect5GHzMargin,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_STRICT_5GHZ_PREF_BY_MARGIN_DEFAULT,
                 CFG_STRICT_5GHZ_PREF_BY_MARGIN_MIN,
                 CFG_STRICT_5GHZ_PREF_BY_MARGIN_MAX ),

   REG_VARIABLE( CFG_ENABLE_TCP_CHKSUM_OFFLOAD, WLAN_PARAM_Integer,
                 hdd_config_t, enableTCPChkSumOffld,
                 VAR_FLAGS_OPTIONAL,
                 CFG_ENABLE_TCP_CHKSUM_OFFLOAD_DEFAULT,
                 CFG_ENABLE_TCP_CHKSUM_OFFLOAD_MIN,
                 CFG_ENABLE_TCP_CHKSUM_OFFLOAD_MAX),

   REG_VARIABLE( CFG_ENABLE_IP_CHKSUM_OFFLOAD, WLAN_PARAM_Integer,
                 hdd_config_t, enableIPChecksumOffload,
                 VAR_FLAGS_OPTIONAL,
                 CFG_ENABLE_IP_CHKSUM_OFFLOAD_DEFAULT,
                 CFG_ENABLE_IP_CHKSUM_OFFLOAD_DISABLE,
                 CFG_ENABLE_IP_CHKSUM_OFFLOAD_ENABLE ),

   REG_VARIABLE( CFG_POWERSAVE_OFFLOAD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enablePowersaveOffload,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_POWERSAVE_OFFLOAD_DEFAULT,
                 CFG_POWERSAVE_OFFLOAD_MIN,
                 CFG_POWERSAVE_OFFLOAD_MAX ),

   REG_VARIABLE( CFG_ENABLE_FW_UART_PRINT_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enablefwprint,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_FW_UART_PRINT_DEFAULT,
                 CFG_ENABLE_FW_UART_PRINT_DISABLE,
                 CFG_ENABLE_FW_UART_PRINT_ENABLE),

   REG_VARIABLE( CFG_ENABLE_FW_LOG_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enablefwlog,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_FW_LOG_DEFAULT,
                 CFG_ENABLE_FW_LOG_DISABLE,
                 CFG_ENABLE_FW_LOG_ENABLE),

   REG_VARIABLE( CFG_ENABLE_FW_SELF_RECOVERY_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableFwSelfRecovery,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_FW_SELF_RECOVERY_DEFAULT,
                 CFG_ENABLE_FW_SELF_RECOVERY_DISABLE,
                 CFG_ENABLE_FW_SELF_RECOVERY_ENABLE),

#ifdef IPA_OFFLOAD
   REG_VARIABLE( CFG_IPA_OFFLOAD_CONFIG_NAME, WLAN_PARAM_HexInteger,
                 hdd_config_t, IpaConfig,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IPA_OFFLOAD_CONFIG_DEFAULT,
                 CFG_IPA_OFFLOAD_CONFIG_MIN,
                 CFG_IPA_OFFLOAD_CONFIG_MAX),

   REG_VARIABLE( CFG_IPA_DESC_SIZE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, IpaDescSize,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IPA_DESC_SIZE_DEFAULT,
                 CFG_IPA_DESC_SIZE_MIN,
                 CFG_IPA_DESC_SIZE_MAX ),

   REG_VARIABLE( CFG_IPA_HIGH_BANDWIDTH_MBPS, WLAN_PARAM_Integer,
                 hdd_config_t, IpaHighBandwidthMbps,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IPA_HIGH_BANDWIDTH_MBPS_DEFAULT,
                 CFG_IPA_HIGH_BANDWIDTH_MBPS_MIN,
                 CFG_IPA_HIGH_BANDWIDTH_MBPS_MAX),

   REG_VARIABLE( CFG_IPA_MEDIUM_BANDWIDTH_MBPS, WLAN_PARAM_Integer,
                 hdd_config_t, IpaMediumBandwidthMbps,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IPA_MEDIUM_BANDWIDTH_MBPS_DEFAULT,
                 CFG_IPA_MEDIUM_BANDWIDTH_MBPS_MIN,
                 CFG_IPA_MEDIUM_BANDWIDTH_MBPS_MAX),

   REG_VARIABLE( CFG_IPA_LOW_BANDWIDTH_MBPS, WLAN_PARAM_Integer,
                 hdd_config_t, IpaLowBandwidthMbps,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IPA_LOW_BANDWIDTH_MBPS_DEFAULT,
                 CFG_IPA_LOW_BANDWIDTH_MBPS_MIN,
                 CFG_IPA_LOW_BANDWIDTH_MBPS_MAX),
#endif
   REG_VARIABLE( CFG_P2P_LISTEN_OFFLOAD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fP2pListenOffload,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_P2P_LISTEN_OFFLOAD_DEFAULT,
                 CFG_P2P_LISTEN_OFFLOAD_DISABLE,
                 CFG_P2P_LISTEN_OFFLOAD_ENABLE ),

#ifdef WLAN_FEATURE_11AC
   REG_VARIABLE( CFG_VHT_AMPDU_LEN_EXPONENT_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fVhtAmpduLenExponent,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK ,
                 CFG_VHT_AMPDU_LEN_EXPONENT_DEFAULT,
                 CFG_VHT_AMPDU_LEN_EXPONENT_MIN,
                 CFG_VHT_AMPDU_LEN_EXPONENT_MAX),

   REG_VARIABLE( CFG_VHT_MPDU_LEN_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, vhtMpduLen,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK ,
                 CFG_VHT_MPDU_LEN_DEFAULT,
                 CFG_VHT_MPDU_LEN_MIN,
                 CFG_VHT_MPDU_LEN_MAX),
#endif

   REG_VARIABLE( CFG_MAX_WOW_FILTERS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, maxWoWFilters,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK ,
                 CFG_MAX_WOW_FILTERS_DEFAULT,
                 CFG_MAX_WOW_FILTERS_MIN,
                 CFG_MAX_WOW_FILTERS_MAX),

   REG_VARIABLE( CFG_WOW_STATUS_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, wowEnable,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_WOW_STATUS_DEFAULT,
                 CFG_WOW_ENABLE_MIN,
                 CFG_WOW_ENABLE_MAX),

   REG_VARIABLE( CFG_COALESING_IN_IBSS_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, isCoalesingInIBSSAllowed,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_COALESING_IN_IBSS_DEFAULT,
                 CFG_COALESING_IN_IBSS_MIN,
                 CFG_COALESING_IN_IBSS_MAX ),

   REG_VARIABLE( CFG_IBSS_ATIM_WIN_SIZE_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, ibssATIMWinSize,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_ATIM_WIN_SIZE_DEFAULT,
                 CFG_IBSS_ATIM_WIN_SIZE_MIN,
                 CFG_IBSS_ATIM_WIN_SIZE_MAX ),

   REG_VARIABLE( CFG_SAP_MAX_NO_PEERS, WLAN_PARAM_Integer,
                 hdd_config_t, maxNumberOfPeers,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SAP_MAX_NO_PEERS_DEFAULT,
                 CFG_SAP_MAX_NO_PEERS_MIN,
                 CFG_SAP_MAX_NO_PEERS_MAX),

   REG_VARIABLE( CFG_IBSS_IS_POWER_SAVE_ALLOWED_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, isIbssPowerSaveAllowed,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_IS_POWER_SAVE_ALLOWED_DEFAULT,
                 CFG_IBSS_IS_POWER_SAVE_ALLOWED_MIN,
                 CFG_IBSS_IS_POWER_SAVE_ALLOWED_MAX ),

   REG_VARIABLE( CFG_IBSS_IS_POWER_COLLAPSE_ALLOWED_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, isIbssPowerCollapseAllowed,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_IS_POWER_COLLAPSE_ALLOWED_DEFAULT,
                 CFG_IBSS_IS_POWER_COLLAPSE_ALLOWED_MIN,
                 CFG_IBSS_IS_POWER_COLLAPSE_ALLOWED_MAX ),

   REG_VARIABLE( CFG_IBSS_AWAKE_ON_TX_RX_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, isIbssAwakeOnTxRx,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_AWAKE_ON_TX_RX_DEFAULT,
                 CFG_IBSS_AWAKE_ON_TX_RX_MIN,
                 CFG_IBSS_AWAKE_ON_TX_RX_MAX ),

   REG_VARIABLE( CFG_IBSS_INACTIVITY_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ibssInactivityCount,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_INACTIVITY_TIME_DEFAULT,
                 CFG_IBSS_INACTIVITY_TIME_MIN,
                 CFG_IBSS_INACTIVITY_TIME_MAX ),

   REG_VARIABLE( CFG_IBSS_TXSP_END_INACTIVITY_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ibssTxSpEndInactivityTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_TXSP_END_INACTIVITY_DEFAULT,
                 CFG_IBSS_TXSP_END_INACTIVITY_MIN,
                 CFG_IBSS_TXSP_END_INACTIVITY_MAX ),

   REG_VARIABLE( CFG_IBSS_PS_WARMUP_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ibssPsWarmupTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_PS_WARMUP_TIME_DEFAULT,
                 CFG_IBSS_PS_WARMUP_TIME_MIN,
                 CFG_IBSS_PS_WARMUP_TIME_MAX ),

   REG_VARIABLE( CFG_IBSS_PS_1RX_CHAIN_IN_ATIM_WINDOW_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, ibssPs1RxChainInAtimEnable,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_IBSS_PS_1RX_CHAIN_IN_ATIM_WINDOW_DEFAULT,
                 CFG_IBSS_PS_1RX_CHAIN_IN_ATIM_WINDOW_MIN,
                 CFG_IBSS_PS_1RX_CHAIN_IN_ATIM_WINDOW_MAX ),

   REG_VARIABLE( CFG_THERMAL_TEMP_MIN_LEVEL0_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, thermalTempMinLevel0,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THERMAL_TEMP_MIN_LEVEL0_DEFAULT,
                 CFG_THERMAL_TEMP_MIN_LEVEL0_MIN,
                 CFG_THERMAL_TEMP_MIN_LEVEL0_MAX ),

   REG_VARIABLE( CFG_THERMAL_TEMP_MAX_LEVEL0_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, thermalTempMaxLevel0,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THERMAL_TEMP_MAX_LEVEL0_DEFAULT,
                 CFG_THERMAL_TEMP_MAX_LEVEL0_MIN,
                 CFG_THERMAL_TEMP_MAX_LEVEL0_MAX ),

   REG_VARIABLE( CFG_THERMAL_TEMP_MIN_LEVEL1_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, thermalTempMinLevel1,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THERMAL_TEMP_MIN_LEVEL1_DEFAULT,
                 CFG_THERMAL_TEMP_MIN_LEVEL1_MIN,
                 CFG_THERMAL_TEMP_MIN_LEVEL1_MAX ),

   REG_VARIABLE( CFG_THERMAL_TEMP_MAX_LEVEL1_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, thermalTempMaxLevel1,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THERMAL_TEMP_MAX_LEVEL1_DEFAULT,
                 CFG_THERMAL_TEMP_MAX_LEVEL1_MIN,
                 CFG_THERMAL_TEMP_MAX_LEVEL1_MAX ),

   REG_VARIABLE( CFG_THERMAL_TEMP_MIN_LEVEL2_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, thermalTempMinLevel2,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THERMAL_TEMP_MIN_LEVEL2_DEFAULT,
                 CFG_THERMAL_TEMP_MIN_LEVEL2_MIN,
                 CFG_THERMAL_TEMP_MIN_LEVEL2_MAX ),

   REG_VARIABLE( CFG_THERMAL_TEMP_MAX_LEVEL2_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, thermalTempMaxLevel2,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THERMAL_TEMP_MAX_LEVEL2_DEFAULT,
                 CFG_THERMAL_TEMP_MAX_LEVEL2_MIN,
                 CFG_THERMAL_TEMP_MAX_LEVEL2_MAX ),

   REG_VARIABLE( CFG_THERMAL_TEMP_MIN_LEVEL3_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, thermalTempMinLevel3,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THERMAL_TEMP_MIN_LEVEL3_DEFAULT,
                 CFG_THERMAL_TEMP_MIN_LEVEL3_MIN,
                 CFG_THERMAL_TEMP_MIN_LEVEL3_MAX ),

   REG_VARIABLE( CFG_THERMAL_TEMP_MAX_LEVEL3_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, thermalTempMaxLevel3,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_THERMAL_TEMP_MAX_LEVEL3_DEFAULT,
                 CFG_THERMAL_TEMP_MAX_LEVEL3_MIN,
                 CFG_THERMAL_TEMP_MAX_LEVEL3_MAX ),

  REG_VARIABLE( CFG_SET_TXPOWER_LIMIT2G_NAME , WLAN_PARAM_Integer,
                hdd_config_t, TxPower2g,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_SET_TXPOWER_LIMIT2G_DEFAULT,
                CFG_SET_TXPOWER_LIMIT2G_MIN,
                CFG_SET_TXPOWER_LIMIT2G_MAX ),

  REG_VARIABLE( CFG_SET_TXPOWER_LIMIT5G_NAME , WLAN_PARAM_Integer,
                hdd_config_t, TxPower5g,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_SET_TXPOWER_LIMIT5G_DEFAULT,
                CFG_SET_TXPOWER_LIMIT5G_MIN,
                CFG_SET_TXPOWER_LIMIT5G_MAX ),

   REG_VARIABLE( CFG_ENABLE_DEBUG_CONNECT_ISSUE, WLAN_PARAM_Integer,
                 hdd_config_t, gEnableDebugLog,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_DEBUG_CONNECT_ISSUE_DEFAULT,
                 CFG_ENABLE_DEBUG_CONNECT_ISSUE_MIN ,
                 CFG_ENABLE_DEBUG_CONNECT_ISSUE_MAX),

   REG_VARIABLE( CFG_ENABLE_RX_THREAD, WLAN_PARAM_Integer,
                 hdd_config_t, enableRxThread,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_RX_THREAD_DEFAULT,
                 CFG_ENABLE_RX_THREAD_MIN,
                 CFG_ENABLE_RX_THREAD_MAX),

   REG_VARIABLE( CFG_ENABLE_DFS_PHYERR_FILTEROFFLOAD_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fDfsPhyerrFilterOffload,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_DFS_PHYERR_FILTEROFFLOAD_DEFAULT,
                 CFG_ENABLE_DFS_PHYERR_FILTEROFFLOAD_MIN,
                 CFG_ENABLE_DFS_PHYERR_FILTEROFFLOAD_MAX ),

   REG_VARIABLE( CFG_ENABLE_OVERLAP_CH, WLAN_PARAM_Integer,
                 hdd_config_t, gEnableOverLapCh,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_ENABLE_OVERLAP_CH_DEFAULT,
                 CFG_ENABLE_OVERLAP_CH_MIN,
                 CFG_ENABLE_OVERLAP_CH_MAX ),

   REG_VARIABLE_STRING( CFG_ONLY_ALLOWED_CHANNELS, WLAN_PARAM_String,
                        hdd_config_t, acsAllowedChnls,
#ifndef WLAN_FEATURE_MBSSID
                        VAR_FLAGS_DYNAMIC_CFG |
#endif
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_ONLY_ALLOWED_CHANNELS_DEFAULT),

   REG_VARIABLE( CFG_REG_CHANGE_DEF_COUNTRY_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, fRegChangeDefCountry,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_REG_CHANGE_DEF_COUNTRY_DEFAULT,
                 CFG_REG_CHANGE_DEF_COUNTRY_MIN,
                 CFG_REG_CHANGE_DEF_COUNTRY_MAX),

   REG_VARIABLE( CFG_SAP_SCAN_BAND_PREFERENCE, WLAN_PARAM_Integer,
                 hdd_config_t, acsScanBandPreference,
#ifndef WLAN_FEATURE_MBSSID
                 VAR_FLAGS_DYNAMIC_CFG |
#endif
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SAP_SCAN_BAND_PREFERENCE_DEFAULT,
                 CFG_SAP_SCAN_BAND_PREFERENCE_MIN,
                 CFG_SAP_SCAN_BAND_PREFERENCE_MAX ),

#ifdef QCA_LL_TX_FLOW_CT
   REG_VARIABLE( CFG_LL_TX_FLOW_LWM, WLAN_PARAM_Integer,
                 hdd_config_t, TxFlowLowWaterMark,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LL_TX_FLOW_LWM_DEFAULT,
                 CFG_LL_TX_FLOW_LWM_MIN,
                 CFG_LL_TX_FLOW_LWM_MAX ),

   REG_VARIABLE( CFG_LL_TX_FLOW_HWM_OFFSET, WLAN_PARAM_Integer,
                 hdd_config_t, TxFlowHighWaterMarkOffset,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LL_TX_FLOW_HWM_OFFSET_DEFAULT,
                 CFG_LL_TX_FLOW_HWM_OFFSET_MIN,
                 CFG_LL_TX_FLOW_HWM_OFFSET_MAX ),

   REG_VARIABLE( CFG_LL_TX_FLOW_MAX_Q_DEPTH, WLAN_PARAM_Integer,
                 hdd_config_t, TxFlowMaxQueueDepth,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LL_TX_FLOW_MAX_Q_DEPTH_DEFAULT,
                 CFG_LL_TX_FLOW_MAX_Q_DEPTH_MIN,
                 CFG_LL_TX_FLOW_MAX_Q_DEPTH_MAX ),

   REG_VARIABLE( CFG_LL_TX_LBW_FLOW_LWM, WLAN_PARAM_Integer,
                 hdd_config_t, TxLbwFlowLowWaterMark,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LL_TX_LBW_FLOW_LWM_DEFAULT,
                 CFG_LL_TX_LBW_FLOW_LWM_MIN,
                 CFG_LL_TX_LBW_FLOW_LWM_MAX ),

   REG_VARIABLE( CFG_LL_TX_LBW_FLOW_HWM_OFFSET, WLAN_PARAM_Integer,
                 hdd_config_t, TxLbwFlowHighWaterMarkOffset,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LL_TX_LBW_FLOW_HWM_OFFSET_DEFAULT,
                 CFG_LL_TX_LBW_FLOW_HWM_OFFSET_MIN,
                 CFG_LL_TX_LBW_FLOW_HWM_OFFSET_MAX ),

   REG_VARIABLE( CFG_LL_TX_LBW_FLOW_MAX_Q_DEPTH, WLAN_PARAM_Integer,
                 hdd_config_t, TxLbwFlowMaxQueueDepth,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LL_TX_LBW_FLOW_MAX_Q_DEPTH_DEFAULT,
                 CFG_LL_TX_LBW_FLOW_MAX_Q_DEPTH_MIN,
                 CFG_LL_TX_LBW_FLOW_MAX_Q_DEPTH_MAX ),

   REG_VARIABLE( CFG_LL_TX_HBW_FLOW_LWM, WLAN_PARAM_Integer,
                 hdd_config_t, TxHbwFlowLowWaterMark,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LL_TX_HBW_FLOW_LWM_DEFAULT,
                 CFG_LL_TX_HBW_FLOW_LWM_MIN,
                 CFG_LL_TX_HBW_FLOW_LWM_MAX ),

   REG_VARIABLE( CFG_LL_TX_HBW_FLOW_HWM_OFFSET, WLAN_PARAM_Integer,
                 hdd_config_t, TxHbwFlowHighWaterMarkOffset,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LL_TX_HBW_FLOW_HWM_OFFSET_DEFAULT,
                 CFG_LL_TX_HBW_FLOW_HWM_OFFSET_MIN,
                 CFG_LL_TX_HBW_FLOW_HWM_OFFSET_MAX ),

   REG_VARIABLE( CFG_LL_TX_HBW_FLOW_MAX_Q_DEPTH, WLAN_PARAM_Integer,
                 hdd_config_t, TxHbwFlowMaxQueueDepth,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_LL_TX_HBW_FLOW_MAX_Q_DEPTH_DEFAULT,
                 CFG_LL_TX_HBW_FLOW_MAX_Q_DEPTH_MIN,
                 CFG_LL_TX_HBW_FLOW_MAX_Q_DEPTH_MAX ),
#endif /* QCA_LL_TX_FLOW_CT */

   REG_VARIABLE( CFG_INITIAL_DWELL_TIME_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, nInitialDwellTime,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_INITIAL_DWELL_TIME_DEFAULT,
                 CFG_INITIAL_DWELL_TIME_MIN,
                 CFG_INITIAL_DWELL_TIME_MAX ),

   REG_VARIABLE( CFG_ACS_BAND_SWITCH_THRESHOLD, WLAN_PARAM_Integer,
                 hdd_config_t, acsBandSwitchThreshold,
#ifndef WLAN_FEATURE_MBSSID
                 VAR_FLAGS_DYNAMIC_CFG |
#endif
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_ACS_BAND_SWITCH_THRESHOLD_DEFAULT,
                 CFG_ACS_BAND_SWITCH_THRESHOLD_MIN,
                 CFG_ACS_BAND_SWITCH_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_SAP_MAX_OFFLOAD_PEERS, WLAN_PARAM_Integer,
                 hdd_config_t, apMaxOffloadPeers,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SAP_MAX_OFFLOAD_PEERS_DEFAULT,
                 CFG_SAP_MAX_OFFLOAD_PEERS_MIN,
                 CFG_SAP_MAX_OFFLOAD_PEERS_MAX ),

   REG_VARIABLE( CFG_SAP_MAX_OFFLOAD_REORDER_BUFFS, WLAN_PARAM_Integer,
                 hdd_config_t, apMaxOffloadReorderBuffs,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SAP_MAX_OFFLOAD_REORDER_BUFFS_DEFAULT,
                 CFG_SAP_MAX_OFFLOAD_REORDER_BUFFS_MIN,
                 CFG_SAP_MAX_OFFLOAD_REORDER_BUFFS_MAX ),

   REG_VARIABLE( CFG_ADVERTISE_CONCURRENT_OPERATION_NAME , WLAN_PARAM_Integer,
                 hdd_config_t, advertiseConcurrentOperation,
                 VAR_FLAGS_OPTIONAL,
                 CFG_ADVERTISE_CONCURRENT_OPERATION_DEFAULT,
                 CFG_ADVERTISE_CONCURRENT_OPERATION_MIN,
                 CFG_ADVERTISE_CONCURRENT_OPERATION_MAX ),

   REG_VARIABLE( CFG_ENABLE_HYSTERETIC_MODE, WLAN_PARAM_Integer,
                 hdd_config_t, enableHystereticMode,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_HYSTERETIC_MODE_DEFAULT,
                 CFG_ENABLE_HYSTERETIC_MODE_MIN,
                 CFG_ENABLE_HYSTERETIC_MODE_MAX ),

   REG_VARIABLE( CFG_DEFAULT_RATE_INDEX_24GH, WLAN_PARAM_Integer,
                 hdd_config_t, defaultRateIndex24Ghz,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_DEFAULT_RATE_INDEX_24GH_DEFAULT,
                 CFG_DEFAULT_RATE_INDEX_24GH_MIN,
                 CFG_DEFAULT_RATE_INDEX_24GH_MAX ),

   REG_VARIABLE_STRING( CFG_OVERRIDE_COUNTRY_CODE, WLAN_PARAM_String,
                        hdd_config_t, overrideCountryCode,
                        VAR_FLAGS_OPTIONAL,
                        (void *)CFG_OVERRIDE_COUNTRY_CODE_DEFAULT),

#ifdef MEMORY_DEBUG
   REG_VARIABLE( CFG_ENABLE_MEMORY_DEBUG_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, IsMemoryDebugSupportEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_MEMORY_DEBUG_DEFAULT,
                 CFG_ENABLE_MEMORY_DEBUG_MIN,
                 CFG_ENABLE_MEMORY_DEBUG_MAX ),
#endif

   REG_VARIABLE( CFG_DEBUG_P2P_REMAIN_ON_CHANNEL_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, debugP2pRemainOnChannel,
                 VAR_FLAGS_OPTIONAL,
                 CFG_DEBUG_P2P_REMAIN_ON_CHANNEL_DEFAULT,
                 CFG_DEBUG_P2P_REMAIN_ON_CHANNEL_MIN,
                 CFG_DEBUG_P2P_REMAIN_ON_CHANNEL_MAX ),

   REG_VARIABLE( CFG_ENABLE_PACKET_LOG, WLAN_PARAM_Integer,
                 hdd_config_t, enablePacketLog,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_PACKET_LOG_DEFAULT,
                 CFG_ENABLE_PACKET_LOG_MIN,
                 CFG_ENABLE_PACKET_LOG_MAX ),

#ifdef WLAN_FEATURE_ROAM_OFFLOAD
   REG_VARIABLE( CFG_ROAMING_OFFLOAD_NAME,  WLAN_PARAM_Integer,
                 hdd_config_t, isRoamOffloadEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_ROAMING_OFFLOAD_DEFAULT,
                 CFG_ROAMING_OFFLOAD_MIN,
                 CFG_ROAMING_OFFLOAD_MAX),
#endif
#ifdef MSM_PLATFORM
   REG_VARIABLE( CFG_BUS_BANDWIDTH_HIGH_THRESHOLD, WLAN_PARAM_Integer,
                 hdd_config_t, busBandwidthHighThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BUS_BANDWIDTH_HIGH_THRESHOLD_DEFAULT,
                 CFG_BUS_BANDWIDTH_HIGH_THRESHOLD_MIN,
                 CFG_BUS_BANDWIDTH_HIGH_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_BUS_BANDWIDTH_MEDIUM_THRESHOLD, WLAN_PARAM_Integer,
                 hdd_config_t, busBandwidthMediumThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BUS_BANDWIDTH_MEDIUM_THRESHOLD_DEFAULT,
                 CFG_BUS_BANDWIDTH_MEDIUM_THRESHOLD_MIN,
                 CFG_BUS_BANDWIDTH_MEDIUM_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_BUS_BANDWIDTH_LOW_THRESHOLD, WLAN_PARAM_Integer,
                 hdd_config_t, busBandwidthLowThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BUS_BANDWIDTH_LOW_THRESHOLD_DEFAULT,
                 CFG_BUS_BANDWIDTH_LOW_THRESHOLD_MIN,
                 CFG_BUS_BANDWIDTH_LOW_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_BUS_BANDWIDTH_COMPUTE_INTERVAL, WLAN_PARAM_Integer,
                 hdd_config_t, busBandwidthComputeInterval,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_BUS_BANDWIDTH_COMPUTE_INTERVAL_DEFAULT,
                 CFG_BUS_BANDWIDTH_COMPUTE_INTERVAL_MIN,
                 CFG_BUS_BANDWIDTH_COMPUTE_INTERVAL_MAX),

   REG_VARIABLE( CFG_TCP_DELACK_THRESHOLD_HIGH, WLAN_PARAM_Integer,
                hdd_config_t, tcpDelackThresholdHigh,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_TCP_DELACK_THRESHOLD_HIGH_DEFAULT,
                CFG_TCP_DELACK_THRESHOLD_HIGH_MIN,
                CFG_TCP_DELACK_THRESHOLD_HIGH_MAX ),

   REG_VARIABLE( CFG_TCP_DELACK_THRESHOLD_LOW, WLAN_PARAM_Integer,
                hdd_config_t, tcpDelackThresholdLow,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_TCP_DELACK_THRESHOLD_LOW_DEFAULT,
                CFG_TCP_DELACK_THRESHOLD_LOW_MIN,
                CFG_TCP_DELACK_THRESHOLD_LOW_MAX ),
#endif


   REG_VARIABLE( CFG_ENABLE_FW_LOG_TYPE , WLAN_PARAM_Integer,
                hdd_config_t, enableFwLogType,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_ENABLE_FW_LOG_TYPE_DEFAULT,
                CFG_ENABLE_FW_LOG_TYPE_MIN,
                CFG_ENABLE_FW_LOG_TYPE_MAX ),

   REG_VARIABLE( CFG_ENABLE_FW_DEBUG_LOG_LEVEL, WLAN_PARAM_Integer,
                hdd_config_t, enableFwLogLevel,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_ENABLE_FW_DEBUG_LOG_LEVEL_DEFAULT,
                CFG_ENABLE_FW_DEBUG_LOG_LEVEL_MIN,
                CFG_ENABLE_FW_DEBUG_LOG_LEVEL_MAX ),

   REG_VARIABLE_STRING( CFG_ENABLE_FW_MODULE_LOG_LEVEL, WLAN_PARAM_String,
                hdd_config_t, enableFwModuleLogLevel,
                VAR_FLAGS_OPTIONAL,
                (void *) CFG_ENABLE_FW_MODULE_LOG_DEFAULT),



#ifdef WLAN_FEATURE_11W
   REG_VARIABLE(CFG_PMF_SA_QUERY_MAX_RETRIES_NAME, WLAN_PARAM_Integer,
                hdd_config_t, pmfSaQueryMaxRetries,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_PMF_SA_QUERY_MAX_RETRIES_DEFAULT,
                CFG_PMF_SA_QUERY_MAX_RETRIES_MIN,
                CFG_PMF_SA_QUERY_MAX_RETRIES_MAX ),

   REG_VARIABLE(CFG_PMF_SA_QUERY_RETRY_INTERVAL_NAME, WLAN_PARAM_Integer,
                hdd_config_t, pmfSaQueryRetryInterval,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_PMF_SA_QUERY_RETRY_INTERVAL_DEFAULT,
                CFG_PMF_SA_QUERY_RETRY_INTERVAL_MIN,
                CFG_PMF_SA_QUERY_RETRY_INTERVAL_MAX ),
#endif
   REG_VARIABLE(CFG_MAX_CONCURRENT_CONNECTIONS_NAME, WLAN_PARAM_Integer,
                hdd_config_t, gMaxConcurrentActiveSessions,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_MAX_CONCURRENT_CONNECTIONS_DEFAULT,
                CFG_MAX_CONCURRENT_CONNECTIONS_MIN,
                CFG_MAX_CONCURRENT_CONNECTIONS_MAX ),

#ifdef QCA_HT_2040_COEX
   REG_VARIABLE(CFG_ENABLE_HT_2040_COEX, WLAN_PARAM_Integer,
                hdd_config_t, ht2040CoexEnabled,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_ENABLE_HT_2040_COEX_DEFAULT,
                CFG_ENABLE_HT_2040_COEX_MIN,
                CFG_ENABLE_HT_2040_COEX_MAX ),
#endif

#ifdef FEATURE_GREEN_AP
   REG_VARIABLE( CFG_ENABLE_GREEN_AP_FEATURE, WLAN_PARAM_Integer,
                 hdd_config_t, enableGreenAP,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_ENABLE_GREEN_AP_FEATURE_DEFAULT,
                 CFG_ENABLE_GREEN_AP_FEATURE_MIN,
                 CFG_ENABLE_GREEN_AP_FEATURE_MAX ),
#endif

   REG_VARIABLE(CFG_IGNORE_CAC_NAME, WLAN_PARAM_Integer,
                hdd_config_t, ignoreCAC,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_IGNORE_CAC_DEFAULT,
                CFG_IGNORE_CAC_MIN,
                CFG_IGNORE_CAC_MAX),

   REG_VARIABLE(CFG_ENABLE_SAP_DFS_CH_SIFS_BURST_NAME, WLAN_PARAM_Integer,
                hdd_config_t, IsSapDfsChSifsBurstEnabled,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_ENABLE_SAP_DFS_CH_SIFS_BURST_DEFAULT,
                CFG_ENABLE_SAP_DFS_CH_SIFS_BURST_MIN,
                CFG_ENABLE_SAP_DFS_CH_SIFS_BURST_MAX ),

   REG_VARIABLE(CFG_DFS_RADAR_PRI_MULTIPLIER_NAME, WLAN_PARAM_Integer,
                hdd_config_t, dfsRadarPriMultiplier,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_DFS_RADAR_PRI_MULTIPLIER_DEFAULT,
                CFG_DFS_RADAR_PRI_MULTIPLIER_MIN,
                CFG_DFS_RADAR_PRI_MULTIPLIER_MAX),

   REG_VARIABLE( CFG_REORDER_OFFLOAD_SUPPORT_NAME, WLAN_PARAM_Integer,
                        hdd_config_t, reorderOffloadSupport,
                        VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                        CFG_REORDER_OFFLOAD_SUPPORT_DEFAULT,
                        CFG_REORDER_OFFLOAD_SUPPORT_MIN,
                        CFG_REORDER_OFFLOAD_SUPPORT_MAX ),
#ifdef IPA_UC_OFFLOAD
   REG_VARIABLE( CFG_IPA_UC_OFFLOAD_ENABLED_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, IpaUcOffloadEnabled,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_IPA_UC_OFFLOAD_ENABLED_DEFAULT,
                 CFG_IPA_UC_OFFLOAD_ENABLED_MIN,
                 CFG_IPA_UC_OFFLOAD_ENABLED_MAX ),

   REG_VARIABLE( CFG_IPA_UC_TX_BUF_COUNT_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, IpaUcTxBufCount,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_IPA_UC_TX_BUF_COUNT_DEFAULT,
                 CFG_IPA_UC_TX_BUF_COUNT_MIN,
                 CFG_IPA_UC_TX_BUF_COUNT_MAX ),

   REG_VARIABLE( CFG_IPA_UC_TX_BUF_SIZE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, IpaUcTxBufSize,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_IPA_UC_TX_BUF_SIZE_DEFAULT,
                 CFG_IPA_UC_TX_BUF_SIZE_MIN,
                 CFG_IPA_UC_TX_BUF_SIZE_MAX ),

   REG_VARIABLE( CFG_IPA_UC_RX_IND_RING_COUNT_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, IpaUcRxIndRingCount,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_IPA_UC_RX_IND_RING_COUNT_DEFAULT,
                 CFG_IPA_UC_RX_IND_RING_COUNT_MIN,
                 CFG_IPA_UC_RX_IND_RING_COUNT_MAX ),

   REG_VARIABLE( CFG_IPA_UC_TX_PARTITION_BASE_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, IpaUcTxPartitionBase,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_IPA_UC_TX_PARTITION_BASE_DEFAULT,
                 CFG_IPA_UC_TX_PARTITION_BASE_MIN,
                 CFG_IPA_UC_TX_PARTITION_BASE_MAX ),
#endif /* IPA_UC_OFFLOAD */
#ifdef WLAN_LOGGING_SOCK_SVC_ENABLE
   REG_VARIABLE(CFG_WLAN_LOGGING_SUPPORT_NAME, WLAN_PARAM_Integer,
                hdd_config_t, wlanLoggingEnable,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_WLAN_LOGGING_SUPPORT_DEFAULT,
                CFG_WLAN_LOGGING_SUPPORT_DISABLE,
                CFG_WLAN_LOGGING_SUPPORT_ENABLE ),

   REG_VARIABLE(CFG_WLAN_LOGGING_FE_CONSOLE_SUPPORT_NAME, WLAN_PARAM_Integer,
                hdd_config_t, wlanLoggingFEToConsole,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_WLAN_LOGGING_FE_CONSOLE_SUPPORT_DEFAULT,
                CFG_WLAN_LOGGING_FE_CONSOLE_SUPPORT_DISABLE,
                CFG_WLAN_LOGGING_FE_CONSOLE_SUPPORT_ENABLE ),

   REG_VARIABLE(CFG_WLAN_LOGGING_NUM_BUF_NAME, WLAN_PARAM_Integer,
                hdd_config_t, wlanLoggingNumBuf,
                VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                CFG_WLAN_LOGGING_NUM_BUF_DEFAULT,
                CFG_WLAN_LOGGING_NUM_BUF_MIN,
                CFG_WLAN_LOGGING_NUM_BUF_MAX ),
#endif /* WLAN_LOGGING_SOCK_SVC_ENABLE */

   REG_VARIABLE( CFG_ENABLE_SIFS_BURST, WLAN_PARAM_Integer,
              hdd_config_t, enableSifsBurst,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_ENABLE_SIFS_BURST_DEFAULT,
              CFG_ENABLE_SIFS_BURST_MIN,
              CFG_ENABLE_SIFS_BURST_MAX ),

#ifdef WLAN_FEATURE_LPSS
   REG_VARIABLE(CFG_ENABLE_LPASS_SUPPORT, WLAN_PARAM_Integer,
               hdd_config_t, enablelpasssupport,
               VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
               CFG_ENABLE_LPASS_SUPPORT_DEFAULT,
               CFG_ENABLE_LPASS_SUPPORT_MIN,
               CFG_ENABLE_LPASS_SUPPORT_MAX),
#endif

   REG_VARIABLE( CFG_ENABLE_SELF_RECOVERY, WLAN_PARAM_Integer,
              hdd_config_t, enableSelfRecovery,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_ENABLE_SELF_RECOVERY_DEFAULT,
              CFG_ENABLE_SELF_RECOVERY_MIN,
              CFG_ENABLE_SELF_RECOVERY_MAX ),

#ifdef FEATURE_WLAN_FORCE_SAP_SCC
   REG_VARIABLE(CFG_SAP_SCC_CHAN_AVOIDANCE, WLAN_PARAM_Integer,
                 hdd_config_t, SapSccChanAvoidance,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_SAP_SCC_CHAN_AVOIDANCE_DEFAULT,
                 CFG_SAP_SCC_CHAN_AVOIDANCE_MIN,
                 CFG_SAP_SCC_CHAN_AVOIDANCE_MAX),
#endif /* FEATURE_WLAN_FORCE_SAP_SCC */

   REG_VARIABLE( CFG_ENABLE_SAP_SUSPEND, WLAN_PARAM_Integer,
               hdd_config_t, enableSapSuspend,
               VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
               CFG_ENABLE_SAP_SUSPEND_DEFAULT,
               CFG_ENABLE_SAP_SUSPEND_MIN,
               CFG_ENABLE_SAP_SUSPEND_MAX ),

#ifdef WLAN_FEATURE_EXTWOW_SUPPORT
   REG_VARIABLE( CFG_EXTWOW_GO_TO_SUSPEND, WLAN_PARAM_Integer,
              hdd_config_t, extWowGotoSuspend,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_GO_TO_SUSPEND_DEFAULT,
              CFG_EXTWOW_GO_TO_SUSPEND_MIN,
              CFG_EXTWOW_GO_TO_SUSPEND_MAX ),

   REG_VARIABLE( CFG_EXTWOW_APP1_WAKE_PIN_NUMBER, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp1WakeupPinNumber,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_APP1_WAKE_PIN_NUMBER_DEFAULT,
              CFG_EXTWOW_APP1_WAKE_PIN_NUMBER_MIN,
              CFG_EXTWOW_APP1_WAKE_PIN_NUMBER_MAX ),

   REG_VARIABLE( CFG_EXTWOW_APP2_WAKE_PIN_NUMBER, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp2WakeupPinNumber,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_APP2_WAKE_PIN_NUMBER_DEFAULT,
              CFG_EXTWOW_APP2_WAKE_PIN_NUMBER_MIN,
              CFG_EXTWOW_APP2_WAKE_PIN_NUMBER_MAX ),

   REG_VARIABLE( CFG_EXTWOW_KA_INIT_PING_INTERVAL, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp2KAInitPingInterval,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_KA_INIT_PING_INTERVAL_DEFAULT,
              CFG_EXTWOW_KA_INIT_PING_INTERVAL_MIN,
              CFG_EXTWOW_KA_INIT_PING_INTERVAL_MAX ),

   REG_VARIABLE( CFG_EXTWOW_KA_MIN_PING_INTERVAL, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp2KAMinPingInterval,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_KA_MIN_PING_INTERVAL_DEFAULT,
              CFG_EXTWOW_KA_MIN_PING_INTERVAL_MIN,
              CFG_EXTWOW_KA_MIN_PING_INTERVAL_MAX ),

   REG_VARIABLE( CFG_EXTWOW_KA_MAX_PING_INTERVAL, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp2KAMaxPingInterval,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_KA_MAX_PING_INTERVAL_DEFAULT,
              CFG_EXTWOW_KA_MAX_PING_INTERVAL_MIN,
              CFG_EXTWOW_KA_MAX_PING_INTERVAL_MAX ),

   REG_VARIABLE( CFG_EXTWOW_KA_INC_PING_INTERVAL, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp2KAIncPingInterval,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_KA_INC_PING_INTERVAL_DEFAULT,
              CFG_EXTWOW_KA_INC_PING_INTERVAL_MIN,
              CFG_EXTWOW_KA_INC_PING_INTERVAL_MAX ),

   REG_VARIABLE( CFG_EXTWOW_TCP_SRC_PORT, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp2TcpSrcPort,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_TCP_SRC_PORT_DEFAULT,
              CFG_EXTWOW_TCP_SRC_PORT_MIN,
              CFG_EXTWOW_TCP_SRC_PORT_MAX ),

   REG_VARIABLE( CFG_EXTWOW_TCP_DST_PORT, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp2TcpDstPort,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_TCP_DST_PORT_DEFAULT,
              CFG_EXTWOW_TCP_DST_PORT_MIN,
              CFG_EXTWOW_TCP_DST_PORT_MAX ),

   REG_VARIABLE( CFG_EXTWOW_TCP_TX_TIMEOUT, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp2TcpTxTimeout,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_TCP_TX_TIMEOUT_DEFAULT,
              CFG_EXTWOW_TCP_TX_TIMEOUT_MIN,
              CFG_EXTWOW_TCP_TX_TIMEOUT_MAX ),

   REG_VARIABLE( CFG_EXTWOW_TCP_RX_TIMEOUT, WLAN_PARAM_Integer,
              hdd_config_t, extWowApp2TcpRxTimeout,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_EXTWOW_TCP_RX_TIMEOUT_DEFAULT,
              CFG_EXTWOW_TCP_RX_TIMEOUT_MIN,
              CFG_EXTWOW_TCP_RX_TIMEOUT_MAX ),
#endif
   REG_VARIABLE( CFG_ENABLE_DEAUTH_TO_DISASSOC_MAP_NAME, WLAN_PARAM_Integer,
              hdd_config_t, gEnableDeauthToDisassocMap,
              VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
              CFG_ENABLE_DEAUTH_TO_DISASSOC_MAP_DEFAULT,
              CFG_ENABLE_DEAUTH_TO_DISASSOC_MAP_MIN,
              CFG_ENABLE_DEAUTH_TO_DISASSOC_MAP_MAX ),

#ifdef DHCP_SERVER_OFFLOAD
   REG_VARIABLE( CFG_DHCP_SERVER_OFFLOAD_SUPPORT_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, enableDHCPServerOffload,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_DHCP_SERVER_OFFLOAD_SUPPORT_DEFAULT,
                 CFG_DHCP_SERVER_OFFLOAD_SUPPORT_MIN,
                 CFG_DHCP_SERVER_OFFLOAD_SUPPORT_MAX ),

   REG_VARIABLE( CFG_DHCP_SERVER_OFFLOAD_NUM_CLIENT_NAME, WLAN_PARAM_Integer,
                 hdd_config_t, dhcpMaxNumClients,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT,
                 CFG_DHCP_SERVER_OFFLOAD_NUM_CLIENT_DEFAULT,
                 CFG_DHCP_SERVER_OFFLOAD_NUM_CLIENT_MIN,
                 CFG_DHCP_SERVER_OFFLOAD_NUM_CLIENT_MAX ),

   REG_VARIABLE_STRING( CFG_DHCP_SERVER_IP_NAME, WLAN_PARAM_String,
                 hdd_config_t, dhcpServerIP,
                 VAR_FLAGS_OPTIONAL,
                 (void *) CFG_DHCP_SERVER_IP_DEFAULT ),
#endif /* DHCP_SERVER_OFFLOAD */
};

#ifdef WLAN_FEATURE_MBSSID
REG_TABLE_ENTRY mbssid_sap_dyn_ini_reg_table[] =
{
   REG_VARIABLE( CFG_SAP_CHANNEL_SELECT_START_CHANNEL , WLAN_PARAM_Integer,
                 mbssid_sap_dyn_ini_config_t, apStartChannelNum,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT
                 | VAR_FLAGS_DYNAMIC_CFG,
                 CFG_SAP_CHANNEL_SELECT_START_CHANNEL_DEFAULT,
                 CFG_SAP_CHANNEL_SELECT_START_CHANNEL_MIN,
                 CFG_SAP_CHANNEL_SELECT_START_CHANNEL_MAX ),

   REG_VARIABLE( CFG_SAP_CHANNEL_SELECT_END_CHANNEL , WLAN_PARAM_Integer,
                 mbssid_sap_dyn_ini_config_t, apEndChannelNum,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT
                 | VAR_FLAGS_DYNAMIC_CFG,
                 CFG_SAP_CHANNEL_SELECT_END_CHANNEL_DEFAULT,
                 CFG_SAP_CHANNEL_SELECT_END_CHANNEL_MIN,
                 CFG_SAP_CHANNEL_SELECT_END_CHANNEL_MAX ),

   REG_VARIABLE( CFG_SAP_CHANNEL_SELECT_OPERATING_BAND , WLAN_PARAM_Integer,
                 mbssid_sap_dyn_ini_config_t, apOperatingBand,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT
                 | VAR_FLAGS_DYNAMIC_CFG,
                 CFG_SAP_CHANNEL_SELECT_OPERATING_BAND_DEFAULT,
                 CFG_SAP_CHANNEL_SELECT_OPERATING_BAND_MIN,
                 CFG_SAP_CHANNEL_SELECT_OPERATING_BAND_MAX ),

   REG_VARIABLE( CFG_SAP_AUTO_CHANNEL_SELECTION_NAME , WLAN_PARAM_Integer,
                 mbssid_sap_dyn_ini_config_t, apAutoChannelSelection,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT
                 | VAR_FLAGS_DYNAMIC_CFG,
                 CFG_SAP_AUTO_CHANNEL_SELECTION_DEFAULT,
                 CFG_SAP_AUTO_CHANNEL_SELECTION_MIN,
                 CFG_SAP_AUTO_CHANNEL_SELECTION_MAX ),


   REG_VARIABLE_STRING( CFG_ONLY_ALLOWED_CHANNELS, WLAN_PARAM_String,
                        mbssid_sap_dyn_ini_config_t, acsAllowedChnls,
                        VAR_FLAGS_OPTIONAL | VAR_FLAGS_DYNAMIC_CFG,
                        (void *)CFG_ONLY_ALLOWED_CHANNELS_DEFAULT),

   REG_VARIABLE( CFG_SAP_SCAN_BAND_PREFERENCE, WLAN_PARAM_Integer,
                 mbssid_sap_dyn_ini_config_t, acsScanBandPreference,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK
                 | VAR_FLAGS_DYNAMIC_CFG,
                 CFG_SAP_SCAN_BAND_PREFERENCE_DEFAULT,
                 CFG_SAP_SCAN_BAND_PREFERENCE_MIN,
                 CFG_SAP_SCAN_BAND_PREFERENCE_MAX ),

   REG_VARIABLE( CFG_ACS_BAND_SWITCH_THRESHOLD, WLAN_PARAM_Integer,
                 mbssid_sap_dyn_ini_config_t, acsBandSwitchThreshold,
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK
                 | VAR_FLAGS_DYNAMIC_CFG,
                 CFG_ACS_BAND_SWITCH_THRESHOLD_DEFAULT,
                 CFG_ACS_BAND_SWITCH_THRESHOLD_MIN,
                 CFG_ACS_BAND_SWITCH_THRESHOLD_MAX ),

   REG_VARIABLE( CFG_SAP_FORCE_11AC_FOR_11N, WLAN_PARAM_Integer,
                 mbssid_sap_dyn_ini_config_t, apForce11ACFor11n,
                 VAR_FLAGS_DYNAMIC_CFG |
                 VAR_FLAGS_OPTIONAL | VAR_FLAGS_RANGE_CHECK,
                 CFG_SAP_FORCE_11AC_FOR_11N_DEFAULT,
                 CFG_SAP_FORCE_11AC_FOR_11N_MIN,
                 CFG_SAP_FORCE_11AC_FOR_11N_MAX ),

};
#endif

/*
 * This function returns a pointer to the character after the occurrence
 * of a new line character. It also modifies the original string by replacing
 * the '\n' character with the null character.
 * Function returns NULL if no new line character was found before end of
 * string was reached
 */
static char* get_next_line(char* str)
{
  char c;

  if( str == NULL || *str == '\0') {
    return NULL;
  }

  c = *str;
  while(c != '\n'  && c != '\0' && c != 0xd)  {
    str = str + 1;
    c = *str;
  }

  if (c == '\0' ) {
    return NULL;
  }
  else
  {
    *str = '\0';
    return (str+1);
  }

  return NULL;
}

// look for space. Ascii values to look are -
// 0x09 == horizontal tab
// 0x0a == Newline ("\n")
// 0x0b == vertical tab
// 0x0c == Newpage or feed form.
// 0x0d == carriage return (CR or "\r")
// Null ('\0') should not considered as space.
#define i_isspace(ch)  (((ch) >= 0x09 && (ch) <= 0x0d) || (ch) == ' ')

/*
 * This function trims any leading and trailing white spaces
 */
static char *i_trim(char *str)

{
   char *ptr;

   if(*str == '\0') return str;

   /* Find the first non white-space*/
   for (ptr = str; i_isspace(*ptr); ptr++);
      if (*ptr == '\0')
         return str;

   /* This is the new start of the string*/
   str = ptr;

   /* Find the last non white-space */
   ptr += strlen(ptr) - 1;
   for (; ptr != str && i_isspace(*ptr); ptr--);
      /* Null terminate the following character */
      ptr[1] = '\0';

   return str;
}


//Structure to store each entry in qcom_cfg.ini file
typedef struct
{
   char *name;
   char *value;
}tCfgIniEntry;

static VOS_STATUS hdd_apply_cfg_ini( hdd_context_t * pHddCtx,
    tCfgIniEntry* iniTable, unsigned long entries);

#ifdef WLAN_CFG_DEBUG
void dump_cfg_ini (tCfgIniEntry* iniTable, unsigned long entries)
{
   unsigned long i;

   for (i = 0; i < entries; i++) {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "%s entry Name=[%s] value=[%s]",
           WLAN_INI_FILE, iniTable[i].name, iniTable[i].value);
     }
}
#endif

/*
 * This function reads the qcom_cfg.ini file and
 * parses each 'Name=Value' pair in the ini file
 */
VOS_STATUS hdd_parse_config_ini(hdd_context_t* pHddCtx)
{
   int status, i=0;
   /** Pointer for firmware image data */
   const struct firmware *fw = NULL;
   char *buffer, *line, *pTemp = NULL;
   size_t size;
   char *name, *value;
   /* cfgIniTable is static to avoid excess stack usage */
   static tCfgIniEntry cfgIniTable[MAX_CFG_INI_ITEMS];
   VOS_STATUS vos_status = VOS_STATUS_SUCCESS;

   memset(cfgIniTable, 0, sizeof(cfgIniTable));

   status = request_firmware(&fw, WLAN_INI_FILE, pHddCtx->parent_dev);

   if(status)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL, "%s: request_firmware failed %d",__func__, status);
      vos_status = VOS_STATUS_E_FAILURE;
      goto config_exit;
   }
   if(!fw || !fw->data || !fw->size)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL, "%s: %s download failed",
             __func__, WLAN_INI_FILE);
      vos_status = VOS_STATUS_E_FAILURE;
      goto config_exit;
   }

   hddLog(LOG1, "%s: qcom_cfg.ini Size %zu", __func__, fw->size);

   buffer = (char*)vos_mem_malloc(fw->size);

   if(NULL == buffer) {
      hddLog(VOS_TRACE_LEVEL_FATAL, "%s: kmalloc failure",__func__);
      release_firmware(fw);
      return VOS_STATUS_E_FAILURE;
   }
   pTemp = buffer;

   vos_mem_copy((void*)buffer,(void *)fw->data, fw->size);
   size = fw->size;

   while (buffer != NULL)
   {
      line = get_next_line(buffer);
      buffer = i_trim(buffer);

      hddLog(LOG1, "%s: item", buffer);

      if(strlen((char*)buffer) == 0 || *buffer == '#')  {
         buffer = line;
         continue;
      }
      else if(strncmp(buffer, "END", 3) == 0 ) {
         break;
      }
      else
      {
         name = buffer;
         while(*buffer != '=' && *buffer != '\0')
            buffer++;
         if(*buffer != '\0') {
            *buffer++ = '\0';
            i_trim(name);
            if(strlen (name) != 0) {
               buffer = i_trim(buffer);
               if(strlen(buffer)>0) {
                  value = buffer;
                  while(!i_isspace(*buffer) && *buffer != '\0')
                     buffer++;
                  *buffer = '\0';
                  cfgIniTable[i].name= name;
                  cfgIniTable[i++].value= value;
                  if(i >= MAX_CFG_INI_ITEMS) {
                     hddLog(LOGE,"%s: Number of items in %s > %d",
                        __func__, WLAN_INI_FILE, MAX_CFG_INI_ITEMS);
                     break;
                  }
               }
            }
         }
      }
      buffer = line;
   }

   //Loop through the registry table and apply all these configs
   vos_status = hdd_apply_cfg_ini(pHddCtx, cfgIniTable, i);

config_exit:
   release_firmware(fw);
   vos_mem_free(pTemp);
   return vos_status;
}


void print_hdd_cfg(hdd_context_t *pHddCtx)
{
  int i;

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "*********Config values in HDD Adapter*******");
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [RTSThreshold] Value = %u",pHddCtx->cfg_ini->RTSThreshold) ;
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [OperatingChannel] Value = [%u]",pHddCtx->cfg_ini->OperatingChannel);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [PowerUsageControl] Value = [%s]",pHddCtx->cfg_ini->PowerUsageControl);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fIsImpsEnabled] Value = [%u]",pHddCtx->cfg_ini->fIsImpsEnabled);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [AutoBmpsTimerEnabled] Value = [%u]",pHddCtx->cfg_ini->fIsAutoBmpsTimerEnabled);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nAutoBmpsTimerValue] Value = [%u]",pHddCtx->cfg_ini->nAutoBmpsTimerValue);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nVccRssiTrigger] Value = [%u]",pHddCtx->cfg_ini->nVccRssiTrigger);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [gIbssBssid] Value =["MAC_ADDRESS_STR"]",
            MAC_ADDR_ARRAY(pHddCtx->cfg_ini->IbssBssid.bytes));

  for (i=0; i < VOS_MAX_CONCURRENCY_PERSONA; i++)
  {
      VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [Intf%dMacAddress] Value =["MAC_ADDRESS_STR"]",
            i, MAC_ADDR_ARRAY(pHddCtx->cfg_ini->intfMacAddr[i].bytes));
  }

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gApEnableUapsd] value = [%u]",pHddCtx->cfg_ini->apUapsdEnabled);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gAPCntryCode] Value =[%c%c%c]",
      pHddCtx->cfg_ini->apCntryCode[0],pHddCtx->cfg_ini->apCntryCode[1],
      pHddCtx->cfg_ini->apCntryCode[2]);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableApProt] value = [%u]", pHddCtx->cfg_ini->apProtEnabled);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gAPAutoShutOff] Value = [%u]", pHddCtx->cfg_ini->nAPAutoShutOff);
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gWlanMccToSccSwitchMode] Value = [%u]", pHddCtx->cfg_ini->WlanMccToSccSwitchMode);
#endif
#ifdef FEATURE_WLAN_AUTO_SHUTDOWN
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gWlanAutoShutdown] Value = [%u]", pHddCtx->cfg_ini->WlanAutoShutdown);
#endif
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableListenMode] Value = [%u]", pHddCtx->cfg_ini->nEnableListenMode);
  VOS_TRACE (VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gApProtection] value = [%u]",pHddCtx->cfg_ini->apProtection);
  VOS_TRACE (VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableApOBSSProt] value = [%u]",pHddCtx->cfg_ini->apOBSSProtEnabled);
  VOS_TRACE (VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gApAutoChannelSelection] value = [%u]",pHddCtx->cfg_ini->apAutoChannelSelection);
  VOS_TRACE (VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gACSAllowedChannels] value = [%s]", pHddCtx->cfg_ini->acsAllowedChnls);
  VOS_TRACE (VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gACSBandSwitchThreshold] value = [%u]", pHddCtx->cfg_ini->acsBandSwitchThreshold);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [ChannelBondingMode] Value = [%u]",pHddCtx->cfg_ini->nChannelBondingMode24GHz);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [ChannelBondingMode] Value = [%u]",pHddCtx->cfg_ini->nChannelBondingMode5GHz);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [dot11Mode] Value = [%u]",pHddCtx->cfg_ini->dot11Mode);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [WmmMode] Value = [%u] ",pHddCtx->cfg_ini->WmmMode);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [UapsdMask] Value = [0x%x] ",pHddCtx->cfg_ini->UapsdMask);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [PktClassificationBasis] Value = [%u] ",pHddCtx->cfg_ini->PktClassificationBasis);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [ImplicitQosIsEnabled] Value = [%u]",(int)pHddCtx->cfg_ini->bImplicitQosEnabled);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraUapsdVoSrvIntv] Value = [%u] ",pHddCtx->cfg_ini->InfraUapsdVoSrvIntv);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraUapsdVoSuspIntv] Value = [%u] ",pHddCtx->cfg_ini->InfraUapsdVoSuspIntv);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraUapsdViSrvIntv] Value = [%u] ",pHddCtx->cfg_ini->InfraUapsdViSrvIntv);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraUapsdViSuspIntv] Value = [%u] ",pHddCtx->cfg_ini->InfraUapsdViSuspIntv);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraUapsdBeSrvIntv] Value = [%u] ",pHddCtx->cfg_ini->InfraUapsdBeSrvIntv);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraUapsdBeSuspIntv] Value = [%u] ",pHddCtx->cfg_ini->InfraUapsdBeSuspIntv);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraUapsdBkSrvIntv] Value = [%u] ",pHddCtx->cfg_ini->InfraUapsdBkSrvIntv);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraUapsdBkSuspIntv] Value = [%u] ",pHddCtx->cfg_ini->InfraUapsdBkSuspIntv);
#ifdef FEATURE_WLAN_ESE
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraInactivityInterval] Value = [%u] ",pHddCtx->cfg_ini->InfraInactivityInterval);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [EseEnabled] Value = [%u] ",pHddCtx->cfg_ini->isEseIniFeatureEnabled);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [FastTransitionEnabled] Value = [%u] ",pHddCtx->cfg_ini->isFastTransitionEnabled);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gTxPowerCap] Value = [%u] dBm ",pHddCtx->cfg_ini->nTxPowerCap);
#endif
#ifdef FEATURE_WLAN_LFR
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [FastRoamEnabled] Value = [%u] ",pHddCtx->cfg_ini->isFastRoamIniFeatureEnabled);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [MAWCEnabled] Value = [%u] ",pHddCtx->cfg_ini->MAWCEnabled);
#endif
#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [RoamRssiDiff] Value = [%u] ",pHddCtx->cfg_ini->RoamRssiDiff);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [ImmediateRoamRssiDiff] Value = [%u] ",pHddCtx->cfg_ini->nImmediateRoamRssiDiff);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [isWESModeEnabled] Value = [%u] ",pHddCtx->cfg_ini->isWESModeEnabled);
#endif
#ifdef FEATURE_WLAN_OKC
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [OkcEnabled] Value = [%u] ",pHddCtx->cfg_ini->isOkcIniFeatureEnabled);
#endif
#ifdef FEATURE_WLAN_SCAN_PNO
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [configPNOScanSupport] Value = [%u] ",pHddCtx->cfg_ini->configPNOScanSupport);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [configPNOScanTimerRepeatValue] Value = [%u] ",pHddCtx->cfg_ini->configPNOScanTimerRepeatValue);
#endif
#ifdef FEATURE_WLAN_TDLS
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fEnableTDLSSupport] Value = [%u] ",pHddCtx->cfg_ini->fEnableTDLSSupport);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fEnableTDLSImplicitTrigger] Value = [%u] ",pHddCtx->cfg_ini->fEnableTDLSImplicitTrigger);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fTDLSExternalControl] Value = [%u] ",pHddCtx->cfg_ini->fTDLSExternalControl);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fTDLSUapsdMask] Value = [%u] ",pHddCtx->cfg_ini->fTDLSUapsdMask);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fEnableTDLSBufferSta] Value = [%u] ",pHddCtx->cfg_ini->fEnableTDLSBufferSta);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fEnableTDLSWmmMode] Value = [%u] ",pHddCtx->cfg_ini->fEnableTDLSWmmMode);
#endif
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraDirAcVo] Value = [%u] ",pHddCtx->cfg_ini->InfraDirAcVo);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraNomMsduSizeAcVo] Value = [0x%x] ",pHddCtx->cfg_ini->InfraNomMsduSizeAcVo);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraMeanDataRateAcVo] Value = [0x%x] ",pHddCtx->cfg_ini->InfraMeanDataRateAcVo);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraMinPhyRateAcVo] Value = [0x%x] ",pHddCtx->cfg_ini->InfraMinPhyRateAcVo);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraSbaAcVo] Value = [0x%x] ",pHddCtx->cfg_ini->InfraSbaAcVo);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraDirAcVi] Value = [%u] ",pHddCtx->cfg_ini->InfraDirAcVi);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraNomMsduSizeAcVi] Value = [0x%x] ",pHddCtx->cfg_ini->InfraNomMsduSizeAcVi);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraMeanDataRateAcVi] Value = [0x%x] ",pHddCtx->cfg_ini->InfraMeanDataRateAcVi);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraMinPhyRateAcVi] Value = [0x%x] ",pHddCtx->cfg_ini->InfraMinPhyRateAcVi);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraSbaAcVi] Value = [0x%x] ",pHddCtx->cfg_ini->InfraSbaAcVi);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraDirAcBe] Value = [%u] ",pHddCtx->cfg_ini->InfraDirAcBe);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraNomMsduSizeAcBe] Value = [0x%x] ",pHddCtx->cfg_ini->InfraNomMsduSizeAcBe);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraMeanDataRateAcBe] Value = [0x%x] ",pHddCtx->cfg_ini->InfraMeanDataRateAcBe);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraMinPhyRateAcBe] Value = [0x%x] ",pHddCtx->cfg_ini->InfraMinPhyRateAcBe);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraSbaAcBe] Value = [0x%x] ",pHddCtx->cfg_ini->InfraSbaAcBe);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraDirAcBk] Value = [%u] ",pHddCtx->cfg_ini->InfraDirAcBk);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraNomMsduSizeAcBk] Value = [0x%x] ",pHddCtx->cfg_ini->InfraNomMsduSizeAcBk);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraMeanDataRateAcBk] Value = [0x%x] ",pHddCtx->cfg_ini->InfraMeanDataRateAcBk);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraMinPhyRateAcBk] Value = [0x%x] ",pHddCtx->cfg_ini->InfraMinPhyRateAcBk);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [InfraSbaAcBk] Value = [0x%x] ",pHddCtx->cfg_ini->InfraSbaAcBk);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [DelayedTriggerFrmInt] Value = [%u] ",pHddCtx->cfg_ini->DelayedTriggerFrmInt);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [mcastBcastFilterSetting] Value = [%u] ",pHddCtx->cfg_ini->mcastBcastFilterSetting);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fhostArpOffload] Value = [%u] ",pHddCtx->cfg_ini->fhostArpOffload);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [ssdp] Value = [%u] ", pHddCtx->cfg_ini->ssdp);
#ifdef FEATURE_WLAN_RA_FILTERING
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [RArateLimitInterval] Value = [%u] ", pHddCtx->cfg_ini->RArateLimitInterval);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [IsRArateLimitEnabled] Value = [%u] ", pHddCtx->cfg_ini->IsRArateLimitEnabled);
#endif
#ifdef WLAN_FEATURE_VOWIFI_11R
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fFTResourceReqSupported] Value = [%u] ",pHddCtx->cfg_ini->fFTResourceReqSupported);
#endif

#ifdef WLAN_FEATURE_NEIGHBOR_ROAMING
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nNeighborReassocRssiThreshold] Value = [%u] ",pHddCtx->cfg_ini->nNeighborReassocRssiThreshold);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nNeighborLookupRssiThreshold] Value = [%u] ",pHddCtx->cfg_ini->nNeighborLookupRssiThreshold);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [nOpportunisticThresholdDiff] Value = [%u] ",
            pHddCtx->cfg_ini->nOpportunisticThresholdDiff);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [nRoamRescanRssiDiff] Value = [%u] ",
            pHddCtx->cfg_ini->nRoamRescanRssiDiff);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nNeighborScanMinChanTime] Value = [%u] ",pHddCtx->cfg_ini->nNeighborScanMinChanTime);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nNeighborScanMaxChanTime] Value = [%u] ",pHddCtx->cfg_ini->nNeighborScanMaxChanTime);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nMaxNeighborRetries] Value = [%u] ",pHddCtx->cfg_ini->nMaxNeighborReqTries);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nNeighborScanPeriod] Value = [%u] ",pHddCtx->cfg_ini->nNeighborScanPeriod);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nNeighborScanResultsRefreshPeriod] Value = [%u] ",pHddCtx->cfg_ini->nNeighborResultsRefreshPeriod);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nEmptyScanRefreshPeriod] Value = [%u] ",pHddCtx->cfg_ini->nEmptyScanRefreshPeriod);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [nRoamBmissFirstBcnt] Value = [%u] ",
            pHddCtx->cfg_ini->nRoamBmissFirstBcnt);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [nRoamBmissFinalBcnt] Value = [%u] ",
            pHddCtx->cfg_ini->nRoamBmissFinalBcnt);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [nRoamBeaconRssiWeight] Value = [%u] ",
            pHddCtx->cfg_ini->nRoamBeaconRssiWeight);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [allowDFSChannelRoam] Value = [%u] ",
            pHddCtx->cfg_ini->allowDFSChannelRoam);
#endif
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [burstSizeDefinition] Value = [0x%x] ",pHddCtx->cfg_ini->burstSizeDefinition);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [tsInfoAckPolicy] Value = [0x%x] ",pHddCtx->cfg_ini->tsInfoAckPolicy);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [rfSettlingTimeUs] Value = [%u] ",pHddCtx->cfg_ini->rfSettlingTimeUs);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [bSingleTidRc] Value = [%u] ",pHddCtx->cfg_ini->bSingleTidRc);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gDynamicPSPollvalue] Value = [%u] ",pHddCtx->cfg_ini->dynamicPsPollValue);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gAddTSWhenACMIsOff] Value = [%u] ",pHddCtx->cfg_ini->AddTSWhenACMIsOff);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gValidateScanList] Value = [%u] ",pHddCtx->cfg_ini->fValidateScanList);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gStaKeepAlivePeriod] Value = [%u] ",pHddCtx->cfg_ini->infraStaKeepAlivePeriod);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gApDataAvailPollInterVal] Value = [%u] ",pHddCtx->cfg_ini->apDataAvailPollPeriodInMs);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [BandCapability] Value = [%u] ",pHddCtx->cfg_ini->nBandCapability);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fEnableBeaconEarlyTermination] Value = [%u] ",pHddCtx->cfg_ini->fEnableBeaconEarlyTermination);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [teleBcnWakeupEnable] Value = [%u] ",pHddCtx->cfg_ini->teleBcnWakeupEn);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [transListenInterval] Value = [%u] ",pHddCtx->cfg_ini->nTeleBcnTransListenInterval);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [transLiNumIdleBeacons] Value = [%u] ",pHddCtx->cfg_ini->nTeleBcnTransLiNumIdleBeacons);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [maxListenInterval] Value = [%u] ",pHddCtx->cfg_ini->nTeleBcnMaxListenInterval);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [maxLiNumIdleBeacons] Value = [%u] ",pHddCtx->cfg_ini->nTeleBcnMaxLiNumIdleBeacons);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [bcnEarlyTermWakeInterval] Value = [%u] ",pHddCtx->cfg_ini->bcnEarlyTermWakeInterval);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gApDataAvailPollInterVal] Value = [%u] ",pHddCtx->cfg_ini->apDataAvailPollPeriodInMs);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableBypass11d] Value = [%u] ",pHddCtx->cfg_ini->enableBypass11d);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableDFSChnlScan] Value = [%u] ",pHddCtx->cfg_ini->enableDFSChnlScan);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gReportMaxLinkSpeed] Value = [%u] ",pHddCtx->cfg_ini->reportMaxLinkSpeed);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [thermalMitigationEnable] Value = [%u] ",pHddCtx->cfg_ini->thermalMitigationEnable);
#ifdef WLAN_FEATURE_11AC
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gVhtChannelWidth] value = [%u]",pHddCtx->cfg_ini->vhtChannelWidth);
#endif
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [enableFirstScan2GOnly] Value = [%u] ",pHddCtx->cfg_ini->enableFirstScan2GOnly);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [skipDfsChnlInP2pSearch] Value = [%u] ",pHddCtx->cfg_ini->skipDfsChnlInP2pSearch);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [ignoreDynamicDtimInP2pMode] Value = [%u] ",pHddCtx->cfg_ini->ignoreDynamicDtimInP2pMode);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [enableRxSTBC] Value = [%u] ",pHddCtx->cfg_ini->enableRxSTBC);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableLpwrImgTransition] Value = [%u] ",pHddCtx->cfg_ini->enableLpwrImgTransition);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableSSR] Value = [%u] ",pHddCtx->cfg_ini->enableSSR);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableVhtFor24GHzBand] Value = [%u] ",pHddCtx->cfg_ini->enableVhtFor24GHzBand);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gFlexConnectPowerFactor] Value = [%u] ", pHddCtx->cfg_ini->flexConnectPowerFactor);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableIbssHeartBeatOffload] Value = [%u] ", pHddCtx->cfg_ini->enableIbssHeartBeatOffload);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gAntennaDiversity] Value = [%u] ", pHddCtx->cfg_ini->antennaDiversity);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gGoLinkMonitorPeriod] Value = [%u]",pHddCtx->cfg_ini->goLinkMonitorPeriod);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gApLinkMonitorPeriod] Value = [%u]",pHddCtx->cfg_ini->apLinkMonitorPeriod);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gGoKeepAlivePeriod] Value = [%u]",pHddCtx->cfg_ini->goKeepAlivePeriod);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gApKeepAlivePeriod]Value = [%u]",pHddCtx->cfg_ini->apKeepAlivePeriod);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gAmsduSupportInAMPDU] Value = [%u] ",pHddCtx->cfg_ini->isAmsduSupportInAMPDU);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [nSelect5GHzMargin] Value = [%u] ",pHddCtx->cfg_ini->nSelect5GHzMargin);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gCoalesingInIBSS] Value = [%u] ",pHddCtx->cfg_ini->isCoalesingInIBSSAllowed);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gIbssATIMWinSize] Value = [%u] ",pHddCtx->cfg_ini->ibssATIMWinSize);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gIbssIsPowerSaveAllowed] Value = [%u] ",pHddCtx->cfg_ini->isIbssPowerSaveAllowed);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gIbssIsPowerCollapseAllowed] Value = [%u] ",pHddCtx->cfg_ini->isIbssPowerCollapseAllowed);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gIbssAwakeOnTxRx] Value = [%u] ",pHddCtx->cfg_ini->isIbssAwakeOnTxRx);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gIbssInactivityTime] Value = [%u] ",pHddCtx->cfg_ini->ibssInactivityCount);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gIbssTxSpEndInactivityTime] Value = [%u] ",pHddCtx->cfg_ini->ibssTxSpEndInactivityTime);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gIbssPsWarmupTime] Value = [%u] ",pHddCtx->cfg_ini->ibssPsWarmupTime);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gIbssPs1RxChainInAtim] Value = [%u] ",
          pHddCtx->cfg_ini->ibssPs1RxChainInAtimEnable);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [fDfsPhyerrFilterOffload] Value = [%u] ",pHddCtx->cfg_ini->fDfsPhyerrFilterOffload);

#ifdef IPA_OFFLOAD
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gIPAConfig] Value = [0x%x] ",pHddCtx->cfg_ini->IpaConfig);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gIPADescSize] Value = [%u] ",pHddCtx->cfg_ini->IpaDescSize);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [IpaHighBandwidthMbpsg] Value = [%u] ",pHddCtx->cfg_ini->IpaHighBandwidthMbps);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [IpaMediumBandwidthMbps] Value = [%u] ",pHddCtx->cfg_ini->IpaMediumBandwidthMbps);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [IpaLowBandwidthMbps] Value = [%u] ",pHddCtx->cfg_ini->IpaLowBandwidthMbps);
#endif
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gEnableOverLapCh] Value = [%u] ",pHddCtx->cfg_ini->gEnableOverLapCh);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gAcsScanBandPreference] Value = [%u] ",pHddCtx->cfg_ini->acsScanBandPreference);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gMaxOffloadPeers] Value = [%u] ",pHddCtx->cfg_ini->apMaxOffloadPeers);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gMaxOffloadReorderBuffs] value = [%u] ",pHddCtx->cfg_ini->apMaxOffloadReorderBuffs);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [overrideCountryCode] Value = [%s] ",pHddCtx->cfg_ini->overrideCountryCode);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gAllowDFSChannelRoam] Value = [%u] ",pHddCtx->cfg_ini->allowDFSChannelRoam);
  hddLog(VOS_TRACE_LEVEL_INFO_HIGH, "Name = [gMaxConcurrentActiveSessions] Value = [%u] ", pHddCtx->cfg_ini->gMaxConcurrentActiveSessions);

#ifdef MSM_PLATFORM
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gBusBandwidthHighThreshold] Value = [%u] ",
          pHddCtx->cfg_ini->busBandwidthHighThreshold);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gBusBandwidthMediumThreshold] Value = [%u] ",
          pHddCtx->cfg_ini->busBandwidthMediumThreshold);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gBusBandwidthLowThreshold] Value = [%u] ",
          pHddCtx->cfg_ini->busBandwidthLowThreshold);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gbusBandwidthComputeInterval] Value = [%u] ",
          pHddCtx->cfg_ini->busBandwidthComputeInterval);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gTcpDelAckThresholdHigh] Value = [%u] ",
          pHddCtx->cfg_ini->tcpDelackThresholdHigh);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gTcpDelAckThresholdLow] Value = [%u] ",
          pHddCtx->cfg_ini->tcpDelackThresholdLow);
#endif

#ifdef QCA_HT_2040_COEX
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gHT2040CoexEnabled] Value = [%u]",
          pHddCtx->cfg_ini->ht2040CoexEnabled);
#endif
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gIgnoreCAC] Value = [%u] ",
          pHddCtx->cfg_ini->ignoreCAC);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gSapPreferredChanLocation] Value = [%u] ",
          pHddCtx->cfg_ini->gSapPreferredChanLocation);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gDisableDfsJapanW53] Value = [%u] ",
          pHddCtx->cfg_ini->gDisableDfsJapanW53);
#ifdef FEATURE_GREEN_AP
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
          "Name = [gEnableGreenAp] Value = [%u] ",
          pHddCtx->cfg_ini->enableGreenAP);
#endif
#ifdef WLAN_FEATURE_ROAM_OFFLOAD
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [isRoamOffloadEnabled] Value = [%u]",
                   pHddCtx->cfg_ini->isRoamOffloadEnabled);
#endif
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gEnableSifsBurst] Value = [%u]",
                   pHddCtx->cfg_ini->enableSifsBurst);

#ifdef WLAN_FEATURE_LPSS
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [gEnableLpassSupport] Value = [%u] ",
            pHddCtx->cfg_ini->enablelpasssupport);
#endif

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gEnableSelfRecovery] Value = [%u]",
                   pHddCtx->cfg_ini->enableSelfRecovery);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "Name = [gEnableSapSuspend] Value = [%u]",
            pHddCtx->cfg_ini->enableSapSuspend);

#ifdef WLAN_FEATURE_EXTWOW_SUPPORT
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWoWgotoSuspend] Value = [%u]",
                   pHddCtx->cfg_ini->extWowGotoSuspend);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWowApp1WakeupPinNumber] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp1WakeupPinNumber);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWowApp2WakeupPinNumber] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp2WakeupPinNumber);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWoWApp2KAInitPingInterval] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp2KAInitPingInterval);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWoWApp2KAMinPingInterval] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp2KAMinPingInterval);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWoWApp2KAMaxPingInterval] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp2KAMaxPingInterval);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWoWApp2KAIncPingInterval] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp2KAIncPingInterval);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWoWApp2TcpSrcPort] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp2TcpSrcPort);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWoWApp2TcpDstPort] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp2TcpDstPort);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWoWApp2TcpTxTimeout] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp2TcpTxTimeout);

  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gExtWoWApp2TcpRxTimeout] Value = [%u]",
                   pHddCtx->cfg_ini->extWowApp2TcpRxTimeout);
#endif

#ifdef DHCP_SERVER_OFFLOAD
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gDHCPServerOffloadEnable] Value = [%u]",
                   pHddCtx->cfg_ini->enableDHCPServerOffload);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gDHCPMaxNumClients] Value = [%u]",
                   pHddCtx->cfg_ini->dhcpMaxNumClients);
  VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
           "Name = [gDHCPServerIP] Value = [%s]",
                   pHddCtx->cfg_ini->dhcpServerIP);
#endif
}

#define CFG_VALUE_MAX_LEN 256
#define CFG_ENTRY_MAX_LEN (32+CFG_VALUE_MAX_LEN)
static VOS_STATUS hdd_cfg_get_config(REG_TABLE_ENTRY *reg_table,
                              unsigned long cRegTableEntries,
                              v_U8_t *ini_struct,
                              hdd_context_t *pHddCtx, char *pBuf, int buflen)
{
   unsigned int idx;
   REG_TABLE_ENTRY *pRegEntry = reg_table;
   v_U32_t value;
   char valueStr[CFG_VALUE_MAX_LEN];
   char configStr[CFG_ENTRY_MAX_LEN];
   char *fmt;
   void *pField;
   v_MACADDR_t *pMacAddr;
   char *pCur = pBuf;
   int curlen;

   // start with an empty string
   *pCur = '\0';

   for ( idx = 0; idx < cRegTableEntries; idx++, pRegEntry++ )
   {
      pField = ini_struct + pRegEntry->VarOffset;

      if ( ( WLAN_PARAM_Integer       == pRegEntry->RegType ) ||
           ( WLAN_PARAM_SignedInteger == pRegEntry->RegType ) ||
           ( WLAN_PARAM_HexInteger    == pRegEntry->RegType ) )
      {
         value = 0;
         memcpy( &value, pField, pRegEntry->VarSize );
         if ( WLAN_PARAM_HexInteger == pRegEntry->RegType )
         {
            fmt = "%x";
         }
         else if ( WLAN_PARAM_SignedInteger == pRegEntry->RegType )
         {
            fmt = "%d";
         }
         else
         {
            fmt = "%u";
         }
         snprintf(valueStr, CFG_VALUE_MAX_LEN, fmt, value);
      }
      else if ( WLAN_PARAM_String == pRegEntry->RegType )
      {
         snprintf(valueStr, CFG_VALUE_MAX_LEN, "%s", (char *)pField);
      }
      else if ( WLAN_PARAM_MacAddr == pRegEntry->RegType )
      {
         pMacAddr = (v_MACADDR_t *) pField;
         snprintf(valueStr, CFG_VALUE_MAX_LEN,
                  "%02x:%02x:%02x:%02x:%02x:%02x",
                  pMacAddr->bytes[0],
                  pMacAddr->bytes[1],
                  pMacAddr->bytes[2],
                  pMacAddr->bytes[3],
                  pMacAddr->bytes[4],
                  pMacAddr->bytes[5]);
      }
      else
      {
         snprintf(valueStr, CFG_VALUE_MAX_LEN, "(unhandled)");
      }
      curlen = scnprintf(configStr, CFG_ENTRY_MAX_LEN,
                        "%s=[%s]%s\n",
                        pRegEntry->RegName,
                        valueStr,
                        test_bit(idx, (void *)&pHddCtx->cfg_ini->bExplicitCfg) ?
                        "*" : "");

      // ideally we want to return the config to the application
      // however the config is too big so we just printk() for now
#ifdef RETURN_IN_BUFFER
      if (curlen <= buflen)
      {
         // copy string + '\0'
         memcpy(pCur, configStr, curlen+1);

         // account for addition;
         pCur += curlen;
         buflen -= curlen;
      }
      else
      {
         // buffer space exhausted, return what we have
         return VOS_STATUS_E_RESOURCES;
      }
#else
      printk(KERN_INFO "%s", configStr);
#endif // RETURN_IN_BUFFER

}

#ifndef RETURN_IN_BUFFER
   // notify application that output is in system log
   snprintf(pCur, buflen, "WLAN configuration written to system log");
#endif // RETURN_IN_BUFFER

   return VOS_STATUS_SUCCESS;
}

VOS_STATUS hdd_cfg_get_global_config(hdd_context_t *pHddCtx, char *pBuf,
                                                                    int buflen)
{
    return hdd_cfg_get_config(g_registry_table, ARRAY_SIZE(g_registry_table),
                              (v_U8_t *) pHddCtx->cfg_ini,
                              pHddCtx, pBuf, buflen);
}

#ifdef WLAN_FEATURE_MBSSID
VOS_STATUS hdd_cfg_get_sap_dyn_config(hdd_adapter_t *pAdapter, char *pBuf,
                                                                    int buflen)
{
    return hdd_cfg_get_config(mbssid_sap_dyn_ini_reg_table,
                              ARRAY_SIZE(mbssid_sap_dyn_ini_reg_table),
                              (v_U8_t *) &pAdapter->sap_dyn_ini_cfg,
                              WLAN_HDD_GET_CTX(pAdapter),
                              pBuf, buflen);
}
#endif

static VOS_STATUS find_cfg_item (tCfgIniEntry* iniTable, unsigned long entries,
    char *name, char** value)
{
   VOS_STATUS status = VOS_STATUS_E_FAILURE;
   unsigned long i;

   for (i = 0; i < entries; i++) {
     if (strcmp(iniTable[i].name, name) == 0) {
       *value = iniTable[i].value;
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "Found %s entry for Name=[%s] Value=[%s] ",
           WLAN_INI_FILE, name, *value);
       return VOS_STATUS_SUCCESS;
     }
   }

   return status;
}

static int parseHexDigit(char c)
{
  if (c >= '0' && c <= '9')
    return c-'0';
  if (c >= 'a' && c <= 'f')
    return c-'a'+10;
  if (c >= 'A' && c <= 'F')
    return c-'A'+10;

  return 0;
}

/* convert string to 6 bytes mac address
 * 00AA00BB00CC -> 0x00 0xAA 0x00 0xBB 0x00 0xCC
 */
static void update_mac_from_string(hdd_context_t *pHddCtx, tCfgIniEntry *macTable, int num)
{
   int i = 0, j = 0, res = 0;
   char *candidate = NULL;
   v_MACADDR_t macaddr[VOS_MAX_CONCURRENCY_PERSONA];

   memset(macaddr, 0, sizeof(macaddr));

   for (i = 0; i < num; i++)
   {
      candidate = macTable[i].value;
      for (j = 0; j < VOS_MAC_ADDR_SIZE; j++) {
         res = hex2bin(&macaddr[i].bytes[j], &candidate[(j<<1)], 1);
         if (res < 0)
            break;
      }
      if (res == 0 && !vos_is_macaddr_zero(&macaddr[i])) {
         vos_mem_copy((v_U8_t *)&pHddCtx->cfg_ini->intfMacAddr[i].bytes[0],
                      (v_U8_t *)&macaddr[i].bytes[0], VOS_MAC_ADDR_SIZE);
      }
   }
}

/*
 * This function tries to update mac address from cfg file.
 * It overwrites the MAC address if config file exist.
 */
VOS_STATUS hdd_update_mac_config(hdd_context_t *pHddCtx)
{
   int status, i = 0;
   const struct firmware *fw = NULL;
   char *line, *buffer = NULL;
   char *name, *value;
   tCfgIniEntry macTable[VOS_MAX_CONCURRENCY_PERSONA];
   tSirMacAddr customMacAddr;

   VOS_STATUS vos_status = VOS_STATUS_SUCCESS;

   memset(macTable, 0, sizeof(macTable));
   status = request_firmware(&fw, WLAN_MAC_FILE, pHddCtx->parent_dev);

   if (status)
   {
      hddLog(VOS_TRACE_LEVEL_WARN, "%s: request_firmware failed %d",
             __func__, status);
      vos_status = VOS_STATUS_E_FAILURE;
      return vos_status;
   }
   if (!fw || !fw->data || !fw->size)
   {
      hddLog(VOS_TRACE_LEVEL_FATAL, "%s: invalid firmware", __func__);
      vos_status = VOS_STATUS_E_INVAL;
      goto config_exit;
   }

   buffer = (char *)fw->data;

   /* data format:
    * Intf0MacAddress=00AA00BB00CC
    * Intf1MacAddress=00AA00BB00CD
    * END
    */
   while (buffer != NULL)
   {
      line = get_next_line(buffer);
      buffer = i_trim(buffer);

      if (strlen((char *)buffer) == 0 || *buffer == '#') {
         buffer = line;
         continue;
      }
      if (strncmp(buffer, "END", 3) == 0)
         break;

      name = buffer;
      buffer = strchr(buffer, '=');
      if (buffer) {
         *buffer++ = '\0';
         i_trim(name);
         if (strlen(name) != 0) {
            buffer = i_trim(buffer);
            if (strlen(buffer) == 12) {
               value = buffer;
               macTable[i].name = name;
               macTable[i++].value = value;
               if (i >= VOS_MAX_CONCURRENCY_PERSONA)
                  break;
            }
         }
      }
      buffer = line;
   }
   if (i <= VOS_MAX_CONCURRENCY_PERSONA) {
      hddLog(VOS_TRACE_LEVEL_INFO, "%s: %d Mac addresses provided", __func__, i);
   }
   else {
      hddLog(VOS_TRACE_LEVEL_ERROR, "%s: invalid number of Mac address provided, nMac = %d",
             __func__, i);
      vos_status = VOS_STATUS_E_INVAL;
      goto config_exit;
   }

   update_mac_from_string(pHddCtx, &macTable[0], i);

   vos_mem_copy(&customMacAddr,
                     &pHddCtx->cfg_ini->intfMacAddr[0].bytes[0],
                     sizeof(tSirMacAddr));
   sme_SetCustomMacAddr(customMacAddr);

config_exit:
   release_firmware(fw);
   return vos_status;
}

static VOS_STATUS hdd_apply_cfg_ini( hdd_context_t *pHddCtx, tCfgIniEntry* iniTable, unsigned long entries)
{
   VOS_STATUS match_status = VOS_STATUS_E_FAILURE;
   VOS_STATUS ret_status = VOS_STATUS_SUCCESS;
   unsigned int idx;
   void *pField;
   char *value_str = NULL;
   unsigned long len_value_str;
   char *candidate;
   v_U32_t value;
   v_S31_t svalue;
   void *pStructBase = pHddCtx->cfg_ini;
   REG_TABLE_ENTRY *pRegEntry = g_registry_table;
   unsigned long cRegTableEntries  = sizeof(g_registry_table) / sizeof( g_registry_table[ 0 ]);
   v_U32_t cbOutString;
   int i;
   int rv;

   // sanity test
   if (MAX_CFG_INI_ITEMS < cRegTableEntries)
   {
      hddLog(LOGE, "%s: MAX_CFG_INI_ITEMS too small, must be at least %ld",
             __func__, cRegTableEntries);
   }

   for ( idx = 0; idx < cRegTableEntries; idx++, pRegEntry++ )
   {
      //Calculate the address of the destination field in the structure.
      pField = ( (v_U8_t *)pStructBase )+ pRegEntry->VarOffset;

      match_status = find_cfg_item(iniTable, entries, pRegEntry->RegName, &value_str);

      if( (match_status != VOS_STATUS_SUCCESS) && ( pRegEntry->Flags & VAR_FLAGS_REQUIRED ) )
      {
         // If we could not read the cfg item and it is required, this is an error.
         hddLog(LOGE, "%s: Failed to read required config parameter %s",
            __func__, pRegEntry->RegName);
         ret_status = VOS_STATUS_E_FAILURE;
         break;
      }

      if ( ( WLAN_PARAM_Integer    == pRegEntry->RegType ) ||
           ( WLAN_PARAM_HexInteger == pRegEntry->RegType ) )
      {
         // If successfully read from the registry, use the value read.
         // If not, use the default value.
         if ( match_status == VOS_STATUS_SUCCESS && (WLAN_PARAM_Integer == pRegEntry->RegType)) {
            rv = kstrtou32(value_str, 10, &value);
            if (rv < 0) {
                hddLog(LOGE, "%s: Reg Parameter %s invalid. Enforcing default",
                       __func__, pRegEntry->RegName);
                value = pRegEntry->VarDefault;
            }
         }
         else if ( match_status == VOS_STATUS_SUCCESS && (WLAN_PARAM_HexInteger == pRegEntry->RegType)) {
            rv = kstrtou32(value_str, 16, &value);
            if (rv < 0) {
                hddLog(LOGE, "%s: Reg paramter %s invalid. Enforcing default",
                       __func__, pRegEntry->RegName);
                value = pRegEntry->VarDefault;
            }
         }
         else {
            value = pRegEntry->VarDefault;
         }

         // If this parameter needs range checking, do it here.
         if ( pRegEntry->Flags & VAR_FLAGS_RANGE_CHECK )
         {
            if ( value > pRegEntry->VarMax )
            {
               hddLog(LOGE, "%s: Reg Parameter %s > allowed Maximum [%u > %lu]. Enforcing Maximum",
                      __func__, pRegEntry->RegName, value, pRegEntry->VarMax );
               value = pRegEntry->VarMax;
            }

            if ( value < pRegEntry->VarMin )
            {
               hddLog(LOGE, "%s: Reg Parameter %s < allowed Minimum [%u < %lu]. Enforcing Minimum",
                      __func__, pRegEntry->RegName, value, pRegEntry->VarMin);
               value = pRegEntry->VarMin;
            }
         }
         // If this parameter needs range checking, do it here.
         else if ( pRegEntry->Flags & VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT )
         {
            if ( value > pRegEntry->VarMax )
            {
               hddLog(LOGE, "%s: Reg Parameter %s > allowed Maximum [%u > %lu]. Enforcing Default= %lu",
                  __func__, pRegEntry->RegName, value, pRegEntry->VarMax, pRegEntry->VarDefault  );
               value = pRegEntry->VarDefault;
            }

            if ( value < pRegEntry->VarMin )
            {
               hddLog(LOGE, "%s: Reg Parameter %s < allowed Minimum [%u < %lu]. Enforcing Default= %lu",
                  __func__, pRegEntry->RegName, value, pRegEntry->VarMin, pRegEntry->VarDefault  );
               value = pRegEntry->VarDefault;
            }
         }

         // Move the variable into the output field.
         memcpy( pField, &value, pRegEntry->VarSize );
      }
      else if ( WLAN_PARAM_SignedInteger == pRegEntry->RegType )
      {
         // If successfully read from the registry, use the value read.
         // If not, use the default value.
         if (VOS_STATUS_SUCCESS == match_status)
         {
            rv = kstrtos32(value_str, 10, &svalue);
            if (rv < 0) {
                hddLog(VOS_TRACE_LEVEL_WARN, "%s: Reg Parameter %s invalid. Enforcing Default",
                       __func__, pRegEntry->RegName);
                svalue = (v_S31_t)pRegEntry->VarDefault;
            }
         }
         else
         {
            svalue = (v_S31_t)pRegEntry->VarDefault;
         }

         // If this parameter needs range checking, do it here.
         if ( pRegEntry->Flags & VAR_FLAGS_RANGE_CHECK )
         {
            if ( svalue > (v_S31_t)pRegEntry->VarMax )
            {
               hddLog(LOGE, "%s: Reg Parameter %s > allowed Maximum "
                      "[%d > %d]. Enforcing Maximum", __func__,
                      pRegEntry->RegName, svalue, (int)pRegEntry->VarMax );
               svalue = (v_S31_t)pRegEntry->VarMax;
            }

            if ( svalue < (v_S31_t)pRegEntry->VarMin )
            {
               hddLog(LOGE, "%s: Reg Parameter %s < allowed Minimum "
                      "[%d < %d]. Enforcing Minimum",  __func__,
                      pRegEntry->RegName, svalue, (int)pRegEntry->VarMin);
               svalue = (v_S31_t)pRegEntry->VarMin;
            }
         }
         // If this parameter needs range checking, do it here.
         else if ( pRegEntry->Flags & VAR_FLAGS_RANGE_CHECK_ASSUME_DEFAULT )
         {
            if ( svalue > (v_S31_t)pRegEntry->VarMax )
            {
               hddLog(LOGE, "%s: Reg Parameter %s > allowed Maximum "
                      "[%d > %d]. Enforcing Default= %d",
                      __func__, pRegEntry->RegName, svalue,
                      (int)pRegEntry->VarMax,
                      (int)pRegEntry->VarDefault  );
               svalue = (v_S31_t)pRegEntry->VarDefault;
            }

            if ( svalue < (v_S31_t)pRegEntry->VarMin )
            {
               hddLog(LOGE, "%s: Reg Parameter %s < allowed Minimum "
                      "[%d < %d]. Enforcing Default= %d",
                      __func__, pRegEntry->RegName, svalue,
                      (int)pRegEntry->VarMin,
                      (int)pRegEntry->VarDefault);
               svalue = pRegEntry->VarDefault;
            }
         }

         // Move the variable into the output field.
         memcpy( pField, &svalue, pRegEntry->VarSize );
      }
      // Handle string parameters
      else if ( WLAN_PARAM_String == pRegEntry->RegType )
      {
#ifdef WLAN_CFG_DEBUG
         VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "RegName = %s, VarOffset %u VarSize %u VarDefault %s",
            pRegEntry->RegName, pRegEntry->VarOffset, pRegEntry->VarSize, (char*)pRegEntry->VarDefault);
#endif

         if ( match_status == VOS_STATUS_SUCCESS)
         {
            len_value_str = strlen(value_str);

            if(len_value_str > (pRegEntry->VarSize - 1)) {
               hddLog(LOGE, "%s: Invalid Value=[%s] specified for Name=[%s] in %s",
                  __func__, value_str, pRegEntry->RegName, WLAN_INI_FILE);
               cbOutString = utilMin( strlen( (char *)pRegEntry->VarDefault ), pRegEntry->VarSize - 1 );
               memcpy( pField, (void *)(pRegEntry->VarDefault), cbOutString );
               ( (v_U8_t *)pField )[ cbOutString ] = '\0';
            }
            else
            {
               memcpy( pField, (void *)(value_str), len_value_str);
               ( (v_U8_t *)pField )[ len_value_str ] = '\0';
            }
         }
         else
         {
            // Failed to read the string parameter from the registry.  Use the default.
            cbOutString = utilMin( strlen( (char *)pRegEntry->VarDefault ), pRegEntry->VarSize - 1 );
            memcpy( pField, (void *)(pRegEntry->VarDefault), cbOutString );
            ( (v_U8_t *)pField )[ cbOutString ] = '\0';
         }
      }
      else if ( WLAN_PARAM_MacAddr == pRegEntry->RegType )
      {
         if(pRegEntry->VarSize != VOS_MAC_ADDR_SIZE) {
               hddLog(LOGE, "%s: Invalid VarSize %u for Name=[%s]",
                   __func__, pRegEntry->VarSize, pRegEntry->RegName);
            continue;
         }
         candidate = (char*)pRegEntry->VarDefault;
         if ( match_status == VOS_STATUS_SUCCESS) {
            len_value_str = strlen(value_str);
            if(len_value_str != (VOS_MAC_ADDR_SIZE*2)) {
               hddLog(LOGE, "%s: Invalid MAC addr [%s] specified for Name=[%s] in %s",
                  __func__, value_str, pRegEntry->RegName, WLAN_INI_FILE);
            }
            else
               candidate = value_str;
         }
         //parse the string and store it in the byte array
         for(i=0; i<VOS_MAC_ADDR_SIZE; i++)
         {
            ((char*)pField)[i] =
              (char)(parseHexDigit(candidate[i*2])*16 + parseHexDigit(candidate[i*2+1]));
         }
      }
      else
      {
         hddLog(LOGE, "%s: Unknown param type for name[%s] in registry table",
            __func__, pRegEntry->RegName);
      }

      // did we successfully parse a cfg item for this parameter?
      if( (match_status == VOS_STATUS_SUCCESS) &&
          (idx < MAX_CFG_INI_ITEMS) )
      {
         set_bit(idx, (void *)&pHddCtx->cfg_ini->bExplicitCfg);
      }
   }

   print_hdd_cfg(pHddCtx);

   return( ret_status );
}

#ifdef WLAN_FEATURE_MBSSID
v_VOID_t hdd_mbssid_apply_def_cfg_ini(hdd_adapter_t *pAdapter)
{
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   hdd_config_t *iniConfig = pHddCtx->cfg_ini;
   mbssid_sap_dyn_ini_config_t *sap_ini_cfg = &pAdapter->sap_dyn_ini_cfg;

   sap_ini_cfg->apStartChannelNum = iniConfig->apStartChannelNum;
   sap_ini_cfg->apEndChannelNum = iniConfig->apEndChannelNum;
   sap_ini_cfg->apOperatingBand = iniConfig->apOperatingBand;
   sap_ini_cfg->apAutoChannelSelection = iniConfig->apAutoChannelSelection;
   sap_ini_cfg->acsScanBandPreference = iniConfig->acsScanBandPreference;
   sap_ini_cfg->acsBandSwitchThreshold = iniConfig->acsBandSwitchThreshold;
   vos_mem_copy(sap_ini_cfg->acsAllowedChnls, iniConfig->acsAllowedChnls,
                                                              CFG_MAX_STR_LEN);
   sap_ini_cfg->apForce11ACFor11n = iniConfig->apForce11ACFor11n;
}
#endif

eCsrPhyMode hdd_cfg_xlate_to_csr_phy_mode( eHddDot11Mode dot11Mode )
{
   switch (dot11Mode)
   {
      case (eHDD_DOT11_MODE_abg):
         return eCSR_DOT11_MODE_abg;
      case (eHDD_DOT11_MODE_11b):
         return eCSR_DOT11_MODE_11b;
      case (eHDD_DOT11_MODE_11g):
         return eCSR_DOT11_MODE_11g;
      default:
      case (eHDD_DOT11_MODE_11n):
         return eCSR_DOT11_MODE_11n;
      case (eHDD_DOT11_MODE_11g_ONLY):
         return eCSR_DOT11_MODE_11g_ONLY;
      case (eHDD_DOT11_MODE_11n_ONLY):
         return eCSR_DOT11_MODE_11n_ONLY;
      case (eHDD_DOT11_MODE_11b_ONLY):
         return eCSR_DOT11_MODE_11b_ONLY;
#ifdef WLAN_FEATURE_11AC
      case (eHDD_DOT11_MODE_11ac_ONLY):
         return eCSR_DOT11_MODE_11ac_ONLY;
      case (eHDD_DOT11_MODE_11ac):
         return eCSR_DOT11_MODE_11ac;
#endif
      case (eHDD_DOT11_MODE_AUTO):
         return eCSR_DOT11_MODE_AUTO;
   }

}

static void hdd_set_btc_config(hdd_context_t *pHddCtx)
{
   hdd_config_t *pConfig = pHddCtx->cfg_ini;
   tSmeBtcConfig btcParams;
   int i;

   sme_BtcGetConfig(pHddCtx->hHal, &btcParams);

   btcParams.btcExecutionMode = pConfig->btcExecutionMode;

   for (i = 0; i < 6; i++) {
      btcParams.mwsCoexConfig[i] = pConfig->mwsCoexConfig[i];
   }

   sme_BtcSetConfig(pHddCtx->hHal, &btcParams);
}

static void hdd_set_power_save_config(hdd_context_t *pHddCtx, tSmeConfigParams *smeConfig)
{
   hdd_config_t *pConfig = pHddCtx->cfg_ini;

   tPmcBmpsConfigParams bmpsParams;

   sme_GetConfigPowerSave(pHddCtx->hHal, ePMC_BEACON_MODE_POWER_SAVE, &bmpsParams);

   if (strcmp(pConfig->PowerUsageControl, "Min") == 0)
   {
      smeConfig->csrConfig.impsSleepTime   = pConfig->nImpsMinSleepTime;
      bmpsParams.bmpsPeriod                = pConfig->nBmpsMinListenInterval;
      bmpsParams.enableBeaconEarlyTermination = pConfig->fEnableBeaconEarlyTermination;
      bmpsParams.bcnEarlyTermWakeInterval  = pConfig->bcnEarlyTermWakeInterval;
   }
   if (strcmp(pConfig->PowerUsageControl, "Max") == 0)
   {
      smeConfig->csrConfig.impsSleepTime   = pConfig->nImpsMaxSleepTime;
      bmpsParams.bmpsPeriod                = pConfig->nBmpsMaxListenInterval;
      bmpsParams.enableBeaconEarlyTermination = pConfig->fEnableBeaconEarlyTermination;
      bmpsParams.bcnEarlyTermWakeInterval  = pConfig->bcnEarlyTermWakeInterval;
   }
   if (strcmp(pConfig->PowerUsageControl, "Mod") == 0)
   {
      smeConfig->csrConfig.impsSleepTime   = pConfig->nImpsModSleepTime;
      bmpsParams.bmpsPeriod                = pConfig->nBmpsModListenInterval;
      bmpsParams.enableBeaconEarlyTermination = pConfig->fEnableBeaconEarlyTermination;
      bmpsParams.bcnEarlyTermWakeInterval  = pConfig->bcnEarlyTermWakeInterval;
   }

   if (pConfig->fIsImpsEnabled)
   {
      sme_EnablePowerSave (pHddCtx->hHal, ePMC_IDLE_MODE_POWER_SAVE);
   }
   else
   {
      sme_DisablePowerSave (pHddCtx->hHal, ePMC_IDLE_MODE_POWER_SAVE);
   }

  /*If isAndroidPsEn is 1 then Host driver and above layers control the PS mechanism
    If Value set to 0 Driver/Core Stack internally control the Power saving mechanism */
   sme_SetHostPowerSave (pHddCtx->hHal, pConfig->isAndroidPsEn);

   if (pConfig->fIsBmpsEnabled)
   {
      sme_EnablePowerSave (pHddCtx->hHal, ePMC_BEACON_MODE_POWER_SAVE);
   }
   else
   {
      sme_DisablePowerSave (pHddCtx->hHal, ePMC_BEACON_MODE_POWER_SAVE);
   }

   bmpsParams.trafficMeasurePeriod = pConfig->nAutoBmpsTimerValue;

   if (sme_SetConfigPowerSave(pHddCtx->hHal, ePMC_BEACON_MODE_POWER_SAVE, &bmpsParams)== eHAL_STATUS_FAILURE)
   {
      hddLog(LOGE, "SetConfigPowerSave failed to set BMPS params");
   }

   if(pConfig->fIsAutoBmpsTimerEnabled)
   {
      sme_StartAutoBmpsTimer(pHddCtx->hHal);
   }

}

static void hdd_set_power_save_offload_config(hdd_context_t *pHddCtx)
{
   hdd_config_t *pConfig = pHddCtx->cfg_ini;
   tANI_U32 listenInterval = 0;

   if (strcmp(pConfig->PowerUsageControl, "Min") == 0)
   {
      listenInterval = pConfig->nBmpsMinListenInterval;
   }
   else if (strcmp(pConfig->PowerUsageControl, "Max") == 0)
   {
      listenInterval = pConfig->nBmpsMaxListenInterval;
   }
   else if (strcmp(pConfig->PowerUsageControl, "Mod") == 0)
   {
      listenInterval = pConfig->nBmpsModListenInterval;
   }

   /*
    * Based on Mode Set the LI
    * Otherwise default LI value of 1 will
    * be taken
    */
   if (listenInterval)
   {
      /*
       * setcfg for listenInterval.
       * Make sure CFG is updated because PE reads this
       * from CFG at the time of assoc or reassoc
       */
      ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_LISTEN_INTERVAL, listenInterval,
                   NULL, eANI_BOOLEAN_FALSE);
   }

   if(pConfig->fIsBmpsEnabled)
   {
      sme_ConfigEnablePowerSave(pHddCtx->hHal, ePMC_BEACON_MODE_POWER_SAVE);
   }
   else
   {
      sme_ConfigDisablePowerSave(pHddCtx->hHal, ePMC_BEACON_MODE_POWER_SAVE);
   }
}

VOS_STATUS hdd_set_idle_ps_config(hdd_context_t *pHddCtx, v_U32_t val)
{
   hdd_config_t *pConfig = pHddCtx->cfg_ini;
   VOS_STATUS status = VOS_STATUS_SUCCESS;

   hddLog(LOG1, "hdd_set_idle_ps_config: Enter Val %d", val);

   if(pConfig->fIsImpsEnabled)
   {
      status = sme_SetIdlePowersaveConfig(pHddCtx->pvosContext, val);
      if(VOS_STATUS_SUCCESS != status)
      {
         hddLog(LOGE, "Fail to Set Idle PS Config val %d", val);
      }
   }
   else
   {
      hddLog(LOG1, "hdd_set_idle_ps_config: IMPS not enabled in ini");
   }
   return status;
}

VOS_STATUS hdd_string_to_u8_array( char *str, tANI_U8 *intArray, tANI_U8 *len,
    tANI_U8 intArrayMaxLen )
{
   char *s = str;

   if( str == NULL || intArray == NULL || len == NULL )
   {
      return VOS_STATUS_E_INVAL;
   }
   *len = 0;

   while ( (s != NULL) && (*len < intArrayMaxLen) )
   {
      int val;
      /* Increment length only if sscanf successfully extracted one element.
         Any other return value means error. Ignore it. */
      if( sscanf(s, "%d", &val ) == 1 )
      {
         intArray[*len] = (tANI_U8) val;
         *len += 1;
      }
      s = strpbrk( s, "," );
      if( s )
         s++;
   }

   return VOS_STATUS_SUCCESS;

}


v_BOOL_t hdd_update_config_dat( hdd_context_t *pHddCtx )
{
   v_BOOL_t  fStatus = TRUE;
   tANI_U32 val;
   tANI_U16 val16;

   hdd_config_t *pConfig = pHddCtx->cfg_ini;
   tSirMacHTCapabilityInfo *phtCapInfo;


   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_SHORT_GI_20MHZ,
      pConfig->ShortGI20MhzEnable, NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_SHORT_GI_20MHZ to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_BA_AUTO_SETUP, pConfig->BlockAckAutoSetup,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_BA_AUTO_SETUP to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_FIXED_RATE, pConfig->TxRate,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_FIXED_RATE to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_MAX_RX_AMPDU_FACTOR,
      pConfig->MaxRxAmpduFactor, NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE,"Could not pass on WNI_CFG_HT_AMPDU_PARAMS_MAX_RX_AMPDU_FACTOR to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_SHORT_PREAMBLE, pConfig->fIsShortPreamble,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE,"Could not pass on WNI_CFG_SHORT_PREAMBLE to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_PASSIVE_MINIMUM_CHANNEL_TIME,
        pConfig->nPassiveMinChnTime, NULL,
        eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_PASSIVE_MINIMUM_CHANNEL_TIME"
                     " to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_PASSIVE_MAXIMUM_CHANNEL_TIME,
        pConfig->nPassiveMaxChnTime, NULL,
        eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_PASSIVE_MAXIMUM_CHANNEL_TIME"
                     " to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_BEACON_INTERVAL, pConfig->nBeaconInterval,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_BEACON_INTERVAL to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_MAX_PS_POLL, pConfig->nMaxPsPoll,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_MAX_PS_POLL to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_CURRENT_RX_ANTENNA, pConfig-> nRxAnt, NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Failure: Could not pass on WNI_CFG_CURRENT_RX_ANTENNA configuration info to HAL");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_LOW_GAIN_OVERRIDE, pConfig->fIsLowGainOverride,
         NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_LOW_GAIN_OVERRIDE to HAL");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_RSSI_FILTER_PERIOD, pConfig->nRssiFilterPeriod,
         NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_RSSI_FILTER_PERIOD to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_IGNORE_DTIM, pConfig->fIgnoreDtim,
         NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_IGNORE_DTIM configuration to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_PS_ENABLE_HEART_BEAT, pConfig->fEnableFwHeartBeatMonitoring,
                    NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Failure: Could not pass on WNI_CFG_PS_HEART_BEAT configuration info to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_PS_ENABLE_BCN_FILTER, pConfig->fEnableFwBeaconFiltering,
                    NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Failure: Could not pass on WNI_CFG_PS_BCN_FILTER configuration info to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_PS_ENABLE_RSSI_MONITOR, pConfig->fEnableFwRssiMonitoring,
                    NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Failure: Could not pass on WNI_CFG_PS_RSSI_MONITOR configuration info to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_PS_DATA_INACTIVITY_TIMEOUT, pConfig->nDataInactivityTimeout,
                    NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE,"Failure: Could not pass on WNI_CFG_PS_DATA_INACTIVITY_TIMEOUT configuration info to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_NTH_BEACON_FILTER, pConfig->nthBeaconFilter,
                    NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE,"Failure: Could not pass on WNI_CFG_NTH_BEACON_FILTER configuration info to CCM");
   }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_ENABLE_LTE_COEX, pConfig->enableLTECoex,
        NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE, "Could not pass on WNI_CFG_ENABLE_LTE_COEX to CCM");
     }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_ENABLE_PHY_AGC_LISTEN_MODE, pConfig->nEnableListenMode,
        NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE, "Could not pass on WNI_CFG_ENABLE_PHY_AGC_LISTEN_MODE to CCM");
     }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_AP_KEEP_ALIVE_TIMEOUT, pConfig->apKeepAlivePeriod,
        NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE, "Could not pass on WNI_CFG_AP_KEEP_ALIVE_TIMEOUT to CCM");
     }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_GO_KEEP_ALIVE_TIMEOUT, pConfig->goKeepAlivePeriod,
        NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE, "Could not pass on WNI_CFG_GO_KEEP_ALIVE_TIMEOUT to CCM");
     }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_AP_LINK_MONITOR_TIMEOUT, pConfig->apLinkMonitorPeriod,
        NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE, "Could not pass on WNI_CFG_AP_LINK_MONITOR_TIMEOUT to CCM");
     }

    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_GO_LINK_MONITOR_TIMEOUT, pConfig->goLinkMonitorPeriod,
        NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE, "Could not pass on WNI_CFG_GO_LINK_MONITOR_TIMEOUT to CCM");
     }


#if defined WLAN_FEATURE_VOWIFI
    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_RRM_ENABLED, pConfig->fRrmEnable,
                     NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
       fStatus = FALSE;
       hddLog(LOGE,"Failure: Could not pass on WNI_CFG_RRM_ENABLE configuration info to CCM");
    }

    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_RRM_OPERATING_CHAN_MAX, pConfig->nInChanMeasMaxDuration,
                     NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
       fStatus = FALSE;
       hddLog(LOGE,"Failure: Could not pass on WNI_CFG_RRM_OPERATING_CHAN_MAX configuration info to CCM");
    }

    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_RRM_NON_OPERATING_CHAN_MAX, pConfig->nOutChanMeasMaxDuration,
                     NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
       fStatus = FALSE;
       hddLog(LOGE,"Failure: Could not pass on WNI_CFG_RRM_OUT_CHAN_MAX configuration info to CCM");
    }

    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_MCAST_BCAST_FILTER_SETTING, pConfig->mcastBcastFilterSetting,
                     NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
#endif

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_SINGLE_TID_RC, pConfig->bSingleTidRc,
                      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE,"Failure: Could not pass on WNI_CFG_SINGLE_TID_RC configuration info to CCM");
     }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TELE_BCN_WAKEUP_EN, pConfig->teleBcnWakeupEn,
                      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE,"Failure: Could not pass on WNI_CFG_TELE_BCN_WAKEUP_EN configuration info to CCM");
     }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TELE_BCN_TRANS_LI, pConfig->nTeleBcnTransListenInterval,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
      fStatus = FALSE;
      hddLog(LOGE,"Failure: Could not pass on WNI_CFG_TELE_BCN_TRANS_LI configuration info to CCM");
    }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TELE_BCN_MAX_LI, pConfig->nTeleBcnMaxListenInterval,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
      fStatus = FALSE;
      hddLog(LOGE,"Failure: Could not pass on WNI_CFG_TELE_BCN_MAX_LI configuration info to CCM");
    }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TELE_BCN_TRANS_LI_IDLE_BCNS, pConfig->nTeleBcnTransLiNumIdleBeacons,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
      fStatus = FALSE;
      hddLog(LOGE,"Failure: Could not pass on WNI_CFG_TELE_BCN_TRANS_LI_IDLE_BCNS configuration info to CCM");
    }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TELE_BCN_MAX_LI_IDLE_BCNS, pConfig->nTeleBcnMaxLiNumIdleBeacons,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
      fStatus = FALSE;
      hddLog(LOGE,"Failure: Could not pass on WNI_CFG_TELE_BCN_MAX_LI_IDLE_BCNS configuration info to CCM");
    }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_RF_SETTLING_TIME_CLK, pConfig->rfSettlingTimeUs,
        NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
        fStatus = FALSE;
        hddLog(LOGE,"Failure: Could not pass on WNI_CFG_RF_SETTLING_TIME_CLK configuration info to CCM");
    }

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_INFRA_STA_KEEP_ALIVE_PERIOD, pConfig->infraStaKeepAlivePeriod,
                      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE,"Failure: Could not pass on WNI_CFG_INFRA_STA_KEEP_ALIVE_PERIOD configuration info to CCM");
     }
    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_DYNAMIC_PS_POLL_VALUE, pConfig->dynamicPsPollValue,
                     NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
       fStatus = FALSE;
       hddLog(LOGE,"Failure: Could not pass on WNI_CFG_DYNAMIC_PS_POLL_VALUE configuration info to CCM");
    }

    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_PS_NULLDATA_AP_RESP_TIMEOUT, pConfig->nNullDataApRespTimeout,
               NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
       fStatus = FALSE;
       hddLog(LOGE,"Failure: Could not pass on WNI_CFG_PS_NULLDATA_DELAY_TIMEOUT configuration info to CCM");
    }

    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_AP_DATA_AVAIL_POLL_PERIOD, pConfig->apDataAvailPollPeriodInMs,
               NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
    {
        fStatus = FALSE;
        hddLog(LOGE,"Failure: Could not pass on WNI_CFG_AP_DATA_AVAIL_POLL_PERIOD configuration info to CCM");
    }
    if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_FRAGMENTATION_THRESHOLD, pConfig->FragmentationThreshold,
                   NULL, eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
    {
        fStatus = FALSE;
        hddLog(LOGE,"Failure: Could not pass on WNI_CFG_FRAGMENTATION_THRESHOLD configuration info to CCM");
    }
    if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_RTS_THRESHOLD, pConfig->RTSThreshold,
                        NULL, eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
    {
        fStatus = FALSE;
        hddLog(LOGE,"Failure: Could not pass on WNI_CFG_RTS_THRESHOLD configuration info to CCM");
    }

    if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_11D_ENABLED, pConfig->Is11dSupportEnabled,
                        NULL, eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
    {
        fStatus = FALSE;
        hddLog(LOGE,"Failure: Could not pass on WNI_CFG_11D_ENABLED configuration info to CCM");
    }

    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_DFS_MASTER_ENABLED,
                     pConfig->enableDFSMasterCap, NULL,
                     eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE) {
        fStatus = FALSE;
        hddLog(LOGE,
               "Failure: Could not set value for WNI_CFG_DFS_MASTER_ENABLED");
    }

    if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_ENABLE_TXBF_20MHZ,
                     pConfig->enableTxBFin20MHz, NULL,
                     eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE) {
        fStatus = FALSE;
        hddLog(LOGE,
               "Failure: Could not set value for WNI_CFG_VHT_ENABLE_TXBF_20MHZ");
    }

    if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_HEART_BEAT_THRESHOLD, pConfig->HeartbeatThresh24,
                        NULL, eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
    {
        fStatus = FALSE;
        hddLog(LOGE,"Failure: Could not pass on WNI_CFG_HEART_BEAT_THRESHOLD configuration info to CCM");
    }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_AP_DATA_AVAIL_POLL_PERIOD, pConfig->apDataAvailPollPeriodInMs,
               NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE,"Failure: Could not pass on WNI_CFG_AP_DATA_AVAIL_POLL_PERIOD configuration info to CCM");
   }

   if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_ENABLE_CLOSE_LOOP,
                   pConfig->enableCloseLoop, NULL, eANI_BOOLEAN_FALSE)
       ==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_ENABLE_CLOSE_LOOP to CCM");
   }

   if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TX_PWR_CTRL_ENABLE,
                   pConfig->enableAutomaticTxPowerControl, NULL, eANI_BOOLEAN_FALSE)
                   ==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_TX_PWR_CTRL_ENABLE to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_SHORT_GI_40MHZ,
      pConfig->ShortGI40MhzEnable, NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_SHORT_GI_40MHZ to CCM");
   }


     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_ENABLE_MC_ADDR_LIST, pConfig->fEnableMCAddrList,
        NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE, "Could not pass on WNI_CFG_ENABLE_MC_ADDR_LIST to CCM");
     }

#ifdef WLAN_FEATURE_11AC
   /* Based on cfg.ini, update the Basic MCS set, RX/TX MCS map in the cfg.dat */
   /* valid values are 0(MCS0-7), 1(MCS0-8), 2(MCS0-9) */
   /* we update only the least significant 2 bits in the corresponding fields */
   if( (pConfig->dot11Mode == eHDD_DOT11_MODE_AUTO) ||
       (pConfig->dot11Mode == eHDD_DOT11_MODE_11ac_ONLY) ||
       (pConfig->dot11Mode == eHDD_DOT11_MODE_11ac) )
   {
       {
           tANI_U32 temp = 0;

           ccmCfgGetInt(pHddCtx->hHal, WNI_CFG_VHT_BASIC_MCS_SET, &temp);
           temp = (temp & 0xFFFC) | pConfig->vhtRxMCS;
           if (pConfig->enable2x2)
               temp = (temp & 0xFFF3) | (pConfig->vhtRxMCS2x2 << 2);

           if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_BASIC_MCS_SET,
                           temp, NULL, eANI_BOOLEAN_FALSE)
               ==eHAL_STATUS_FAILURE)
           {
               fStatus = FALSE;
               hddLog(LOGE, "Could not pass on WNI_CFG_VHT_BASIC_MCS_SET to CCM");
           }

           ccmCfgGetInt(pHddCtx->hHal, WNI_CFG_VHT_RX_MCS_MAP, &temp);
           temp = (temp & 0xFFFC) | pConfig->vhtRxMCS;
           if (pConfig->enable2x2)
               temp = (temp & 0xFFF3) | (pConfig->vhtRxMCS2x2 << 2);

           if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_RX_MCS_MAP,
                           temp, NULL, eANI_BOOLEAN_FALSE)
               ==eHAL_STATUS_FAILURE)
           {
              fStatus = FALSE;
              hddLog(LOGE, "Could not pass on WNI_CFG_VHT_RX_MCS_MAP to CCM");
           }

           ccmCfgGetInt(pHddCtx->hHal, WNI_CFG_VHT_TX_MCS_MAP, &temp);
           temp = (temp & 0xFFFC) | pConfig->vhtTxMCS;
           if (pConfig->enable2x2)
               temp = (temp & 0xFFF3) | (pConfig->vhtTxMCS2x2 << 2);

           VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
                    "vhtRxMCS2x2 - %x temp - %u enable2x2 %d",
                    pConfig->vhtRxMCS2x2, temp, pConfig->enable2x2);

           if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_TX_MCS_MAP,
                           temp, NULL, eANI_BOOLEAN_FALSE)
               ==eHAL_STATUS_FAILURE)
           {
               fStatus = FALSE;
               hddLog(LOGE, "Could not pass on WNI_CFG_VHT_TX_MCS_MAP to CCM");
           }
           /* Currently shortGI40Mhz is used for shortGI80Mhz */
           if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_SHORT_GI_80MHZ,
               pConfig->ShortGI40MhzEnable, NULL, eANI_BOOLEAN_FALSE)
               == eHAL_STATUS_FAILURE)
           {
               fStatus = FALSE;
               hddLog(LOGE, "Could not pass WNI_VHT_SHORT_GI_80MHZ to CCM");
           }
           // Hardware is capable of doing 128K AMPDU in 11AC mode
           if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_AMPDU_LEN_EXPONENT,
                           pConfig->fVhtAmpduLenExponent, NULL,
                           eANI_BOOLEAN_FALSE)
               == eHAL_STATUS_FAILURE)
           {
               fStatus = FALSE;
               hddLog(LOGE, "Could not pass on WNI_CFG_VHT_AMPDU_LEN_EXPONENT to CCM");
           }

           /* Change MU Bformee only when TxBF is enabled */
           if (pConfig->enableTxBF)
           {
               ccmCfgGetInt(pHddCtx->hHal, WNI_CFG_VHT_MU_BEAMFORMEE_CAP,
                            &temp);

               if(temp != pConfig->enableMuBformee)
               {
                   if(ccmCfgSetInt(pHddCtx->hHal,
                           WNI_CFG_VHT_MU_BEAMFORMEE_CAP,
                           pConfig->enableMuBformee, NULL,
                           eANI_BOOLEAN_FALSE) ==eHAL_STATUS_FAILURE)
                   {
                        fStatus = FALSE;
                        hddLog(LOGE, "Could not pass on\
                               WNI_CFG_VHT_MU_BEAMFORMEE_CAP to CCM");
                   }
               }
          }
           if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_MAX_MPDU_LENGTH,
                           pConfig->vhtMpduLen, NULL,
                           eANI_BOOLEAN_FALSE)
               == eHAL_STATUS_FAILURE)
           {
               fStatus = FALSE;
               hddLog(LOGE, "Could not pass on WNI_CFG_VHT_MAX_MPDU_LENGTH to CCM");
           }
       }
   }
#endif

     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_NUM_BUFF_ADVERT,pConfig->numBuffAdvert,
        NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
     {
        fStatus = FALSE;
        hddLog(LOGE, "Could not pass on WNI_CFG_NUM_BUFF_ADVERT to CCM");
     }

     if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_HT_RX_STBC,
                     pConfig->enableRxSTBC, NULL, eANI_BOOLEAN_FALSE)
         ==eHAL_STATUS_FAILURE)
     {
         fStatus = FALSE;
         hddLog(LOGE, "Could not pass on WNI_CFG_HT_RX_STBC to CCM");
     }

     ccmCfgGetInt(pHddCtx->hHal, WNI_CFG_HT_CAP_INFO, &val);
     val16 = (tANI_U16)val;
     phtCapInfo = (tSirMacHTCapabilityInfo *)&val16;
     phtCapInfo->rxSTBC = pConfig->enableRxSTBC;
     phtCapInfo->txSTBC = pConfig->enableTxSTBC;
     phtCapInfo->advCodingCap = pConfig->enableRxLDPC;
     val = val16;
     if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_HT_CAP_INFO,
                      val, NULL, eANI_BOOLEAN_FALSE)
         == eHAL_STATUS_FAILURE)
     {
         fStatus = FALSE;
         hddLog(LOGE, "Could not pass on WNI_CFG_HT_CAP_INFO to CCM");
     }

     if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_RXSTBC,
                     pConfig->enableRxSTBC, NULL, eANI_BOOLEAN_FALSE)
         ==eHAL_STATUS_FAILURE)
     {
         fStatus = FALSE;
         hddLog(LOGE, "Could not pass on WNI_CFG_VHT_RXSTBC to CCM");
     }

     if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_TXSTBC,
                     pConfig->enableTxSTBC, NULL, eANI_BOOLEAN_FALSE)
         ==eHAL_STATUS_FAILURE)
     {
         fStatus = FALSE;
         hddLog(LOGE, "Could not pass on WNI_CFG_VHT_TXSTBC to CCM");
     }

     if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_VHT_LDPC_CODING_CAP,
                     pConfig->enableRxLDPC, NULL, eANI_BOOLEAN_FALSE)
         ==eHAL_STATUS_FAILURE)
     {
         fStatus = FALSE;
         hddLog(LOGE, "Could not pass on WNI_CFG_VHT_LDPC_CODING_CAP to CCM");
     }

#ifdef WLAN_SOFTAP_VSTA_FEATURE
     if(pConfig->fEnableVSTASupport)
     {
        ccmCfgGetInt(pHddCtx->hHal, WNI_CFG_ASSOC_STA_LIMIT, &val);
        if( val <= WNI_CFG_ASSOC_STA_LIMIT_STADEF)
            val = WNI_CFG_ASSOC_STA_LIMIT_STAMAX;
     }
     else
     {
            val = pConfig->maxNumberOfPeers;

     }
     if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_ASSOC_STA_LIMIT, val,
         NULL, eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
     {
         fStatus = FALSE;
         hddLog(LOGE,"Failure: Could not pass on WNI_CFG_ASSOC_STA_LIMIT configuration info to CCM");
     }
#endif
   if(ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_ENABLE_LPWR_IMG_TRANSITION,
                   pConfig->enableLpwrImgTransition, NULL, eANI_BOOLEAN_FALSE)
       ==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_ENABLE_LPWR_IMG_TRANSITION to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED, pConfig->enableMCCAdaptiveScheduler,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_ENABLE_MCC_ADAPTIVE_SCHED to CCM");
   }
   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_DISABLE_LDPC_WITH_TXBF_AP, pConfig->disableLDPCWithTxbfAP,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_DISABLE_LDPC_WITH_TXBF_AP to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_DYNAMIC_THRESHOLD_ZERO, pConfig->retryLimitZero,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_DYNAMIC_THRESHOLD_ZERO to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_DYNAMIC_THRESHOLD_ONE, pConfig->retryLimitOne,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_DYNAMIC_THRESHOLD_ONE to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_DYNAMIC_THRESHOLD_TWO, pConfig->retryLimitTwo,
      NULL, eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_DYNAMIC_THRESHOLD_TWO to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_MAX_MEDIUM_TIME, pConfig->cfgMaxMediumTime,
      NULL, eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_MAX_MEDIUM_TIME to CCM");
   }

#ifdef FEATURE_WLAN_TDLS

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TDLS_QOS_WMM_UAPSD_MASK,
                    pConfig->fTDLSUapsdMask, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_TDLS_QOS_WMM_UAPSD_MASK to CCM");
   }
   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TDLS_BUF_STA_ENABLED,
                    pConfig->fEnableTDLSBufferSta, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_TDLS_BUF_STA_ENABLED to CCM");
   }
   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TDLS_PUAPSD_INACT_TIME,
                    pConfig->fTDLSPuapsdInactivityTimer, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_TDLS_PUAPSD_INACT_TIME to CCM");
   }
   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TDLS_RX_FRAME_THRESHOLD,
                    pConfig->fTDLSRxFrameThreshold, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_TDLS_RX_FRAME_THRESHOLD to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TDLS_OFF_CHANNEL_ENABLED,
                    pConfig->fEnableTDLSOffChannel, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_TDLS_BUF_STA_ENABLED to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_TDLS_WMM_MODE_ENABLED,
                    pConfig->fEnableTDLSWmmMode, NULL,
                    eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_TDLS_WMM_MODE_ENABLED to CCM");
   }

#endif

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_ENABLE_ADAPT_RX_DRAIN,
                    pConfig->fEnableAdaptRxDrain, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_ENABLE_ADAPT_RX_DRAIN to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_FLEX_CONNECT_POWER_FACTOR,
                    pConfig->flexConnectPowerFactor, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Failure: Could not pass on "
             "WNI_CFG_FLEX_CONNECT_POWER_FACTOR to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_ANTENNA_DIVESITY,
                    pConfig->antennaDiversity, NULL,
                    eANI_BOOLEAN_FALSE)==eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_ANTENNA_DIVESITY to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal,
                    WNI_CFG_DEFAULT_RATE_INDEX_24GHZ,
                    pConfig->defaultRateIndex24Ghz,
                    NULL, eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
       fStatus = FALSE;
       hddLog(LOGE, "Could not pass on WNI_CFG_DEFAULT_RATE_INDEX_24GHZ to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal,
                    WNI_CFG_DEBUG_P2P_REMAIN_ON_CHANNEL,
                    pConfig->debugP2pRemainOnChannel,
                    NULL, eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
       fStatus = FALSE;
       hddLog(LOGE,
              "Could not pass on WNI_CFG_DEBUG_P2P_REMAIN_ON_CHANNEL to CCM");
   }

#ifdef WLAN_FEATURE_11W
   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_PMF_SA_QUERY_MAX_RETRIES,
                    pConfig->pmfSaQueryMaxRetries, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_SA_QUERY_MAX_RETRIES to CCM");
   }

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_PMF_SA_QUERY_RETRY_INTERVAL,
                    pConfig->pmfSaQueryRetryInterval, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_SA_QUERY_RETRY_INTERVAL to CCM");
   }
#endif

   if (ccmCfgSetInt(pHddCtx->hHal, WNI_CFG_IBSS_ATIM_WIN_SIZE,
                    pConfig->ibssATIMWinSize, NULL,
                    eANI_BOOLEAN_FALSE) == eHAL_STATUS_FAILURE)
   {
      fStatus = FALSE;
      hddLog(LOGE, "Could not pass on WNI_CFG_IBSS_ATIM_WIN_SIZE to CCM");
   }
   return fStatus;
}


/**---------------------------------------------------------------------------

  \brief hdd_set_sme_config() -

   This function initializes the sme configuration parameters

  \param  - pHddCtx - Pointer to the HDD Adapter.

  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/

VOS_STATUS hdd_set_sme_config( hdd_context_t *pHddCtx )
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   eHalStatus halStatus;
   tSmeConfigParams *smeConfig;

   hdd_config_t *pConfig = pHddCtx->cfg_ini;

   smeConfig = vos_mem_malloc(sizeof(*smeConfig));
   if (NULL == smeConfig)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
              "%s: unable to allocate smeConfig", __func__);
      return VOS_STATUS_E_NOMEM;
   }
   vos_mem_zero(smeConfig, sizeof(*smeConfig));

   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
              "%s bWmmIsEnabled=%d 802_11e_enabled=%d dot11Mode=%d", __func__,
              pConfig->WmmMode, pConfig->b80211eIsEnabled, pConfig->dot11Mode);

   // Config params obtained from the registry
   /* To Do */
   // set regulatory information here

   smeConfig->csrConfig.RTSThreshold             = pConfig->RTSThreshold;
   smeConfig->csrConfig.FragmentationThreshold   = pConfig->FragmentationThreshold;
   smeConfig->csrConfig.shortSlotTime            = pConfig->ShortSlotTimeEnabled;
   smeConfig->csrConfig.Is11dSupportEnabled      = pConfig->Is11dSupportEnabled;
   smeConfig->csrConfig.HeartbeatThresh24        = pConfig->HeartbeatThresh24;

   smeConfig->csrConfig.phyMode                  = hdd_cfg_xlate_to_csr_phy_mode ( pConfig->dot11Mode );

   smeConfig->csrConfig.channelBondingMode24GHz  = pConfig->nChannelBondingMode24GHz;
   smeConfig->csrConfig.channelBondingMode5GHz   = pConfig->nChannelBondingMode5GHz;
   smeConfig->csrConfig.TxRate                   = pConfig->TxRate;
   smeConfig->csrConfig.nScanResultAgeCount      = pConfig->ScanResultAgeCount;
   smeConfig->csrConfig.scanAgeTimeNCNPS         = pConfig->nScanAgeTimeNCNPS;
   smeConfig->csrConfig.scanAgeTimeNCPS          = pConfig->nScanAgeTimeNCPS;
   smeConfig->csrConfig.scanAgeTimeCNPS          = pConfig->nScanAgeTimeCNPS;
   smeConfig->csrConfig.scanAgeTimeCPS           = pConfig->nScanAgeTimeCPS;
   smeConfig->csrConfig.AdHocChannel24           = pConfig->OperatingChannel;
   smeConfig->csrConfig.fEnforce11dChannels      = pConfig->fEnforce11dChannels;
   smeConfig->csrConfig.fSupplicantCountryCodeHasPriority     = pConfig->fSupplicantCountryCodeHasPriority;
   smeConfig->csrConfig.fEnforceCountryCodeMatch = pConfig->fEnforceCountryCodeMatch;
   smeConfig->csrConfig.fEnforceDefaultDomain    = pConfig->fEnforceDefaultDomain;
   smeConfig->csrConfig.bCatRssiOffset           = pConfig->nRssiCatGap;
   smeConfig->csrConfig.vccRssiThreshold         = pConfig->nVccRssiTrigger;
   smeConfig->csrConfig.vccUlMacLossThreshold    = pConfig->nVccUlMacLossThreshold;
   smeConfig->csrConfig.nRoamingTime             = pConfig->nRoamingTime;
   smeConfig->csrConfig.IsIdleScanEnabled        = pConfig->nEnableIdleScan;
   smeConfig->csrConfig.nInitialDwellTime        = pConfig->nInitialDwellTime;
   smeConfig->csrConfig.nActiveMaxChnTime        = pConfig->nActiveMaxChnTime;
   smeConfig->csrConfig.nActiveMinChnTime        = pConfig->nActiveMinChnTime;
   smeConfig->csrConfig.nPassiveMaxChnTime       = pConfig->nPassiveMaxChnTime;
   smeConfig->csrConfig.nPassiveMinChnTime       = pConfig->nPassiveMinChnTime;
   smeConfig->csrConfig.nActiveMaxChnTimeBtc     = pConfig->nActiveMaxChnTimeBtc;
   smeConfig->csrConfig.nActiveMinChnTimeBtc     = pConfig->nActiveMinChnTimeBtc;
   smeConfig->csrConfig.disableAggWithBtc        = pConfig->disableAggWithBtc;
#ifdef WLAN_AP_STA_CONCURRENCY
   smeConfig->csrConfig.nActiveMaxChnTimeConc    = pConfig->nActiveMaxChnTimeConc;
   smeConfig->csrConfig.nActiveMinChnTimeConc    = pConfig->nActiveMinChnTimeConc;
   smeConfig->csrConfig.nPassiveMaxChnTimeConc   = pConfig->nPassiveMaxChnTimeConc;
   smeConfig->csrConfig.nPassiveMinChnTimeConc   = pConfig->nPassiveMinChnTimeConc;
   smeConfig->csrConfig.nRestTimeConc            = pConfig->nRestTimeConc;
   smeConfig->csrConfig.nNumStaChanCombinedConc  = pConfig->nNumStaChanCombinedConc;
   smeConfig->csrConfig.nNumP2PChanCombinedConc  = pConfig->nNumP2PChanCombinedConc;

#endif
   smeConfig->csrConfig.Is11eSupportEnabled      = pConfig->b80211eIsEnabled;
   smeConfig->csrConfig.WMMSupportMode           = pConfig->WmmMode;

#if defined WLAN_FEATURE_VOWIFI
   smeConfig->rrmConfig.rrmEnabled = pConfig->fRrmEnable;
   smeConfig->rrmConfig.maxRandnInterval = pConfig->nRrmRandnIntvl;
#endif
   //Remaining config params not obtained from registry
   // On RF EVB beacon using channel 1.
#ifdef WLAN_FEATURE_11AC
   smeConfig->csrConfig.nVhtChannelWidth = pConfig->vhtChannelWidth;
   smeConfig->csrConfig.enableTxBF = pConfig->enableTxBF;
   smeConfig->csrConfig.txBFCsnValue = pConfig->txBFCsnValue;
   smeConfig->csrConfig.enable2x2 = pConfig->enable2x2;
   smeConfig->csrConfig.enableVhtFor24GHz = pConfig->enableVhtFor24GHzBand;
   smeConfig->csrConfig.enableMuBformee = pConfig->enableMuBformee;
   smeConfig->csrConfig.enableVhtpAid = pConfig->enableVhtpAid;
   smeConfig->csrConfig.enableVhtGid = pConfig->enableVhtGid;
#endif
   smeConfig->csrConfig.enableAmpduPs = pConfig->enableAmpduPs;
   smeConfig->csrConfig.enableHtSmps = pConfig->enableHtSmps;
   smeConfig->csrConfig.htSmps = pConfig->htSmps;
   smeConfig->csrConfig.AdHocChannel5G            = pConfig->AdHocChannel5G;
   smeConfig->csrConfig.AdHocChannel24            = pConfig->AdHocChannel24G;
   smeConfig->csrConfig.ProprietaryRatesEnabled   = 0;
   smeConfig->csrConfig.HeartbeatThresh50         = 40;
   smeConfig->csrConfig.bandCapability            = pConfig->nBandCapability;
   if (pConfig->nBandCapability == eCSR_BAND_24)
   {
       smeConfig->csrConfig.Is11hSupportEnabled       = 0;
   } else {
       smeConfig->csrConfig.Is11hSupportEnabled       = pConfig->Is11hSupportEnabled;
   }
   smeConfig->csrConfig.cbChoice                  = 0;
   smeConfig->csrConfig.bgScanInterval            = 0;
   smeConfig->csrConfig.eBand                     = pConfig->nBandCapability;
   smeConfig->csrConfig.nTxPowerCap = pConfig->nTxPowerCap;
   smeConfig->csrConfig.fEnableBypass11d          = pConfig->enableBypass11d;
   smeConfig->csrConfig.fEnableDFSChnlScan        = pConfig->enableDFSChnlScan;
#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
   smeConfig->csrConfig.nRoamPrefer5GHz           = pConfig->nRoamPrefer5GHz;
   smeConfig->csrConfig.nRoamIntraBand            = pConfig->nRoamIntraBand;
   smeConfig->csrConfig.nProbes                   = pConfig->nProbes;

   smeConfig->csrConfig.nRoamScanHomeAwayTime     = pConfig->nRoamScanHomeAwayTime;
#endif
   smeConfig->csrConfig.fFirstScanOnly2GChnl      = pConfig->enableFirstScan2GOnly;

   //FIXME 11d config is hardcoded
   if ( VOS_STA_SAP_MODE != hdd_get_conparam())
   {
      smeConfig->csrConfig.Csr11dinfo.Channels.numChannels = 0;

      /* if there is a requirement that HDD will control the default
       * channel list & country code (say from .ini file) we need to
       * add some logic here. Otherwise the default 11d info should
       * come from NV as per our current implementation */
   }
   else
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "AP country Code %s", pConfig->apCntryCode);

      if (memcmp(pConfig->apCntryCode, CFG_AP_COUNTRY_CODE_DEFAULT, 3) != 0)
         sme_setRegInfo(pHddCtx->hHal, pConfig->apCntryCode);
      sme_set11dinfo(pHddCtx->hHal, smeConfig);
   }

   if (!pConfig->enablePowersaveOffload)
   {
       hdd_set_power_save_config(pHddCtx, smeConfig);
   }
   else
   {
       hdd_set_power_save_offload_config(pHddCtx);
   }

   hdd_set_btc_config(pHddCtx);

#ifdef WLAN_FEATURE_VOWIFI_11R
   smeConfig->csrConfig.csr11rConfig.IsFTResourceReqSupported = pConfig->fFTResourceReqSupported;
#endif
#ifdef FEATURE_WLAN_LFR
   smeConfig->csrConfig.isFastRoamIniFeatureEnabled = pConfig->isFastRoamIniFeatureEnabled;
   smeConfig->csrConfig.MAWCEnabled = pConfig->MAWCEnabled;
#endif
#ifdef FEATURE_WLAN_ESE
   smeConfig->csrConfig.isEseIniFeatureEnabled = pConfig->isEseIniFeatureEnabled;
   if( pConfig->isEseIniFeatureEnabled )
   {
       pConfig->isFastTransitionEnabled = TRUE;
   }
#endif
#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
   smeConfig->csrConfig.isFastTransitionEnabled = pConfig->isFastTransitionEnabled;
   smeConfig->csrConfig.RoamRssiDiff = pConfig->RoamRssiDiff;
   smeConfig->csrConfig.nImmediateRoamRssiDiff = pConfig->nImmediateRoamRssiDiff;
   smeConfig->csrConfig.isWESModeEnabled = pConfig->isWESModeEnabled;
#endif
#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
   smeConfig->csrConfig.isRoamOffloadScanEnabled = pConfig->isRoamOffloadScanEnabled;
   smeConfig->csrConfig.bFastRoamInConIniFeatureEnabled = pConfig->bFastRoamInConIniFeatureEnabled;

   if (0 == smeConfig->csrConfig.isRoamOffloadScanEnabled)
   {
       /* Disable roaming in concurrency if roam scan offload is disabled */
       smeConfig->csrConfig.bFastRoamInConIniFeatureEnabled = 0;
   }
#endif
#ifdef WLAN_FEATURE_NEIGHBOR_ROAMING
   smeConfig->csrConfig.neighborRoamConfig.nNeighborReassocRssiThreshold = pConfig->nNeighborReassocRssiThreshold;
   smeConfig->csrConfig.neighborRoamConfig.nNeighborLookupRssiThreshold = pConfig->nNeighborLookupRssiThreshold;
   smeConfig->csrConfig.neighborRoamConfig.nOpportunisticThresholdDiff =
       pConfig->nOpportunisticThresholdDiff;
   smeConfig->csrConfig.neighborRoamConfig.nRoamRescanRssiDiff =
       pConfig->nRoamRescanRssiDiff;
   smeConfig->csrConfig.neighborRoamConfig.nNeighborScanMaxChanTime = pConfig->nNeighborScanMaxChanTime;
   smeConfig->csrConfig.neighborRoamConfig.nNeighborScanMinChanTime = pConfig->nNeighborScanMinChanTime;
   smeConfig->csrConfig.neighborRoamConfig.nNeighborScanTimerPeriod = pConfig->nNeighborScanPeriod;
   smeConfig->csrConfig.neighborRoamConfig.nMaxNeighborRetries = pConfig->nMaxNeighborReqTries;
   smeConfig->csrConfig.neighborRoamConfig.nNeighborResultsRefreshPeriod = pConfig->nNeighborResultsRefreshPeriod;
   smeConfig->csrConfig.neighborRoamConfig.nEmptyScanRefreshPeriod = pConfig->nEmptyScanRefreshPeriod;
   hdd_string_to_u8_array( pConfig->neighborScanChanList,
                                        smeConfig->csrConfig.neighborRoamConfig.neighborScanChanList.channelList,
                                        &smeConfig->csrConfig.neighborRoamConfig.neighborScanChanList.numChannels,
                                        WNI_CFG_VALID_CHANNEL_LIST_LEN );
   smeConfig->csrConfig.neighborRoamConfig.nRoamBmissFirstBcnt = pConfig->nRoamBmissFirstBcnt;
   smeConfig->csrConfig.neighborRoamConfig.nRoamBmissFinalBcnt = pConfig->nRoamBmissFinalBcnt;
   smeConfig->csrConfig.neighborRoamConfig.nRoamBeaconRssiWeight = pConfig->nRoamBeaconRssiWeight;
#endif

   smeConfig->csrConfig.addTSWhenACMIsOff = pConfig->AddTSWhenACMIsOff;
   smeConfig->csrConfig.fValidateList = pConfig->fValidateScanList;
   smeConfig->csrConfig.allowDFSChannelRoam = pConfig->allowDFSChannelRoam;

   //Enable/Disable MCC
   smeConfig->csrConfig.fEnableMCCMode = pConfig->enableMCC;
   smeConfig->csrConfig.fAllowMCCGODiffBI = pConfig->allowMCCGODiffBI;

   //Scan Results Aging Time out value
   smeConfig->csrConfig.scanCfgAgingTime = pConfig->scanAgingTimeout;

   smeConfig->csrConfig.enableTxLdpc = pConfig->enableTxLdpc;
#ifdef FEATURE_WLAN_MCC_TO_SCC_SWITCH
   smeConfig->csrConfig.cc_switch_mode = pConfig->WlanMccToSccSwitchMode;
#endif

   smeConfig->csrConfig.isAmsduSupportInAMPDU = pConfig->isAmsduSupportInAMPDU;
   smeConfig->csrConfig.nSelect5GHzMargin = pConfig->nSelect5GHzMargin;

   smeConfig->csrConfig.isCoalesingInIBSSAllowed =
                       pHddCtx->cfg_ini->isCoalesingInIBSSAllowed;
#ifdef QCA_HT_2040_COEX
   smeConfig->csrConfig.obssEnabled = pHddCtx->cfg_ini->ht2040CoexEnabled;
#endif

   /* update SSR config */
   sme_UpdateEnableSSR((tHalHandle)(pHddCtx->hHal), pHddCtx->cfg_ini->enableSSR);
   /* Update the Directed scan offload setting */
   smeConfig->fScanOffload =  pHddCtx->cfg_ini->fScanOffload;

   /* Update the p2p listen offload setting */
   smeConfig->fP2pListenOffload =  pHddCtx->cfg_ini->fP2pListenOffload;
   smeConfig->csrConfig.scanBandPreference =
                              pHddCtx->cfg_ini->acsScanBandPreference;

#ifdef FEATURE_WLAN_SCAN_PNO
   /* Update PNO offload status */
   smeConfig->pnoOffload = pHddCtx->cfg_ini->PnoOffload;
#endif

   /* Update maximum interfaces information */
   smeConfig->max_intf_count = pHddCtx->max_intf_count;

   smeConfig->fEnableDebugLog = pHddCtx->cfg_ini->gEnableDebugLog;

   smeConfig->enable5gEBT = pHddCtx->cfg_ini->enable5gEBT;

   smeConfig->enableSelfRecovery = pHddCtx->cfg_ini->enableSelfRecovery;
#ifdef WLAN_FEATURE_ROAM_OFFLOAD
   smeConfig->csrConfig.isRoamOffloadEnabled =
                        pHddCtx->cfg_ini->isRoamOffloadEnabled;
#endif

   halStatus = sme_UpdateConfig( pHddCtx->hHal, smeConfig);
   if ( !HAL_STATUS_SUCCESS( halStatus ) )
   {
      status = VOS_STATUS_E_FAILURE;
      hddLog(LOGE, "sme_UpdateConfig() return failure %d", halStatus);
   }

   vos_mem_free(smeConfig);
   return status;
}


/**---------------------------------------------------------------------------

  \brief hdd_execute_config_command() -

   This function executes an arbitrary configuration set command

  \param - pHddCtx - Pointer to the HDD Adapter.
  \parmm - command - a configuration command of the form:
                     <name>=<value>

  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/

static VOS_STATUS hdd_execute_config_command(REG_TABLE_ENTRY *reg_table,
                                   unsigned long tableSize, v_U8_t *ini_struct,
                                   hdd_context_t *pHddCtx, char *command)
{
   REG_TABLE_ENTRY *pRegEntry;
   char *clone;
   char *pCmd;
   void *pField;
   char *name;
   char *value_str;
   v_U32_t value;
   v_S31_t svalue;
   size_t len_value_str;
   unsigned int idx;
   unsigned int i;
   VOS_STATUS vstatus;
   int rv;

   // assume failure until proven otherwise
   vstatus = VOS_STATUS_E_FAILURE;

   // clone the command so that we can manipulate it
   clone = kstrdup(command, GFP_ATOMIC);
   if (NULL == clone)
   {
      hddLog(LOGE, "%s: memory allocation failure, unable to process [%s]",
             __func__, command);
      return vstatus;
   }

   // 'clone' will point to the beginning of the string so it can be freed
   // 'pCmd' will be used to walk/parse the command
   pCmd = clone;

   // get rid of leading/trailing whitespace
   pCmd = i_trim(pCmd);
   if ('\0' == *pCmd)
   {
      // only whitespace
      hddLog(LOGE, "%s: invalid command, only whitespace:[%s]",
             __func__, command);
      goto done;
   }

   // parse the <name> = <value>
   name = pCmd;
   while (('=' != *pCmd) && ('\0' != *pCmd))
   {
      pCmd++;
   }
   if ('\0' == *pCmd)
   {
      // did not find '='
      hddLog(LOGE, "%s: invalid command, no '=':[%s]",
             __func__, command);
      goto done;
   }

   // replace '=' with NUL to terminate the <name>
   *pCmd++ = '\0';
   name = i_trim(name);
   if ('\0' == *name)
   {
      // did not find a name
      hddLog(LOGE, "%s: invalid command, no <name>:[%s]",
             __func__, command);
      goto done;
   }

   value_str = i_trim(pCmd);
   if ('\0' == *value_str)
   {
      // did not find a value
      hddLog(LOGE, "%s: invalid command, no <value>:[%s]",
             __func__, command);
      goto done;
   }

   // lookup the configuration item
   for (idx = 0; idx < tableSize; idx++)
   {
      if (0 == strcmp(name, reg_table[idx].RegName))
      {
         // found a match
         break;
      }
   }
   if (tableSize == idx)
   {
      // did not match the name
      hddLog(LOGE, "%s: invalid command, unknown configuration item:[%s]",
             __func__, command);
      goto done;
   }

   pRegEntry = &reg_table[idx];
   if (!(pRegEntry->Flags & VAR_FLAGS_DYNAMIC_CFG))
   {
      // does not support dynamic configuration
      hddLog(LOGE, "%s: Global_Registry_Table.%s does not support "
             "dynamic configuration", __func__, name);
      vstatus = VOS_STATUS_E_PERM;
      goto done;
   }

   pField = ini_struct + pRegEntry->VarOffset;

   switch (pRegEntry->RegType)
   {
   case WLAN_PARAM_Integer:
      rv = kstrtou32(value_str, 10, &value);
      if (rv < 0)
          goto done;
      if (value < pRegEntry->VarMin)
      {
         // out of range
         hddLog(LOGE, "%s: invalid command, value %u < min value %lu",
                __func__, value, pRegEntry->VarMin);
         goto done;
      }
      if (value > pRegEntry->VarMax)
      {
         // out of range
         hddLog(LOGE, "%s: invalid command, value %u > max value %lu",
                __func__, value, pRegEntry->VarMax);
         goto done;
      }
      memcpy(pField, &value, pRegEntry->VarSize);
      break;

   case WLAN_PARAM_HexInteger:
      rv = kstrtou32(value_str, 16, &value);
      if (rv < 0)
         goto done;
      if (value < pRegEntry->VarMin)
      {
         // out of range
         hddLog(LOGE, "%s: invalid command, value %x < min value %lx",
                __func__, value, pRegEntry->VarMin);
         goto done;
      }
      if (value > pRegEntry->VarMax)
      {
         // out of range
         hddLog(LOGE, "%s: invalid command, value %x > max value %lx",
                __func__, value, pRegEntry->VarMax);
         goto done;
      }
      memcpy(pField, &value, pRegEntry->VarSize);
      break;

   case WLAN_PARAM_SignedInteger:
      rv = kstrtos32(value_str, 10, &svalue);
      if (rv < 0)
         goto done;
      if (svalue < (v_S31_t)pRegEntry->VarMin)
      {
         // out of range
         hddLog(LOGE, "%s: invalid command, value %d < min value %d",
                __func__, svalue, (int)pRegEntry->VarMin);
         goto done;
      }
      if (svalue > (v_S31_t)pRegEntry->VarMax)
      {
         // out of range
         hddLog(LOGE, "%s: invalid command, value %d > max value %d",
                __func__, svalue, (int)pRegEntry->VarMax);
         goto done;
      }
      memcpy(pField, &svalue, pRegEntry->VarSize);
      break;

   case WLAN_PARAM_String:
      len_value_str = strlen(value_str);
      if (len_value_str > (pRegEntry->VarSize - 1))
      {
         // too big
         hddLog(LOGE,
                "%s: invalid command, string [%s] length "
                "%zu exceeds maximum length %u",
                __func__, value_str,
                len_value_str, (pRegEntry->VarSize - 1));
         goto done;
      }
      // copy string plus NUL
      memcpy(pField, value_str, (len_value_str + 1));
      break;

   case WLAN_PARAM_MacAddr:
      len_value_str = strlen(value_str);
      if (len_value_str != (VOS_MAC_ADDR_SIZE * 2))
      {
         // out of range
         hddLog(LOGE,
                "%s: invalid command, MAC address [%s] length "
                "%zu is not expected length %u",
                __func__, value_str,
                len_value_str, (VOS_MAC_ADDR_SIZE * 2));
         goto done;
      }
      //parse the string and store it in the byte array
      for (i = 0; i < VOS_MAC_ADDR_SIZE; i++)
      {
         ((char*)pField)[i] = (char)
            ((parseHexDigit(value_str[(i * 2)]) * 16) +
             parseHexDigit(value_str[(i * 2) + 1]));
      }
      break;

   default:
      goto done;
   }

   // if we get here, we had a successful modification
   vstatus = VOS_STATUS_SUCCESS;

   // config table has been modified, is there a notifier?
   if (NULL != pRegEntry->pfnDynamicNotify)
   {
      (pRegEntry->pfnDynamicNotify)(pHddCtx, pRegEntry->NotifyId);
   }

   // note that this item was explicitly configured
   if (idx < MAX_CFG_INI_ITEMS)
   {
      set_bit(idx, (void *)&pHddCtx->cfg_ini->bExplicitCfg);
   }
 done:
   kfree(clone);
   return vstatus;
}

VOS_STATUS hdd_execute_global_config_command(hdd_context_t *pHddCtx,
                                                                 char *command)
{
    return hdd_execute_config_command(g_registry_table,
                                      ARRAY_SIZE(g_registry_table),
                                      (v_U8_t *) pHddCtx->cfg_ini,
                                      pHddCtx, command);
}

#ifdef WLAN_FEATURE_MBSSID
VOS_STATUS hdd_execute_sap_dyn_config_command(hdd_adapter_t *pAdapter,
                                                                 char *command)
{
    return hdd_execute_config_command(mbssid_sap_dyn_ini_reg_table,
                                      ARRAY_SIZE(mbssid_sap_dyn_ini_reg_table),
                                      (v_U8_t *) &pAdapter->sap_dyn_ini_cfg,
                                      WLAN_HDD_GET_CTX(pAdapter), command);
}
#endif
/**---------------------------------------------------------------------------

  \brief hdd_is_okc_mode_enabled() -

   This function returns whether OKC mode is enabled or not

  \param - pHddCtx - Pointer to the HDD Adapter.

  \return - 1 for enabled, zero for disabled

  --------------------------------------------------------------------------*/

tANI_BOOLEAN hdd_is_okc_mode_enabled(hdd_context_t *pHddCtx)
{
    if (NULL == pHddCtx)
    {
        hddLog(VOS_TRACE_LEVEL_FATAL, "%s: pHddCtx is NULL", __func__);
        return -EINVAL;
    }

#ifdef FEATURE_WLAN_OKC
    return pHddCtx->cfg_ini->isOkcIniFeatureEnabled;
#else
    return eANI_BOOLEAN_FALSE;
#endif
}
