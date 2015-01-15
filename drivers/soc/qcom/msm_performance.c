/*
 * Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/moduleparam.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>

#include <trace/events/power.h>

static struct mutex managed_cpus_lock;

/* Maximum number to clusters that this module will manage*/
static unsigned int num_clusters;
struct cluster {
	cpumask_var_t cpus;
	/* Number of CPUs to maintain online */
	int max_cpu_request;
	/* To track CPUs that the module decides to offline */
	cpumask_var_t offlined_cpus;
};
static struct cluster **managed_clusters;
static bool clusters_inited;

/* Work to evaluate the onlining/offlining CPUs */
struct delayed_work evaluate_hotplug_work;

/* To handle cpufreq min/max request */
struct cpu_status {
	unsigned int min;
	unsigned int max;
};
static DEFINE_PER_CPU(struct cpu_status, cpu_stats);

static unsigned int num_online_managed(struct cpumask *mask);
static int init_cluster_control(void);
static int rm_high_pwr_cost_cpus(struct cluster *cl);

static DEFINE_PER_CPU(unsigned int, cpu_power_cost);

static int set_num_clusters(const char *buf, const struct kernel_param *kp)
{
	unsigned int val;

	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;
	if (num_clusters)
		return -EINVAL;

	num_clusters = val;

	if (init_cluster_control()) {
		num_clusters = 0;
		return -ENOMEM;
	}

	return 0;
}

static int get_num_clusters(char *buf, const struct kernel_param *kp)
{
	return snprintf(buf, PAGE_SIZE, "%u", num_clusters);
}

static const struct kernel_param_ops param_ops_num_clusters = {
	.set = set_num_clusters,
	.get = get_num_clusters,
};
device_param_cb(num_clusters, &param_ops_num_clusters, NULL, 0644);

static int set_max_cpus(const char *buf, const struct kernel_param *kp)
{
	unsigned int i, ntokens = 0;
	const char *cp = buf;
	int val;

	if (!clusters_inited)
		return -EINVAL;

	while ((cp = strpbrk(cp + 1, ":")))
		ntokens++;

	if (ntokens != (num_clusters - 1))
		return -EINVAL;

	cp = buf;
	for (i = 0; i < num_clusters; i++) {

		if (sscanf(cp, "%d\n", &val) != 1)
			return -EINVAL;
		if (val > (int)cpumask_weight(managed_clusters[i]->cpus))
			return -EINVAL;

		managed_clusters[i]->max_cpu_request = val;

		cp = strnchr(cp, strlen(cp), ':');
		cp++;
		trace_set_max_cpus(cpumask_bits(managed_clusters[i]->cpus)[0],
								val);
	}

	schedule_delayed_work(&evaluate_hotplug_work, 0);

	return 0;
}

static int get_max_cpus(char *buf, const struct kernel_param *kp)
{
	int i, cnt = 0;

	if (!clusters_inited)
		return cnt;

	for (i = 0; i < num_clusters; i++)
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt,
				"%d:", managed_clusters[i]->max_cpu_request);
	cnt--;
	cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, " ");
	return cnt;
}

static const struct kernel_param_ops param_ops_max_cpus = {
	.set = set_max_cpus,
	.get = get_max_cpus,
};

device_param_cb(max_cpus, &param_ops_max_cpus, NULL, 0644);

static int set_managed_cpus(const char *buf, const struct kernel_param *kp)
{
	int i, ret;
	struct cpumask tmp_mask;

	if (!clusters_inited)
		return -EINVAL;

	ret = cpulist_parse(buf, &tmp_mask);

	if (ret)
		return ret;

	for (i = 0; i < num_clusters; i++) {
		if (cpumask_empty(managed_clusters[i]->cpus)) {
			mutex_lock(&managed_cpus_lock);
			cpumask_copy(managed_clusters[i]->cpus, &tmp_mask);
			cpumask_clear(managed_clusters[i]->offlined_cpus);
			mutex_unlock(&managed_cpus_lock);
			break;
		}
	}

	return ret;
}

