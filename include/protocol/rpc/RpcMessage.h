/** 
 * @file RpcMessage.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/13
 * @description Rpc协议通信传输的消息数据
 */
#ifndef MNSX_ACHILLESLINK_RPCMESSAGE_H
#define MNSX_ACHILLESLINK_RPCMESSAGE_H

#include <cstdint>
#include <string>

namespace mnsx {
    namespace achilles {

        // 消息类型标识
        enum class RpcMessageType : uint8_t {
            REQUEST = 0, // 请求
            RESPONSE = 1, // 响应
            HEARTBEAT_PING = 2, // 心跳包
            HEARTBEAT_PONG = 3 // 心跳包回复
        };

        struct RpcMessage {
            uint8_t version = 1; // 版本号
            RpcMessageType msg_type = RpcMessageType::REQUEST; // 消息类型
            uint64_t request_id = 0; // 请求的编号，要求响应编号与请求相同
            uint32_t body_len = 0; // 消息体的长度

            std::string body; // 消息体
        };

    }
}

#endif //MNSX_ACHILLESLINK_RPCMESSAGE_H