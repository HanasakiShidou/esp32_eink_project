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

/**
 * @brief MQTT /rpc_interface/request 主题的handler
 *
 * @param data 指向接受的数据的指针。
 *             如果传入 NULL，函数会直接返回。
 * 
 * @param length 接受的数据长度，小于等于0值会导致函数直接返回。
 * 
 * @note 假定函数调用是原子的
 */
void mqtt_handle_request(unsigned char *data, int length);

/**
 * @brief 异步发送日志消息到 MQTT 的 /rpc_interface/response 主题
 *
 * @param data 指向要发送的数据的指针。函数不会获取指针指向内存的所有权。
 *             如果传入 NULL，函数会直接返回。
 *
 * @param length 要发送的数据长度，小于等于0值会导致函数直接返回。
 * 
 * @note 如果内部队列已满，消息将被丢弃。
 */
void mqtt_send_response(char *data, int length);