static int get_managed_cpus(char *buf, const struct kernel_param *kp)
{
	int i, cnt = 0;

	if (!clusters_inited)
		return cnt;

	for (i = 0; i < num_clusters; i++) {
		cnt += cpulist_scnprintf(buf + cnt, PAGE_SIZE - cnt,
						managed_clusters[i]->cpus);
		if ((i + 1) >= num_clusters)
			break;
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, ":");
	}

	return cnt;
}

static const struct kernel_param_ops param_ops_managed_cpus = {
	.set = set_managed_cpus,
	.get = get_managed_cpus,
};
device_param_cb(managed_cpus, &param_ops_managed_cpus, NULL, 0644);

/* Read-only node: To display all the online managed CPUs */
static int get_managed_online_cpus(char *buf, const struct kernel_param *kp)
{
	int i, cnt = 0;
	struct cpumask tmp_mask;
	struct cluster *i_cl;

	if (!clusters_inited)
		return cnt;

	for (i = 0; i < num_clusters; i++) {
		i_cl = managed_clusters[i];

		cpumask_clear(&tmp_mask);
		cpumask_complement(&tmp_mask, i_cl->offlined_cpus);
		cpumask_and(&tmp_mask, i_cl->cpus, &tmp_mask);

		cnt += cpulist_scnprintf(buf + cnt, PAGE_SIZE - cnt,
								&tmp_mask);

		if ((i + 1) >= num_clusters)
			break;
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, ":");
	}

	return cnt;
}

static const struct kernel_param_ops param_ops_managed_online_cpus = {
	.get = get_managed_online_cpus,
};
device_param_cb(managed_online_cpus, &param_ops_managed_online_cpus,
								NULL, 0444);

/*
 * Userspace sends cpu#:min_freq_value to vote for min_freq_value as the new
 * scaling_min. To withdraw its vote it needs to enter cpu#:0
 */
static int set_cpu_min_freq(const char *buf, const struct kernel_param *kp)
{
	int i, j, ntokens = 0;
	unsigned int val, cpu;
	const char *cp = buf;
	struct cpu_status *i_cpu_stats;
	struct cpufreq_policy policy;
	cpumask_var_t limit_mask;
	int ret;

	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	/* CPU:value pair */
	if (!(ntokens % 2))
		return -EINVAL;

	cp = buf;
	cpumask_clear(limit_mask);
	for (i = 0; i < ntokens; i += 2) {
		if (sscanf(cp, "%u:%u", &cpu, &val) != 2)
			return -EINVAL;
		if (cpu > (num_present_cpus() - 1))
			return -EINVAL;

		i_cpu_stats = &per_cpu(cpu_stats, cpu);

		i_cpu_stats->min = val;
		cpumask_set_cpu(cpu, limit_mask);

		cp = strnchr(cp, strlen(cp), ' ');
		cp++;
	}

	/*
	 * Since on synchronous systems policy is shared amongst multiple
	 * CPUs only one CPU needs to be updated for the limit to be
	 * reflected for the entire cluster. We can avoid updating the policy
	 * of other CPUs in the cluster once it is done for at least one CPU
	 * in the cluster
	 */
	get_online_cpus();
	for_each_cpu(i, limit_mask) {
		i_cpu_stats = &per_cpu(cpu_stats, i);

		if (cpufreq_get_policy(&policy, i))
			continue;

		if (cpu_online(i) && (policy.min != i_cpu_stats->min)) {
			ret = cpufreq_update_policy(i);
			if (ret)
				continue;
		}
		for_each_cpu(j, policy.related_cpus)
			cpumask_clear_cpu(j, limit_mask);
	}
	put_online_cpus();

	return 0;
}

static int get_cpu_min_freq(char *buf, const struct kernel_param *kp)
{
	int cnt = 0, cpu;

	for_each_present_cpu(cpu) {
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt,
				"%d:%u ", cpu, per_cpu(cpu_stats, cpu).min);
	}
	cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "\n");
	return cnt;
}

static const struct kernel_param_ops param_ops_cpu_min_freq = {
	.set = set_cpu_min_freq,
	.get = get_cpu_min_freq,
};
module_param_cb(cpu_min_freq, &param_ops_cpu_min_freq, NULL, 0644);

