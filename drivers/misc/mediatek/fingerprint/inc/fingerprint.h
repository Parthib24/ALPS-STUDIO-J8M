#ifndef __FINGERPRINT_H__
#define __FINGERPRINT_H__

bool getFP_driver_probe_status(void);
void setFP_driver_probe_status(bool status);
int rgk_fingerprint_power_on(struct device *dev);
int rgk_fingerprint_power_off(void);

#endif
