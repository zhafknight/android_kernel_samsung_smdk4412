#ifndef PLAT_STUPID_H
#define PLAT_STUPID_H

struct pwm_device_deprecated;
struct pwm_device_deprecated *pwm_request_deprecated(int pwm_id, const char *label);
void pwm_free_deprecated(struct pwm_device_deprecated *pwm);
int pwm_enable_deprecated(struct pwm_device_deprecated *pwm);
void pwm_disable_deprecated(struct pwm_device_deprecated *pwm);
int pwm_config_deprecated(struct pwm_device_deprecated *pwm, int duty_ns, int period_ns);
#endif
