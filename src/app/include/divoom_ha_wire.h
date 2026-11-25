#ifndef DIVOOM_HA_WIRE_H
#define DIVOOM_HA_WIRE_H
#include <stddef.h>
#define DIVOOM_HA_WIRE_MESSAGE_MAX (128 * 1024)
struct divoom_ha_wire;
/* LAN ws:// only in MVP. No silent TLS downgrade. */
/*********************************************************************
  Function:        divoom_ha_wire_open
  Description:     分配资源，建立局域网 TCP 连接并校验 WebSocket 升级握手。
  Input:           out：返回连接；host：主机；port：端口。
  Return:          0：握手成功；-1：分配、解析、连接或握手失败，*out 为 NULL。
  调用约束:         后台任务调用；仅 ws，不支持 WSS，不复用云 MQTT 连接。
**********************************************************************/
int divoom_ha_wire_open(struct divoom_ha_wire **out, const char *host, unsigned port);
/*********************************************************************
  Function:        divoom_ha_wire_send_text
  Description:     将一条 JSON 文本作为 WebSocket 文本帧发送。
  Input:           w：连接；json：正确终止的 JSON 字符串。
  Return:          0：成功；-1：帧发送失败。
  调用约束:         后台任务调用；不接管 json，控制消息须小于 65536 字节。
**********************************************************************/
int divoom_ha_wire_send_text(struct divoom_ha_wire *wire, const char *json);
/* 1=text, 0=idle/control frame, -1=closed/protocol/timeout, -2=message exceeds limit. Text owned by wire. */
/*********************************************************************
  Function:        divoom_ha_wire_receive_text
  Description:     接收并拼接文本分片，处理 ping/pong，拒绝不支持或超限报文。
  Input:           w：连接；text：返回完整消息的内部缓冲指针。
  Return:          1：完整文本；0：无完整文本/已处理控制帧；-1：关闭、协议或超时错误；-2：消息超限。
  调用约束:         后台任务调用；text 不可释放，仅在下一次接收或关闭前有效。
**********************************************************************/
int divoom_ha_wire_receive_text(struct divoom_ha_wire *wire, const char **text);
/*********************************************************************
  Function:        divoom_ha_wire_close
  Description:     关闭 socket，释放收包缓冲、随机数上下文和连接结构。
  Input:           w：连接，可为 NULL。
  Return:          无。
  调用约束:         仅连接所有者调用；返回后 w 及此前 text 指针均不能使用。
**********************************************************************/
void divoom_ha_wire_close(struct divoom_ha_wire *wire);
#endif