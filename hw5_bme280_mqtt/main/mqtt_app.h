// mqtt_app.h
#ifndef MQTT_APP_H
#define MQTT_APP_H

#include "mqtt_client.h"

void mqtt_app_start(void);
void mqtt_app_publish(const char *topic, const char *data);

#endif