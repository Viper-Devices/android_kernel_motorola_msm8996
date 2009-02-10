/*
 * Scanning implementation
 *
 * Copyright 2003, Jouni Malinen <jkmaline@cc.hut.fi>
 * Copyright 2004, Instant802 Networks, Inc.
 * Copyright 2005, Devicescape Software, Inc.
 * Copyright 2006-2007	Jiri Benc <jbenc@suse.cz>
 * Copyright 2007, Michael Wu <flamingice@sourmilk.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* TODO:
 * figure out how to avoid that the "current BSS" expires
 * use cfg80211's BSS handling
 */

#include <linux/wireless.h>
#include <linux/if_arp.h>
#include <linux/rtnetlink.h>
#include <net/mac80211.h>
#include <net/iw_handler.h>

#include "ieee80211_i.h"
#include "mesh.h"

#define IEEE80211_PROBE_DELAY (HZ / 33)
#define IEEE80211_CHANNEL_TIME (HZ / 33)
#define IEEE80211_PASSIVE_CHANNEL_TIME (HZ / 5)

void ieee80211_rx_bss_list_init(struct ieee80211_local *local)
{
	spin_lock_init(&local->bss_lock);
	INIT_LIST_HEAD(&local->bss_list);
}

void ieee80211_rx_bss_list_deinit(struct ieee80211_local *local)
{
	struct ieee80211_bss *bss, *tmp;

	list_for_each_entry_safe(bss, tmp, &local->bss_list, list)
		ieee80211_rx_bss_put(local, bss);
}

struct ieee80211_bss *
ieee80211_rx_bss_get(struct ieee80211_local *local, u8 *bssid, int freq,
		     u8 *ssid, u8 ssid_len)
{
	struct ieee80211_bss *bss;

	spin_lock_bh(&local->bss_lock);
	bss = local->bss_hash[STA_HASH(bssid)];
	while (bss) {
		if (!bss_mesh_cfg(bss) &&
		    !memcmp(bss->bssid, bssid, ETH_ALEN) &&
		    bss->freq == freq &&
		    bss->ssid_len == ssid_len &&
		    (ssid_len == 0 || !memcmp(bss->ssid, ssid, ssid_len))) {
			atomic_inc(&bss->users);
			break;
		}
		bss = bss->hnext;
	}
	spin_unlock_bh(&local->bss_lock);
	return bss;
}

/* Caller must hold local->bss_lock */
static void __ieee80211_rx_bss_hash_add(struct ieee80211_local *local,
					struct ieee80211_bss *bss)
{
	u8 hash_idx;

	if (bss_mesh_cfg(bss))
		hash_idx = mesh_id_hash(bss_mesh_id(bss),
					bss_mesh_id_len(bss));
	else
		hash_idx = STA_HASH(bss->bssid);

	bss->hnext = local->bss_hash[hash_idx];
	local->bss_hash[hash_idx] = bss;
}

/* Caller must hold local->bss_lock */
static void __ieee80211_rx_bss_hash_del(struct ieee80211_local *local,
					struct ieee80211_bss *bss)
{
	struct ieee80211_bss *b, *prev = NULL;
	b = local->bss_hash[STA_HASH(bss->bssid)];
	while (b) {
		if (b == bss) {
			if (!prev)
				local->bss_hash[STA_HASH(bss->bssid)] =
					bss->hnext;
			else
				prev->hnext = bss->hnext;
			break;
		}
		prev = b;
		b = b->hnext;
	}
}

static struct ieee80211_bss *
ieee80211_rx_bss_add(struct ieee80211_local *local, u8 *bssid, int freq,
		     u8 *ssid, u8 ssid_len)
{
	struct ieee80211_bss *bss;

	bss = kzalloc(sizeof(*bss), GFP_ATOMIC);
	if (!bss)
		return NULL;
	atomic_set(&bss->users, 2);
	memcpy(bss->bssid, bssid, ETH_ALEN);
	bss->freq = freq;
	if (ssid && ssid_len <= IEEE80211_MAX_SSID_LEN) {
		memcpy(bss->ssid, ssid, ssid_len);
		bss->ssid_len = ssid_len;
	}

	spin_lock_bh(&local->bss_lock);
	/* TODO: order by RSSI? */
	list_add_tail(&bss->list, &local->bss_list);
	__ieee80211_rx_bss_hash_add(local, bss);
	spin_unlock_bh(&local->bss_lock);
	return bss;
}

