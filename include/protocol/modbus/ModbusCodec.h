/** 
 * @file ModbusCodec.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/12
 * @description ModbusTcp协议编解码器
 */
#ifndef MNSX_ACHILLESLINK_MODBUSCODEC_H
#define MNSX_ACHILLESLINK_MODBUSCODEC_H

#include "../../../src/net/ByteBuffer.h"

#include <cstdint>

namespace mnsx {
    namespace achilles {

        class ModbusCodec {
        public:
            /**
             * @brief 解码
             * @param buf
             * @param out_frame
             * @return false 半包或者协议错误
             */
            static bool decode(ByteBuffer* buf, std::string& out_frame) {
                // 判断半包
                if (buf->readableBytes() < 7) {
                    return false; // 半包
                }
                // 获取元数据
                const char* header = buf->peek();
                // 校验Protocol ID
                uint16_t protocol_id = static_cast<uint16_t>(
                    static_cast<uint8_t>(header[2]) << 8 | static_cast<uint8_t>(header[3]));
                if (protocol_id != 0) {
                    LOG_ERROR << "Invalid Protocol ID";
                    buf->retrieveAll();
                    return false;
                }

                uint16_t length_field = static_cast<uint16_t>(
                    static_cast<uint8_t>(header[4]) << 8 | static_cast<uint8_t>(header[5]));
                size_t total_length = length_field + 6; // length_field中还有uint

                if (buf->readableBytes() < total_length) {
                    return false; // 半包
                }

                // 提取完整数据，回收buffer
                out_frame = buf->retrieveAsString(total_length);
                return true;
            }

            static void encode(uint16_t trans_id, uint8_t unit_id, const std::string& pdu, ByteBuffer* out_buf) {
                uint16_t len = static_cast<uint16_t>(pdu.size() + 1);

                char header[7];
                header[0] = static_cast<char>(trans_id >> 8);
                header[1] = static_cast<char>(trans_id & 0xFF);
                header[2] = 0;
                header[3] = 0;
                header[4] = static_cast<char>(len >> 8);
                header[5] = static_cast<char>(len & 0xFF);
                header[6] = static_cast<char>(unit_id);

                out_buf->append(header, 7);
                out_buf->append(pdu);
            }
        };

    }
}

#endif //MNSX_ACHILLESLINK_MODBUSCODEC_H