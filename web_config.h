#define WEB_CONFIG_H

void (*config_callback) (char *);

void startConfigManager(void (*_func) (char *));