#ifdef CONFIG_MAC80211_MESH
static struct ieee80211_bss *
ieee80211_rx_mesh_bss_get(struct ieee80211_local *local, u8 *mesh_id, int mesh_id_len,
			  u8 *mesh_cfg, int freq)
{
	struct ieee80211_bss *bss;

	spin_lock_bh(&local->bss_lock);
	bss = local->bss_hash[mesh_id_hash(mesh_id, mesh_id_len)];
	while (bss) {
		if (bss_mesh_cfg(bss) &&
		    !memcmp(bss_mesh_cfg(bss), mesh_cfg, MESH_CFG_CMP_LEN) &&
		    bss->freq == freq &&
		    mesh_id_len == bss->mesh_id_len &&
		    (mesh_id_len == 0 || !memcmp(bss->mesh_id, mesh_id,
						 mesh_id_len))) {
			atomic_inc(&bss->users);
			break;
		}
		bss = bss->hnext;
	}
	spin_unlock_bh(&local->bss_lock);
	return bss;
}

static struct ieee80211_bss *
ieee80211_rx_mesh_bss_add(struct ieee80211_local *local, u8 *mesh_id, int mesh_id_len,
			  u8 *mesh_cfg, int mesh_config_len, int freq)
{
	struct ieee80211_bss *bss;

	if (mesh_config_len != IEEE80211_MESH_CONFIG_LEN)
		return NULL;

	bss = kzalloc(sizeof(*bss), GFP_ATOMIC);
	if (!bss)
		return NULL;

	bss->mesh_cfg = kmalloc(MESH_CFG_CMP_LEN, GFP_ATOMIC);
	if (!bss->mesh_cfg) {
		kfree(bss);
		return NULL;
	}

	if (mesh_id_len && mesh_id_len <= IEEE80211_MAX_MESH_ID_LEN) {
		bss->mesh_id = kmalloc(mesh_id_len, GFP_ATOMIC);
		if (!bss->mesh_id) {
			kfree(bss->mesh_cfg);
			kfree(bss);
			return NULL;
		}
		memcpy(bss->mesh_id, mesh_id, mesh_id_len);
	}

	atomic_set(&bss->users, 2);
	memcpy(bss->mesh_cfg, mesh_cfg, MESH_CFG_CMP_LEN);
	bss->mesh_id_len = mesh_id_len;
	bss->freq = freq;
	spin_lock_bh(&local->bss_lock);
	/* TODO: order by RSSI? */
	list_add_tail(&bss->list, &local->bss_list);
	__ieee80211_rx_bss_hash_add(local, bss);
	spin_unlock_bh(&local->bss_lock);
	return bss;
}
#endif

static void ieee80211_rx_bss_free(struct ieee80211_bss *bss)
{
	kfree(bss->ies);
	kfree(bss_mesh_id(bss));
	kfree(bss_mesh_cfg(bss));
	kfree(bss);
}

void ieee80211_rx_bss_put(struct ieee80211_local *local,
			  struct ieee80211_bss *bss)
{
	local_bh_disable();
	if (!atomic_dec_and_lock(&bss->users, &local->bss_lock)) {
		local_bh_enable();
		return;
	}

	__ieee80211_rx_bss_hash_del(local, bss);
	list_del(&bss->list);
	spin_unlock_bh(&local->bss_lock);
	ieee80211_rx_bss_free(bss);
}