/*
 * Userspace sends cpu#:max_freq_value to vote for max_freq_value as the new
 * scaling_max. To withdraw its vote it needs to enter cpu#:UINT_MAX
 */
static int set_cpu_max_freq(const char *buf, const struct kernel_param *kp)
{
	int i, j, ntokens = 0;
	unsigned int val, cpu;
	const char *cp = buf;
	struct cpu_status *i_cpu_stats;
	struct cpufreq_policy policy;
	cpumask_var_t limit_mask;
	int ret;

	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	/* CPU:value pair */
	if (!(ntokens % 2))
		return -EINVAL;

	cp = buf;
	cpumask_clear(limit_mask);
	for (i = 0; i < ntokens; i += 2) {
		if (sscanf(cp, "%u:%u", &cpu, &val) != 2)
			return -EINVAL;
		if (cpu > (num_present_cpus() - 1))
			return -EINVAL;

		i_cpu_stats = &per_cpu(cpu_stats, cpu);

		i_cpu_stats->max = val;
		cpumask_set_cpu(cpu, limit_mask);

		cp = strnchr(cp, strlen(cp), ' ');
		cp++;
	}

	get_online_cpus();
	for_each_cpu(i, limit_mask) {
		i_cpu_stats = &per_cpu(cpu_stats, i);
		if (cpufreq_get_policy(&policy, i))
			continue;

		if (cpu_online(i) && (policy.max != i_cpu_stats->max)) {
			ret = cpufreq_update_policy(i);
			if (ret)
				continue;
		}
		for_each_cpu(j, policy.related_cpus)
			cpumask_clear_cpu(j, limit_mask);
	}
	put_online_cpus();

	return 0;
}

static int get_cpu_max_freq(char *buf, const struct kernel_param *kp)
{
	int cnt = 0, cpu;

	for_each_present_cpu(cpu) {
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt,
				"%d:%u ", cpu, per_cpu(cpu_stats, cpu).max);
	}
	cnt += snprintf(buf + cnt, PAGE_SIZE - cnt, "\n");
	return cnt;
}

static const struct kernel_param_ops param_ops_cpu_max_freq = {
	.set = set_cpu_max_freq,
	.get = get_cpu_max_freq,
};
module_param_cb(cpu_max_freq, &param_ops_cpu_max_freq, NULL, 0644);

static unsigned int num_online_managed(struct cpumask *mask)
{
	struct cpumask tmp_mask;

	cpumask_clear(&tmp_mask);
	cpumask_and(&tmp_mask, mask, cpu_online_mask);

	return cpumask_weight(&tmp_mask);
}

static int perf_adjust_notify(struct notifier_block *nb, unsigned long val,
							void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned int cpu = policy->cpu;
	struct cpu_status *cpu_st = &per_cpu(cpu_stats, cpu);
	unsigned int min = cpu_st->min, max = cpu_st->max;


	if (val != CPUFREQ_ADJUST)
		return NOTIFY_OK;

	pr_debug("msm_perf: CPU%u policy before: %u:%u kHz\n", cpu,
						policy->min, policy->max);
	pr_debug("msm_perf: CPU%u seting min:max %u:%u kHz\n", cpu, min, max);

	cpufreq_verify_within_limits(policy, min, max);

	pr_debug("msm_perf: CPU%u policy after: %u:%u kHz\n", cpu,
						policy->min, policy->max);

	return NOTIFY_OK;
}

static struct notifier_block perf_cpufreq_nb = {
	.notifier_call = perf_adjust_notify,
};

/*
 * Attempt to offline CPUs based on their power cost.
 * CPUs with higher power costs are offlined first.
 */
