/** 
 * @file RpcCodec.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/13
 * @description Rpc协议编码解码器
 */
#ifndef MNSX_ACHILLESLINK_RPCCODEC_H
#define MNSX_ACHILLESLINK_RPCCODEC_H

#include "RpcMessage.h"
#include "../src/net/ByteBuffer.h"

#include <cstring>
#include <string>
#include <netinet/in.h>
#include <endian.h>

namespace mnsx {
    namespace achilles {

        class RpcCodec {
        public:
            constexpr static const int HEADER_LEN = 16;

            static bool decode(ByteBuffer* buffer, RpcMessage& msg) {
                // 判断头文件是否接收完毕
                if (buffer->readableBytes() < HEADER_LEN) {
                    return false;
                }

                const char* header_ptr = buffer->peek();
                // 校验魔数
                if (header_ptr[0] != 'M' || header_ptr[1] != 'X') {
                    LOG_ERROR << "RpcCodec::decode - Invalid Magic Number! Connection might be compromised.";
                    buffer->retrieveAll();
                    return false;
                }

                // 提取转换body长度
                uint32_t b_len;
                std::memcpy(&b_len, header_ptr + 12, sizeof(uint32_t));
                msg.body_len = be32toh(b_len);

                // 防止body粘包
                if (buffer->readableBytes() < msg.body_len + HEADER_LEN) {
                    return false;
                }

                // 数据就绪，结构化数据
                msg.version = static_cast<uint8_t>(header_ptr[2]);
                msg.msg_type = static_cast<RpcMessageType>(header_ptr[3]);

                // 提取RequestId
                uint64_t r_id;
                std::memcpy(&r_id, header_ptr + 4, sizeof(uint64_t));
                msg.request_id = be64toh(r_id);

                // 消耗数据
                buffer->retrieve(HEADER_LEN);
                if (msg.body_len > 0) {
                    msg.body = buffer->retrieveAsString(msg.body_len);
                }

                return true;
            }

            static void encode(const RpcMessage& msg, ByteBuffer* buffer) {
                if (!msg.body.empty()) {
                    buffer->append(msg.body);
                }

                // 组装头
                char header[HEADER_LEN] = {0};
                header[0] = 'M';
                header[1] = 'X';
                header[2] = static_cast<char>(msg.version);
                header[3] = static_cast<char>(msg.msg_type);
                // 加入requestId
                uint64_t res_id = htobe64(msg.request_id);
                std::memcpy(header + 4, &res_id, sizeof(uint64_t));
                // 体长度
                uint32_t b_len = htobe32(msg.body.size());
                std::memcpy(header + 12, &b_len, sizeof(uint32_t));

                buffer->prepend(header, HEADER_LEN);
            }
        };

    }
}

#endif //MNSX_ACHILLESLINK_RPCCODEC_H