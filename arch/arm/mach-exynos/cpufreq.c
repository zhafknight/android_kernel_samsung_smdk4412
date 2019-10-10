/* linux/arch/arm/mach-exynos/cpufreq.c
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS - CPU frequency scaling support for EXYNOS series
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/cpufreq.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-mem.h>
#include <mach/cpufreq.h>
#include <mach/asv.h>

#include <plat/clock.h>
#include <plat/pm.h>
#include <plat/cpu.h>

#if defined(CONFIG_MACH_PX) || defined(CONFIG_MACH_Q1_BD) ||\
	defined(CONFIG_MACH_P4NOTE) || defined(CONFIG_MACH_SP7160LTE) ||\
	defined(CONFIG_MACH_GC1) || defined(CONFIG_MACH_TAB3) ||\
	defined(CONFIG_MACH_GC2PD)
#include <mach/sec_debug.h>
#endif

struct exynos_dvfs_info *exynos_info;

static struct regulator *arm_regulator;
static struct cpufreq_freqs freqs;

static bool exynos_cpufreq_disable;
static bool exynos_cpufreq_lock_disable;
static bool exynos_cpufreq_init_done;

static unsigned int locking_frequency;
static bool frequency_locked;
static DEFINE_MUTEX(cpufreq_lock);
static DEFINE_MUTEX(set_cpufreq_lock);

unsigned int g_cpufreq_limit_id;
unsigned int g_cpufreq_limit_val[DVFS_LOCK_ID_END];
unsigned int g_cpufreq_limit_level;

unsigned int g_cpufreq_lock_id;
unsigned int g_cpufreq_lock_val[DVFS_LOCK_ID_END];
unsigned int g_cpufreq_lock_level;

static unsigned int exynos_get_safe_armvolt(unsigned int old_index, unsigned int new_index)
{
	unsigned int safe_arm_volt = 0;
	struct cpufreq_frequency_table *freq_table = exynos_info->freq_table;
	unsigned int *volt_table = exynos_info->volt_table;

	/*
	 * ARM clock source will be changed APLL to MPLL temporary
	 * To support this level, need to control regulator for
	 * reguired voltage level
	 */

	if (exynos_info->need_apll_change != NULL) {
		if (exynos_info->need_apll_change(old_index, new_index) &&
			(freq_table[new_index].frequency < exynos_info->mpll_freq_khz) &&
			(freq_table[old_index].frequency < exynos_info->mpll_freq_khz)) {
				safe_arm_volt = volt_table[exynos_info->pll_safe_idx];
			}

	}

	return safe_arm_volt;
}

static unsigned int exynos_getspeed(unsigned int cpu)
{
	return clk_get_rate(exynos_info->cpu_clk) / 1000;
}

static int exynos_cpufreq_get_index(unsigned int freq)
{
	struct cpufreq_frequency_table *freq_table = exynos_info->freq_table;
	int index;

	for (index = 0;
		freq_table[index].frequency != CPUFREQ_TABLE_END; index++)
		if (freq_table[index].frequency == freq)
			break;

	if (freq_table[index].frequency == CPUFREQ_TABLE_END)
		return -EINVAL;

	return index;
}