struct ieee80211_bss *
ieee80211_bss_info_update(struct ieee80211_local *local,
			  struct ieee80211_rx_status *rx_status,
			  struct ieee80211_mgmt *mgmt,
			  size_t len,
			  struct ieee802_11_elems *elems,
			  struct ieee80211_channel *channel,
			  bool beacon)
{
	struct ieee80211_bss *bss;
	int clen, freq = channel->center_freq;
	enum cfg80211_signal_type sigtype = CFG80211_SIGNAL_TYPE_NONE;
	s32 signal = 0;

	if (local->hw.flags & IEEE80211_HW_SIGNAL_DBM) {
		sigtype = CFG80211_SIGNAL_TYPE_MBM;
		signal = rx_status->signal * 100;
	} else if (local->hw.flags & IEEE80211_HW_SIGNAL_UNSPEC) {
		sigtype = CFG80211_SIGNAL_TYPE_UNSPEC;
		signal = (rx_status->signal * 100) / local->hw.max_signal;
	}

	cfg80211_put_bss(
		cfg80211_inform_bss_frame(local->hw.wiphy, channel,
					  mgmt, len, signal, sigtype,
					  GFP_ATOMIC));

#ifdef CONFIG_MAC80211_MESH
	if (elems->mesh_config)
		bss = ieee80211_rx_mesh_bss_get(local, elems->mesh_id,
				elems->mesh_id_len, elems->mesh_config, freq);
	else
#endif
		bss = ieee80211_rx_bss_get(local, mgmt->bssid, freq,
					   elems->ssid, elems->ssid_len);
	if (!bss) {
#ifdef CONFIG_MAC80211_MESH
		if (elems->mesh_config)
			bss = ieee80211_rx_mesh_bss_add(local, elems->mesh_id,
				elems->mesh_id_len, elems->mesh_config,
				elems->mesh_config_len, freq);
		else
#endif
			bss = ieee80211_rx_bss_add(local, mgmt->bssid, freq,
						  elems->ssid, elems->ssid_len);
		if (!bss)
			return NULL;
	} else {
#if 0
		/* TODO: order by RSSI? */
		spin_lock_bh(&local->bss_lock);
		list_move_tail(&bss->list, &local->bss_list);
		spin_unlock_bh(&local->bss_lock);
#endif
	}

	/* save the ERP value so that it is available at association time */
	if (elems->erp_info && elems->erp_info_len >= 1) {
		bss->erp_value = elems->erp_info[0];
		bss->has_erp_value = 1;
	}

	bss->beacon_int = le16_to_cpu(mgmt->u.beacon.beacon_int);
	bss->capability = le16_to_cpu(mgmt->u.beacon.capab_info);

	if (elems->tim) {
		struct ieee80211_tim_ie *tim_ie =
			(struct ieee80211_tim_ie *)elems->tim;
		bss->dtim_period = tim_ie->dtim_period;
	}

	/* set default value for buggy APs */
	if (!elems->tim || bss->dtim_period == 0)
		bss->dtim_period = 1;

	bss->supp_rates_len = 0;
	if (elems->supp_rates) {
		clen = IEEE80211_MAX_SUPP_RATES - bss->supp_rates_len;
		if (clen > elems->supp_rates_len)
			clen = elems->supp_rates_len;
		memcpy(&bss->supp_rates[bss->supp_rates_len], elems->supp_rates,
		       clen);
		bss->supp_rates_len += clen;
	}
	if (elems->ext_supp_rates) {
		clen = IEEE80211_MAX_SUPP_RATES - bss->supp_rates_len;
		if (clen > elems->ext_supp_rates_len)
			clen = elems->ext_supp_rates_len;
		memcpy(&bss->supp_rates[bss->supp_rates_len],
		       elems->ext_supp_rates, clen);
		bss->supp_rates_len += clen;
	}

	bss->band = rx_status->band;

	bss->timestamp = le64_to_cpu(mgmt->u.beacon.timestamp);
	bss->last_update = jiffies;
	bss->signal = rx_status->signal;
	bss->noise = rx_status->noise;
	bss->qual = rx_status->qual;
	bss->wmm_used = elems->wmm_param || elems->wmm_info;

	if (!beacon)
		bss->last_probe_resp = jiffies;

	/*
	 * For probe responses, or if we don't have any information yet,
	 * use the IEs from the beacon.
	 */
	if (!bss->ies || !beacon) {
		if (bss->ies == NULL || bss->ies_len < elems->total_len) {
			kfree(bss->ies);
			bss->ies = kmalloc(elems->total_len, GFP_ATOMIC);
		}
		if (bss->ies) {
			memcpy(bss->ies, elems->ie_start, elems->total_len);
			bss->ies_len = elems->total_len;
		} else
			bss->ies_len = 0;
	}

	return bss;
}

void ieee80211_rx_bss_remove(struct ieee80211_sub_if_data *sdata, u8 *bssid,
			     int freq, u8 *ssid, u8 ssid_len)
{
	struct ieee80211_bss *bss;
	struct ieee80211_local *local = sdata->local;

	bss = ieee80211_rx_bss_get(local, bssid, freq, ssid, ssid_len);
	if (bss) {
		atomic_dec(&bss->users);
		ieee80211_rx_bss_put(local, bss);
	}
}