static int __ref rm_high_pwr_cost_cpus(struct cluster *cl)
{
	unsigned int cpu, i;
	struct cpu_pwr_stats *per_cpu_info = get_cpu_pwr_stats();
	struct cpu_pstate_pwr *costs;
	unsigned int *pcpu_pwr;
	unsigned int max_cost_cpu, max_cost;
	int any_cpu = -1;

	if (!per_cpu_info)
		return -ENOSYS;

	for_each_cpu(cpu, cl->cpus) {
		costs = per_cpu_info[cpu].ptable;
		if (!costs || !costs[0].freq)
			continue;

		i = 1;
		while (costs[i].freq)
			i++;

		pcpu_pwr = &per_cpu(cpu_power_cost, cpu);
		*pcpu_pwr = costs[i - 1].power;
		any_cpu = (int)cpu;
		pr_debug("msm_perf: CPU:%d Power:%u\n", cpu, *pcpu_pwr);
	}

	if (any_cpu < 0)
		return -EAGAIN;

	for (i = 0; i < cpumask_weight(cl->cpus); i++) {
		max_cost = 0;
		max_cost_cpu = cpumask_first(cl->cpus);

		for_each_cpu(cpu, cl->cpus) {
			pcpu_pwr = &per_cpu(cpu_power_cost, cpu);
			if (max_cost < *pcpu_pwr) {
				max_cost = *pcpu_pwr;
				max_cost_cpu = cpu;
			}
		}

		if (!cpu_online(max_cost_cpu))
			goto end;

		pr_debug("msm_perf: Offlining CPU%d Power:%d\n", max_cost_cpu,
								max_cost);
		cpumask_set_cpu(max_cost_cpu, cl->offlined_cpus);
		if (cpu_down(max_cost_cpu)) {
			cpumask_clear_cpu(max_cost_cpu, cl->offlined_cpus);
			pr_debug("msm_perf: Offlining CPU%d failed\n",
								max_cost_cpu);
		}

end:
		pcpu_pwr = &per_cpu(cpu_power_cost, max_cost_cpu);
		*pcpu_pwr = 0;
		if (num_online_managed(cl->cpus) <= cl->max_cpu_request)
			break;
	}

	if (num_online_managed(cl->cpus) > cl->max_cpu_request)
		return -EAGAIN;
	else
		return 0;
}

/*
 * try_hotplug tries to online/offline cores based on the current requirement.
 * It loops through the currently managed CPUs and tries to online/offline
 * them until the max_cpu_request criteria is met.
 */
static void __ref try_hotplug(struct cluster *data)
{
	unsigned int i;

	if (!clusters_inited)
		return;

	pr_debug("msm_perf: Trying hotplug...%d:%d\n",
			num_online_managed(data->cpus),	num_online_cpus());

	mutex_lock(&managed_cpus_lock);
	if (num_online_managed(data->cpus) > data->max_cpu_request) {
		if (!rm_high_pwr_cost_cpus(data)) {
			mutex_unlock(&managed_cpus_lock);
			return;
		}

		/*
		 * If power aware offlining fails due to power cost info
		 * being unavaiable fall back to original implementation
		 */
		for (i = num_present_cpus() - 1; i >= 0 &&
						i < num_present_cpus(); i--) {
			if (!cpumask_test_cpu(i, data->cpus) ||	!cpu_online(i))
				continue;

			pr_debug("msm_perf: Offlining CPU%d\n", i);
			cpumask_set_cpu(i, data->offlined_cpus);
			if (cpu_down(i)) {
				cpumask_clear_cpu(i, data->offlined_cpus);
				pr_debug("msm_perf: Offlining CPU%d failed\n",
									i);
				continue;
			}
			if (num_online_managed(data->cpus) <=
							data->max_cpu_request)
				break;
		}
	} else {
		for_each_cpu(i, data->cpus) {
			if (cpu_online(i))
				continue;
			pr_debug("msm_perf: Onlining CPU%d\n", i);
			if (cpu_up(i)) {
				pr_debug("msm_perf: Onlining CPU%d failed\n",
									i);
				continue;
			}
			cpumask_clear_cpu(i, data->offlined_cpus);
			if (num_online_managed(data->cpus) >=
							data->max_cpu_request)
				break;
		}
	}
	mutex_unlock(&managed_cpus_lock);
}