static int exynos_cpufreq_scale(unsigned int target_freq)
{
	struct cpufreq_frequency_table *freq_table = exynos_info->freq_table;
	unsigned int *volt_table = exynos_info->volt_table;
	struct cpufreq_policy *policy = cpufreq_cpu_get(0);
	unsigned int arm_volt, safe_arm_volt = 0;
	unsigned int mpll_freq_khz = exynos_info->mpll_freq_khz;
	unsigned int old_freq;
	int index, old_index;
	int ret = 0;

	old_freq = policy->cur;

	/*
	 * The policy max have been changed so that we cannot get proper
	 * old_index with cpufreq_frequency_table_target(). Thus, ignore
	 * policy and get the index from the raw frequency table.
	 */
	old_index = exynos_cpufreq_get_index(old_freq);
	if (old_index < 0) {
		ret = old_index;
		goto out;
	}

	index = exynos_cpufreq_get_index(target_freq);
	if (index < 0) {
		ret = index;
		goto out;
	}

	/*
	 * ARM clock source will be changed APLL to MPLL temporary
	 * To support this level, need to control regulator for
	 * required voltage level
	 */
	if (exynos_info->need_apll_change != NULL) {
		if (exynos_info->need_apll_change(old_index, index) &&
		   (freq_table[index].frequency < mpll_freq_khz) &&
		   (freq_table[old_index].frequency < mpll_freq_khz))
			safe_arm_volt = volt_table[exynos_info->pll_safe_idx];
	}
	arm_volt = volt_table[index];

	/* When the new frequency is higher than current frequency */
	if ((target_freq > old_freq) && !safe_arm_volt) {
		/* Firstly, voltage up to increase frequency */
		ret = regulator_set_voltage(arm_regulator, arm_volt, arm_volt);
		if (ret) {
			pr_err("%s: failed to set cpu voltage to %d\n",
				__func__, arm_volt);
			return ret;
		}
	}

	if (safe_arm_volt) {
		ret = regulator_set_voltage(arm_regulator, safe_arm_volt,
				      safe_arm_volt);
		if (ret) {
			pr_err("%s: failed to set cpu voltage to %d\n",
				__func__, safe_arm_volt);
			return ret;
		}
	}

	exynos_info->set_freq(old_index, index);

	/* When the new frequency is lower than current frequency */
	if ((target_freq < old_freq) ||
	   ((target_freq > old_freq) && safe_arm_volt)) {
		/* down the voltage after frequency change */
		ret = regulator_set_voltage(arm_regulator, arm_volt,
				arm_volt);
		if (ret) {
			pr_err("%s: failed to set cpu voltage to %d\n",
				__func__, arm_volt);
			goto out;
		}
	}

out:
	cpufreq_cpu_put(policy);

	return ret;
}

static int exynos_target(struct cpufreq_policy *policy, unsigned int index)
{
	struct cpufreq_frequency_table *freq_table = exynos_info->freq_table;
	int ret = 0;

	mutex_lock(&cpufreq_lock);

	if (frequency_locked)
		goto out;

	ret = exynos_cpufreq_scale(freq_table[index].frequency);

out:
	mutex_unlock(&cpufreq_lock);

	return ret;
}

/**
 * exynos_find_cpufreq_level_by_volt - find cpufreqi_level by requested
 * arm voltage.
 *
 * This function finds the cpufreq_level to set for voltage above req_volt
 * and return its value.
 */