ieee80211_rx_result
ieee80211_scan_rx(struct ieee80211_sub_if_data *sdata, struct sk_buff *skb,
		  struct ieee80211_rx_status *rx_status)
{
	struct ieee80211_mgmt *mgmt;
	struct ieee80211_bss *bss;
	u8 *elements;
	struct ieee80211_channel *channel;
	size_t baselen;
	int freq;
	__le16 fc;
	bool presp, beacon = false;
	struct ieee802_11_elems elems;

	if (skb->len < 2)
		return RX_DROP_UNUSABLE;

	mgmt = (struct ieee80211_mgmt *) skb->data;
	fc = mgmt->frame_control;

	if (ieee80211_is_ctl(fc))
		return RX_CONTINUE;

	if (skb->len < 24)
		return RX_DROP_MONITOR;

	presp = ieee80211_is_probe_resp(fc);
	if (presp) {
		/* ignore ProbeResp to foreign address */
		if (memcmp(mgmt->da, sdata->dev->dev_addr, ETH_ALEN))
			return RX_DROP_MONITOR;

		presp = true;
		elements = mgmt->u.probe_resp.variable;
		baselen = offsetof(struct ieee80211_mgmt, u.probe_resp.variable);
	} else {
		beacon = ieee80211_is_beacon(fc);
		baselen = offsetof(struct ieee80211_mgmt, u.beacon.variable);
		elements = mgmt->u.beacon.variable;
	}

	if (!presp && !beacon)
		return RX_CONTINUE;

	if (baselen > skb->len)
		return RX_DROP_MONITOR;

	ieee802_11_parse_elems(elements, skb->len - baselen, &elems);

	if (elems.ds_params && elems.ds_params_len == 1)
		freq = ieee80211_channel_to_frequency(elems.ds_params[0]);
	else
		freq = rx_status->freq;

	channel = ieee80211_get_channel(sdata->local->hw.wiphy, freq);

	if (!channel || channel->flags & IEEE80211_CHAN_DISABLED)
		return RX_DROP_MONITOR;

	bss = ieee80211_bss_info_update(sdata->local, rx_status,
					mgmt, skb->len, &elems,
					channel, beacon);
	if (bss)
		ieee80211_rx_bss_put(sdata->local, bss);

	dev_kfree_skb(skb);
	return RX_QUEUED;
}

void ieee80211_send_nullfunc(struct ieee80211_local *local,
				    struct ieee80211_sub_if_data *sdata,
				    int powersave)
{
	struct sk_buff *skb;
	struct ieee80211_hdr *nullfunc;
	__le16 fc;

	skb = dev_alloc_skb(local->hw.extra_tx_headroom + 24);
	if (!skb) {
		printk(KERN_DEBUG "%s: failed to allocate buffer for nullfunc "
		       "frame\n", sdata->dev->name);
		return;
	}
	skb_reserve(skb, local->hw.extra_tx_headroom);

	nullfunc = (struct ieee80211_hdr *) skb_put(skb, 24);
	memset(nullfunc, 0, 24);
	fc = cpu_to_le16(IEEE80211_FTYPE_DATA | IEEE80211_STYPE_NULLFUNC |
			 IEEE80211_FCTL_TODS);
	if (powersave)
		fc |= cpu_to_le16(IEEE80211_FCTL_PM);
	nullfunc->frame_control = fc;
	memcpy(nullfunc->addr1, sdata->u.sta.bssid, ETH_ALEN);
	memcpy(nullfunc->addr2, sdata->dev->dev_addr, ETH_ALEN);
	memcpy(nullfunc->addr3, sdata->u.sta.bssid, ETH_ALEN);

	ieee80211_tx_skb(sdata, skb, 0);
}

