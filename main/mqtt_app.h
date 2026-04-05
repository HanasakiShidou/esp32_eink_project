// mqtt_app.h
#pragma once

void mqtt5_app_start(void);

/**
 * @brief 异步发送日志消息到 MQTT 的 /logger 主题
 *
 * @param msg 指向要发送的字符串的指针。该指针指向的内存必须通过 malloc 分配，
 *            函数将接管其控制权（最终由内部任务 free）。不要在调用后再次使用该指针。
 *            如果传入 NULL，函数会直接返回。
 *
 * @note 如果内部队列已满，消息将被丢弃，并释放 msg 指向的内存。
 */
void mqtt_log(char *msg);