int exynos_find_cpufreq_level_by_volt(unsigned int arm_volt,
					unsigned int *level)
{
	struct cpufreq_frequency_table *table;
	unsigned int *volt_table = exynos_info->volt_table;
	int i;

	if (!exynos_cpufreq_init_done)
		return -EINVAL;

	table = cpufreq_frequency_get_table(0);
	if (!table) {
		pr_err("%s: Failed to get the cpufreq table\n", __func__);
		return -EINVAL;
	}

	/* check if arm_volt has value or not */
	if (!arm_volt) {
		pr_err("%s: req_volt has no value.\n", __func__);
		return -EINVAL;
	}

	/* find cpufreq level in volt_table */
	for (i = exynos_info->min_support_idx;
			i >= exynos_info->max_support_idx; i--) {
		if (volt_table[i] >= arm_volt) {
			*level = (unsigned int)i;
			return 0;
		}
	}

	pr_err("%s: Failed to get level for %u uV\n", __func__, arm_volt);

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(exynos_find_cpufreq_level_by_volt);

int exynos_cpufreq_get_level(unsigned int freq, unsigned int *level)
{
	struct cpufreq_frequency_table *table;
	unsigned int i;

	if (!exynos_cpufreq_init_done)
		return -EINVAL;

	table = cpufreq_frequency_get_table(0);
	if (!table) {
		pr_err("%s: Failed to get the cpufreq table\n", __func__);
		return -EINVAL;
	}

	for (i = exynos_info->max_support_idx;
		(table[i].frequency != CPUFREQ_TABLE_END); i++) {
		if (table[i].frequency == freq) {
			*level = i;
			return 0;
		}
	}

	pr_err("%s: %u KHz is an unsupported cpufreq\n", __func__, freq);

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(exynos_cpufreq_get_level);

atomic_t exynos_cpufreq_lock_count;

int exynos_cpufreq_lock(unsigned int nId,
			 enum cpufreq_level_index cpufreq_level)
{
	return 0;
#if 0
	int ret = 0, i, old_idx = -EINVAL;
	unsigned int freq_old, freq_new, arm_volt, safe_arm_volt;
	unsigned int *volt_table;
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *freq_table;

	if (!exynos_cpufreq_init_done)
		return -EPERM;

	if (!exynos_info)
		return -EPERM;

	if (exynos_cpufreq_disable && (nId != DVFS_LOCK_ID_TMU)) {
		pr_info("CPUFreq is already fixed\n");
		return -EPERM;
	}

	if (cpufreq_level < exynos_info->max_support_idx
			|| cpufreq_level > exynos_info->min_support_idx) {
		pr_warn("%s: invalid cpufreq_level(%d:%d)\n", __func__, nId,
				cpufreq_level);
		return -EINVAL;
	}

	policy = cpufreq_cpu_get(0);
	if (!policy)
		return -EPERM;

	volt_table = exynos_info->volt_table;
	freq_table = exynos_info->freq_table;

	mutex_lock(&set_cpufreq_lock);
	if (g_cpufreq_lock_id & (1 << nId)) {
		printk(KERN_ERR "%s:Device [%d] already locked cpufreq\n",
				__func__,  nId);
		mutex_unlock(&set_cpufreq_lock);
		return 0;
	}

	g_cpufreq_lock_id |= (1 << nId);
	g_cpufreq_lock_val[nId] = cpufreq_level;

	/* If the requested cpufreq is higher than current min frequency */
	if (cpufreq_level < g_cpufreq_lock_level)
		g_cpufreq_lock_level = cpufreq_level;

	mutex_unlock(&set_cpufreq_lock);

	if ((g_cpufreq_lock_level < g_cpufreq_limit_level)
				&& (nId != DVFS_LOCK_ID_PM))
		return 0;

	/* Do not setting cpufreq lock frequency
	 * because current governor doesn't support dvfs level lock
	 * except DVFS_LOCK_ID_PM */
	if (exynos_cpufreq_lock_disable && (nId != DVFS_LOCK_ID_PM))
		return 0;

	/* If current frequency is lower than requested freq,
	 * it needs to update
	 */
	mutex_lock(&cpufreq_lock);
	freq_old = policy->cur;
	freq_new = freq_table[cpufreq_level].frequency;

	if (freq_old < freq_new) {
		/* Find out current level index */
		for (i = 0; freq_table[i].frequency != CPUFREQ_TABLE_END; i++) {
			if (freq_old == freq_table[i].frequency) {
				old_idx = freq_table[i].driver_data;
				break;
			}
		}
		if (old_idx == -EINVAL) {
			printk(KERN_ERR "%s: Level not found\n", __func__);
			mutex_unlock(&cpufreq_lock);
			return -EINVAL;
		}

		freqs.old = freq_old;
		freqs.new = freq_new;
		cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

		/* get the voltage value */
		safe_arm_volt = exynos_get_safe_armvolt(old_idx, cpufreq_level);
		if (safe_arm_volt)
			regulator_set_voltage(arm_regulator, safe_arm_volt,
					     safe_arm_volt + 25000);

		arm_volt = volt_table[cpufreq_level];
		regulator_set_voltage(arm_regulator, arm_volt,
				     arm_volt + 25000);

		exynos_info->set_freq(old_idx, cpufreq_level);

		cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
	}

	mutex_unlock(&cpufreq_lock);
	return ret;
#endif
}
EXPORT_SYMBOL_GPL(exynos_cpufreq_lock);

void exynos_cpufreq_lock_free(unsigned int nId)
{
#if 0
	unsigned int i;

	if (!exynos_cpufreq_init_done)
		return;

	mutex_lock(&set_cpufreq_lock);
	g_cpufreq_lock_id &= ~(1 << nId);
	g_cpufreq_lock_val[nId] = exynos_info->min_support_idx;
	g_cpufreq_lock_level = exynos_info->min_support_idx;
	if (g_cpufreq_lock_id) {
		for (i = 0; i < DVFS_LOCK_ID_END; i++) {
			if (g_cpufreq_lock_val[i] < g_cpufreq_lock_level)
				g_cpufreq_lock_level = g_cpufreq_lock_val[i];
		}
	}
	mutex_unlock(&set_cpufreq_lock);
#endif
}
EXPORT_SYMBOL_GPL(exynos_cpufreq_lock_free);

#ifdef CONFIG_SLP
static int exynos_cpu_dma_qos_notify(struct notifier_block *nb,
				     unsigned long value, void *data)
{
	int i;
	struct dvfs_qos_info *table;
	enum cpufreq_level_index last_lvl = L0;

	if (!exynos_info || !exynos_info->cpu_dma_latency)
		return NOTIFY_DONE;

	if (value == 0 || value == PM_QOS_DEFAULT_VALUE ||
	    value == PM_QOS_CPU_DMA_LAT_DEFAULT_VALUE) {
		exynos_cpufreq_lock_free(DVFS_LOCK_ID_QOS_DMA_LATENCY);
		return NOTIFY_OK;
	}

	table = exynos_info->cpu_dma_latency;

	for (i = 0; table[i].qos_value; i++) {
		if (value >= table[i].qos_value) {
			exynos_cpufreq_lock(DVFS_LOCK_ID_QOS_DMA_LATENCY,
					    table[i].level);
			return NOTIFY_OK;
		}
		last_lvl = table[i].level;
	}

	if (last_lvl > L0)
		last_lvl--;

	exynos_cpufreq_lock(DVFS_LOCK_ID_QOS_DMA_LATENCY, last_lvl);

	return NOTIFY_OK;
}

static struct notifier_block pm_qos_cpu_dma_notifier = {
	.notifier_call = exynos_cpu_dma_qos_notify,
};
#endif /* CONFIG_SLP */

int exynos_cpufreq_upper_limit(unsigned int nId,
				enum cpufreq_level_index cpufreq_level)
{
	return 0;
#if 0
	int ret = 0, old_idx = 0, i;
	unsigned int freq_old, freq_new, arm_volt, safe_arm_volt;
	unsigned int *volt_table;
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *freq_table;

	if (!exynos_cpufreq_init_done)
		return -EPERM;

	if (!exynos_info)
		return -EPERM;

	if (exynos_cpufreq_disable) {
		pr_info("CPUFreq is already fixed\n");
		return -EPERM;
	}

	if (cpufreq_level < exynos_info->max_support_idx
			|| cpufreq_level > exynos_info->min_support_idx) {
		pr_warn("%s: invalid cpufreq_level(%d:%d)\n", __func__, nId,
				cpufreq_level);
		return -EINVAL;
	}

	policy = cpufreq_cpu_get(0);
	if (!policy)
		return -EPERM;

	volt_table = exynos_info->volt_table;
	freq_table = exynos_info->freq_table;

	mutex_lock(&set_cpufreq_lock);
	if (g_cpufreq_limit_id & (1 << nId)) {
		pr_err("[CPUFREQ]This device [%d] already limited cpufreq\n", nId);
		mutex_unlock(&set_cpufreq_lock);
		return 0;
	}

	g_cpufreq_limit_id |= (1 << nId);
	g_cpufreq_limit_val[nId] = cpufreq_level;

	/* If the requested limit level is lower than current value */
	if (cpufreq_level > g_cpufreq_limit_level)
		g_cpufreq_limit_level = cpufreq_level;

	mutex_unlock(&set_cpufreq_lock);

	mutex_lock(&cpufreq_lock);
	/* If cur frequency is higher than limit freq, it needs to update */
	freq_old = policy->cur;
	freq_new = freq_table[cpufreq_level].frequency;

	if (freq_old > freq_new) {
		/* Find out current level index */
		for (i = 0; i <= exynos_info->min_support_idx; i++) {
			if (freq_old == freq_table[i].frequency) {
				old_idx = freq_table[i].driver_data;
				break;
			} else if (i == exynos_info->min_support_idx) {
				printk(KERN_ERR "%s: Level is not found\n", __func__);
				mutex_unlock(&cpufreq_lock);

				return -EINVAL;
			} else {
				continue;
			}
		}
		freqs.old = freq_old;
		freqs.new = freq_new;

		cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

		exynos_info->set_freq(old_idx, cpufreq_level);

		safe_arm_volt = exynos_get_safe_armvolt(old_idx, cpufreq_level);
		if (safe_arm_volt)
			regulator_set_voltage(arm_regulator, safe_arm_volt,
					     safe_arm_volt + 25000);

		arm_volt = volt_table[cpufreq_level];
		regulator_set_voltage(arm_regulator, arm_volt, arm_volt + 25000);

		cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
	}

	mutex_unlock(&cpufreq_lock);

	return ret;
#endif
}

void exynos_cpufreq_upper_limit_free(unsigned int nId)
{
#if 0
	unsigned int i;

	if (!exynos_cpufreq_init_done)
		return;

	mutex_lock(&set_cpufreq_lock);
	g_cpufreq_limit_id &= ~(1 << nId);
	g_cpufreq_limit_val[nId] = exynos_info->max_support_idx;
	g_cpufreq_limit_level = exynos_info->max_support_idx;

	if (g_cpufreq_limit_id) {
		for (i = 0; i < DVFS_LOCK_ID_END; i++) {
			if (g_cpufreq_limit_val[i] > g_cpufreq_limit_level)
				g_cpufreq_limit_level = g_cpufreq_limit_val[i];
		}
	}
	mutex_unlock(&set_cpufreq_lock);
#endif
}

/* This API serve highest priority level locking */
int exynos_cpufreq_level_fix(unsigned int freq)
{
	return 0;
#if 0
	struct cpufreq_policy *policy;
	int ret = 0;

	if (!exynos_cpufreq_init_done)
		return -EPERM;

	policy = cpufreq_cpu_get(0);
	if (!policy)
		return -EPERM;

	if (exynos_cpufreq_disable) {
		pr_info("CPUFreq is already fixed\n");
		return -EPERM;
	}
	ret = exynos_target(policy, freq, CPUFREQ_RELATION_L);

	exynos_cpufreq_disable = true;
	return ret;
#endif
}
EXPORT_SYMBOL_GPL(exynos_cpufreq_level_fix);

void exynos_cpufreq_level_unfix(void)
{
#if 0
	if (!exynos_cpufreq_init_done)
		return;

	exynos_cpufreq_disable = false;
#endif
}
EXPORT_SYMBOL_GPL(exynos_cpufreq_level_unfix);

int exynos_cpufreq_is_fixed(void)
{
	return exynos_cpufreq_disable;
}
EXPORT_SYMBOL_GPL(exynos_cpufreq_is_fixed);

#ifdef CONFIG_PM
static int exynos_cpufreq_suspend(struct cpufreq_policy *policy)
{
	return 0;
}

static int exynos_cpufreq_resume(struct cpufreq_policy *policy)
{
	return 0;
}
#endif

static int exynos_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	int retval;

	retval = cpufreq_generic_init(policy, exynos_info->freq_table, 100000);

	/* Keep stock frq. as default startup frq. */
	policy->max = 1400000;
	policy->min = 200000;

	return retval;
}

/**
 * exynos_cpufreq_pm_notifier - block CPUFREQ's activities in suspend-resume
 *			context
 * @notifier
 * @pm_event
 * @v
 *
 * While frequency_locked == true, target() ignores every frequency but
 * locking_frequency. The locking_frequency value is the initial frequency,
 * which is set by the bootloader. In order to eliminate possible
 * inconsistency in clock values, we save and restore frequencies during
 * suspend and resume and block CPUFREQ activities. Note that the standard
 * suspend/resume cannot be used as they are too deep (syscore_ops) for
 * regulator actions.
 */
static int exynos_cpufreq_pm_notifier(struct notifier_block *notifier,
				       unsigned long pm_event, void *v)
{
	int ret;

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		mutex_lock(&cpufreq_lock);
		frequency_locked = true;
		mutex_unlock(&cpufreq_lock);

		ret = exynos_cpufreq_scale(locking_frequency);
		if (ret < 0)
			return NOTIFY_BAD;

		break;

	case PM_POST_SUSPEND:
		mutex_lock(&cpufreq_lock);
		frequency_locked = false;
		mutex_unlock(&cpufreq_lock);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block exynos_cpufreq_nb = {
	.notifier_call = exynos_cpufreq_pm_notifier,
};

static struct cpufreq_driver exynos_driver = {
	.flags		= CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= exynos_target,
	.get		= exynos_getspeed,
	.init		= exynos_cpufreq_cpu_init,
	.exit		= cpufreq_generic_exit,
	.name		= "exynos_cpufreq",
	.attr		= cpufreq_generic_attr,
#ifdef CONFIG_PM
	.suspend	= exynos_cpufreq_suspend,
	.resume		= exynos_cpufreq_resume,
#endif
};

static int exynos_cpufreq_probe(struct platform_device *pdev)
{
	int ret = -EINVAL;
	int i;

	exynos_info = kzalloc(sizeof(struct exynos_dvfs_info), GFP_KERNEL);
	if (!exynos_info)
		return -ENOMEM;

	if (soc_is_exynos4210())
		ret = exynos4210_cpufreq_init(exynos_info);
	else if (soc_is_exynos4212() || soc_is_exynos4412())
		ret = exynos4x12_cpufreq_init(exynos_info);
	else if (soc_is_exynos5250())
		ret = exynos5250_cpufreq_init(exynos_info);
	else
		pr_err("%s: CPU type not found\n", __func__);

	if (ret)
		goto err_vdd_arm;

	if (exynos_info->set_freq == NULL) {
		printk(KERN_ERR "%s: No set_freq function (ERR)\n",
				__func__);
		goto err_vdd_arm;
	}

	arm_regulator = regulator_get(NULL, "vdd_arm");
	if (IS_ERR(arm_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vdd_arm");
		goto err_vdd_arm;
	}

	exynos_cpufreq_disable = false;

	exynos_cpufreq_init_done = true;

	for (i = 0; i < DVFS_LOCK_ID_END; i++) {
		g_cpufreq_lock_val[i] = exynos_info->min_support_idx;
		g_cpufreq_limit_val[i] = exynos_info->max_support_idx;
	}

	g_cpufreq_lock_level = exynos_info->min_support_idx;
	g_cpufreq_limit_level = exynos_info->max_support_idx;

	locking_frequency = exynos_getspeed(0);

	register_pm_notifier(&exynos_cpufreq_nb);

	if (cpufreq_register_driver(&exynos_driver)) {
		pr_err("failed to register cpufreq driver\n");
		goto err_cpufreq;
	}

#ifdef CONFIG_SLP
	if (exynos_info->cpu_dma_latency)
		pm_qos_add_notifier(PM_QOS_CPU_DMA_LATENCY,
				    &pm_qos_cpu_dma_notifier);
#endif

	return 0;
err_cpufreq:
	if (!IS_ERR(arm_regulator))
		regulator_put(arm_regulator);
err_vdd_arm:
	kfree(exynos_info);
	pr_debug("%s: failed initialization\n", __func__);
	return -EINVAL;
}

static struct platform_driver exynos_cpufreq_platdrv = {
	.driver = {
		.name	= "exynos-cpufreq",
		.owner	= THIS_MODULE,
	},
	.probe = exynos_cpufreq_probe,
};
module_platform_driver(exynos_cpufreq_platdrv);

/* sysfs interface for UV control */
ssize_t show_UV_mV_table(struct cpufreq_policy *policy, char *buf) {

  int i, len = 0;
  if (buf)
  {
    for (i = exynos_info->max_support_idx; i<=exynos_info->min_support_idx; i++)
    {
      if(exynos_info->freq_table[i].frequency==CPUFREQ_ENTRY_INVALID) continue;
      len += sprintf(buf + len, "%dmhz: %d mV\n", exynos_info->freq_table[i].frequency/1000,
					((exynos_info->volt_table[i] % 1000) + exynos_info->volt_table[i])/1000);
    }
  }
  return len;
}

ssize_t store_UV_mV_table(struct cpufreq_policy *policy,
                                      const char *buf, size_t count) {

	unsigned int ret = -EINVAL;
   int i = 0;
   int j = 0;
	int u[15];
   ret = sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &u[0], &u[1], &u[2], &u[3], &u[4], &u[5], &u[6],
															&u[7], &u[8], &u[9], &u[10], &u[11], &u[12], &u[13], &u[14]);
	if(ret != 15) {
		ret = sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d", &u[0], &u[1], &u[2], &u[3], &u[4], &u[5], &u[6],
															&u[7], &u[8], &u[9], &u[10], &u[11], &u[12], &u[13]);
		if(ret != 14) {
			ret = sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d", &u[0], &u[1], &u[2], &u[3], &u[4], &u[5], &u[6],
															&u[7], &u[8], &u[9], &u[10], &u[11], &u[12]);
			if( ret != 12)
				return -EINVAL;
		}
	}

	for( i = 0; i < 15; i++ )
	{
		u[i] *= 1000;
		// round down voltages - thx to AndreiLux
		if(u[i] % 12500)
			u[i] = (u[i] / 12500) * 12500;

		if (u[i] > CPU_UV_MV_MAX) {
			u[i] = CPU_UV_MV_MAX;
		}
		else if (u[i] < CPU_UV_MV_MIN) {
			u[i] = CPU_UV_MV_MIN;
		}
	}

	for( i = 0; i < 15; i++ ) {
		while(exynos_info->freq_table[i+j].frequency==CPUFREQ_ENTRY_INVALID)
			j++;
		exynos_info->volt_table[i+j] = u[i];
	}
	return count;
}

/* sysfs interface for ASV level */
ssize_t show_asv_level(struct cpufreq_policy *policy, char *buf) {

	return sprintf(buf, "ASV level: %d\n",exynos_result_of_asv); 

}

extern ssize_t store_asv_level(struct cpufreq_policy *policy,
                                      const char *buf, size_t count) {
	
	// the store function does not do anything
	return count;
}