void ieee80211_scan_completed(struct ieee80211_hw *hw, bool aborted)
{
	struct ieee80211_local *local = hw_to_local(hw);
	struct ieee80211_sub_if_data *sdata;

	if (WARN_ON(!local->hw_scanning && !local->sw_scanning))
		return;

	if (WARN_ON(!local->scan_req))
		return;

	if (local->scan_req != &local->int_scan_req)
		cfg80211_scan_done(local->scan_req, aborted);
	local->scan_req = NULL;

	local->last_scan_completed = jiffies;

	if (local->hw_scanning) {
		local->hw_scanning = false;
		/*
		 * Somebody might have requested channel change during scan
		 * that we won't have acted upon, try now. ieee80211_hw_config
		 * will set the flag based on actual changes.
		 */
		ieee80211_hw_config(local, 0);
		goto done;
	}

	local->sw_scanning = false;
	ieee80211_hw_config(local, IEEE80211_CONF_CHANGE_CHANNEL);

	netif_tx_lock_bh(local->mdev);
	netif_addr_lock(local->mdev);
	local->filter_flags &= ~FIF_BCN_PRBRESP_PROMISC;
	local->ops->configure_filter(local_to_hw(local),
				     FIF_BCN_PRBRESP_PROMISC,
				     &local->filter_flags,
				     local->mdev->mc_count,
				     local->mdev->mc_list);

	netif_addr_unlock(local->mdev);
	netif_tx_unlock_bh(local->mdev);

	mutex_lock(&local->iflist_mtx);
	list_for_each_entry(sdata, &local->interfaces, list) {
		if (!netif_running(sdata->dev))
			continue;

		/* Tell AP we're back */
		if (sdata->vif.type == NL80211_IFTYPE_STATION) {
			if (sdata->u.sta.flags & IEEE80211_STA_ASSOCIATED) {
				ieee80211_send_nullfunc(local, sdata, 0);
				netif_tx_wake_all_queues(sdata->dev);
			}
		} else
			netif_tx_wake_all_queues(sdata->dev);

		/* re-enable beaconing */
		if (sdata->vif.type == NL80211_IFTYPE_AP ||
		    sdata->vif.type == NL80211_IFTYPE_ADHOC ||
		    sdata->vif.type == NL80211_IFTYPE_MESH_POINT)
			ieee80211_if_config(sdata,
					    IEEE80211_IFCC_BEACON_ENABLED);
	}
	mutex_unlock(&local->iflist_mtx);

 done:
	ieee80211_mlme_notify_scan_completed(local);
	ieee80211_mesh_notify_scan_completed(local);
}
EXPORT_SYMBOL(ieee80211_scan_completed);

void ieee80211_scan_work(struct work_struct *work)
{
	struct ieee80211_local *local =
		container_of(work, struct ieee80211_local, scan_work.work);
	struct ieee80211_sub_if_data *sdata = local->scan_sdata;
	struct ieee80211_channel *chan;
	int skip, i;
	unsigned long next_delay = 0;

	/*
	 * Avoid re-scheduling when the sdata is going away.
	 */
	if (!netif_running(sdata->dev))
		return;

	switch (local->scan_state) {
	case SCAN_SET_CHANNEL:
		/* if no more bands/channels left, complete scan */
		if (local->scan_channel_idx >= local->scan_req->n_channels) {
			ieee80211_scan_completed(local_to_hw(local), false);
			return;
		}
		skip = 0;
		chan = local->scan_req->channels[local->scan_channel_idx];

		if (chan->flags & IEEE80211_CHAN_DISABLED ||
		    (sdata->vif.type == NL80211_IFTYPE_ADHOC &&
		     chan->flags & IEEE80211_CHAN_NO_IBSS))
			skip = 1;

		if (!skip) {
			local->scan_channel = chan;
			if (ieee80211_hw_config(local,
						IEEE80211_CONF_CHANGE_CHANNEL))
				skip = 1;
		}

		/* advance state machine to next channel/band */
		local->scan_channel_idx++;

		if (skip)
			break;

		next_delay = IEEE80211_PROBE_DELAY +
			     usecs_to_jiffies(local->hw.channel_change_time);
		local->scan_state = SCAN_SEND_PROBE;
		break;
	case SCAN_SEND_PROBE:
		next_delay = IEEE80211_PASSIVE_CHANNEL_TIME;
		local->scan_state = SCAN_SET_CHANNEL;

		if (local->scan_channel->flags & IEEE80211_CHAN_PASSIVE_SCAN ||
		    !local->scan_req->n_ssids)
			break;
		for (i = 0; i < local->scan_req->n_ssids; i++)
			ieee80211_send_probe_req(
				sdata, NULL,
				local->scan_req->ssids[i].ssid,
				local->scan_req->ssids[i].ssid_len);
		next_delay = IEEE80211_CHANNEL_TIME;
		break;
	}

	queue_delayed_work(local->hw.workqueue, &local->scan_work,
			   next_delay);
}