static void __ref release_cluster_control(struct cpumask *off_cpus)
{
	int cpu;

	for_each_cpu(cpu, off_cpus) {
		pr_debug("msm_perf: Release CPU %d\n", cpu);
		if (!cpu_up(cpu))
			cpumask_clear_cpu(cpu, off_cpus);
	}
}

/* Work to evaluate current online CPU status and hotplug CPUs as per need*/
static void check_cluster_status(struct work_struct *work)
{
	int i;
	struct cluster *i_cl;

	for (i = 0; i < num_clusters; i++) {
		i_cl = managed_clusters[i];

		if (cpumask_empty(i_cl->cpus))
			continue;

		if (i_cl->max_cpu_request < 0) {
			if (!cpumask_empty(i_cl->offlined_cpus))
				release_cluster_control(i_cl->offlined_cpus);
			continue;
		}

		if (num_online_managed(i_cl->cpus) !=
					i_cl->max_cpu_request)
			try_hotplug(i_cl);
	}
}

static int __ref msm_performance_cpu_callback(struct notifier_block *nfb,
		unsigned long action, void *hcpu)
{
	uint32_t cpu = (uintptr_t)hcpu;
	unsigned int i;
	struct cluster *i_cl = NULL;

	if (!clusters_inited)
		return NOTIFY_OK;

	for (i = 0; i < num_clusters; i++) {
		if (cpumask_test_cpu(cpu, managed_clusters[i]->cpus)) {
			i_cl = managed_clusters[i];
			break;
		}
	}

	if (i_cl == NULL)
		return NOTIFY_OK;

	if (action == CPU_UP_PREPARE || action == CPU_UP_PREPARE_FROZEN) {
		/*
		 * Prevent onlining of a managed CPU if max_cpu criteria is
		 * already satisfied
		 */
		if (i_cl->max_cpu_request <=
					num_online_managed(i_cl->cpus)) {
			pr_debug("msm_perf: Prevent CPU%d onlining\n", cpu);
			cpumask_set_cpu(cpu, i_cl->offlined_cpus);
			return NOTIFY_BAD;
		}
		cpumask_clear_cpu(cpu, i_cl->offlined_cpus);

	} else if (action == CPU_DEAD) {
		if (cpumask_test_cpu(cpu, i_cl->offlined_cpus))
			return NOTIFY_OK;
		/*
		 * Schedule a re-evaluation to check if any more CPUs can be
		 * brought online to meet the max_cpu_request requirement. This
		 * work is delayed to account for CPU hotplug latencies
		 */
		if (schedule_delayed_work(&evaluate_hotplug_work, 0)) {
			trace_reevaluate_hotplug(cpumask_bits(i_cl->cpus)[0],
							i_cl->max_cpu_request);
			pr_debug("msm_perf: Re-evaluation scheduled %d\n", cpu);
		} else {
			pr_debug("msm_perf: Work scheduling failed %d\n", cpu);
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block __refdata msm_performance_cpu_notifier = {
	.notifier_call = msm_performance_cpu_callback,
};

static int init_cluster_control(void)
{
	unsigned int i;

	managed_clusters = kcalloc(num_clusters, sizeof(struct cluster *),
								GFP_KERNEL);
	if (!managed_clusters)
		return -ENOMEM;
	for (i = 0; i < num_clusters; i++) {
		managed_clusters[i] = kcalloc(1, sizeof(struct cluster),
								GFP_KERNEL);
		if (!managed_clusters[i])
			return -ENOMEM;
		managed_clusters[i]->max_cpu_request = -1;
	}

	INIT_DELAYED_WORK(&evaluate_hotplug_work, check_cluster_status);
	mutex_init(&managed_cpus_lock);

	clusters_inited = true;
	return 0;
}

static int __init msm_performance_init(void)
{
	unsigned int cpu;

	cpufreq_register_notifier(&perf_cpufreq_nb, CPUFREQ_POLICY_NOTIFIER);
	for_each_present_cpu(cpu)
		per_cpu(cpu_stats, cpu).max = UINT_MAX;
	register_cpu_notifier(&msm_performance_cpu_notifier);
	return 0;
}
late_initcall(msm_performance_init);