int ieee80211_start_scan(struct ieee80211_sub_if_data *scan_sdata,
			 struct cfg80211_scan_request *req)
{
	struct ieee80211_local *local = scan_sdata->local;
	struct ieee80211_sub_if_data *sdata;

	if (!req)
		return -EINVAL;

	if (local->scan_req && local->scan_req != req)
		return -EBUSY;

	local->scan_req = req;

	/* MLME-SCAN.request (page 118)  page 144 (11.1.3.1)
	 * BSSType: INFRASTRUCTURE, INDEPENDENT, ANY_BSS
	 * BSSID: MACAddress
	 * SSID
	 * ScanType: ACTIVE, PASSIVE
	 * ProbeDelay: delay (in microseconds) to be used prior to transmitting
	 *    a Probe frame during active scanning
	 * ChannelList
	 * MinChannelTime (>= ProbeDelay), in TU
	 * MaxChannelTime: (>= MinChannelTime), in TU
	 */

	 /* MLME-SCAN.confirm
	  * BSSDescriptionSet
	  * ResultCode: SUCCESS, INVALID_PARAMETERS
	 */

	if (local->sw_scanning || local->hw_scanning) {
		if (local->scan_sdata == scan_sdata)
			return 0;
		return -EBUSY;
	}

	if (local->ops->hw_scan) {
		int rc;

		local->hw_scanning = true;
		rc = local->ops->hw_scan(local_to_hw(local), req);
		if (rc) {
			local->hw_scanning = false;
			return rc;
		}
		local->scan_sdata = scan_sdata;
		return 0;
	}

	local->sw_scanning = true;

	mutex_lock(&local->iflist_mtx);
	list_for_each_entry(sdata, &local->interfaces, list) {
		if (!netif_running(sdata->dev))
			continue;

		/* disable beaconing */
		if (sdata->vif.type == NL80211_IFTYPE_AP ||
		    sdata->vif.type == NL80211_IFTYPE_ADHOC ||
		    sdata->vif.type == NL80211_IFTYPE_MESH_POINT)
			ieee80211_if_config(sdata,
					    IEEE80211_IFCC_BEACON_ENABLED);

		if (sdata->vif.type == NL80211_IFTYPE_STATION) {
			if (sdata->u.sta.flags & IEEE80211_STA_ASSOCIATED) {
				netif_tx_stop_all_queues(sdata->dev);
				ieee80211_send_nullfunc(local, sdata, 1);
			}
		} else
			netif_tx_stop_all_queues(sdata->dev);
	}
	mutex_unlock(&local->iflist_mtx);

	local->scan_state = SCAN_SET_CHANNEL;
	local->scan_channel_idx = 0;
	local->scan_sdata = scan_sdata;
	local->scan_req = req;

	netif_addr_lock_bh(local->mdev);
	local->filter_flags |= FIF_BCN_PRBRESP_PROMISC;
	local->ops->configure_filter(local_to_hw(local),
				     FIF_BCN_PRBRESP_PROMISC,
				     &local->filter_flags,
				     local->mdev->mc_count,
				     local->mdev->mc_list);
	netif_addr_unlock_bh(local->mdev);

	/* TODO: start scan as soon as all nullfunc frames are ACKed */
	queue_delayed_work(local->hw.workqueue, &local->scan_work,
			   IEEE80211_CHANNEL_TIME);

	return 0;
}


int ieee80211_request_scan(struct ieee80211_sub_if_data *sdata,
			   struct cfg80211_scan_request *req)
{
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_if_sta *ifsta;

	if (!req)
		return -EINVAL;

	if (local->scan_req && local->scan_req != req)
		return -EBUSY;

	local->scan_req = req;

	if (sdata->vif.type != NL80211_IFTYPE_STATION)
		return ieee80211_start_scan(sdata, req);

	/*
	 * STA has a state machine that might need to defer scanning
	 * while it's trying to associate/authenticate, therefore we
	 * queue it up to the state machine in that case.
	 */

	if (local->sw_scanning || local->hw_scanning) {
		if (local->scan_sdata == sdata)
			return 0;
		return -EBUSY;
	}

	ifsta = &sdata->u.sta;
	set_bit(IEEE80211_STA_REQ_SCAN, &ifsta->request);
	queue_work(local->hw.workqueue, &ifsta->work);

	return 0;
}
