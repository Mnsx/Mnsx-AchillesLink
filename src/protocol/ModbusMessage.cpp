/** 
 * @file ModbusMessage.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 
 */
#include "ModbusMessage.h"

namespace mnsx {
    namespace achilles {
        bool ModbusMessage::parseFromRaw(const uint8_t *data, size_t len, size_t &parsedLength) {
            if (len < sizeof(MbapHeader)) {
                return false;
            }

            // 将数据强制转换为结构体
            const MbapHeader *header = reinterpret_cast<const MbapHeader*>(data);

            // 校验协议位标识符
            if (ntohs(header->protocol_id)) {
                return false;
            }

            // 计算总长度
            uint16_t bodyLength = ntohs(header->length);
            size_t expectedLength = 6 + bodyLength; // 6是MBAP消息头

            // 校验数据数量是否完整
            if (len < expectedLength) {
                return false;
            }

            // 提取数据
            this->transaction_id_ = ntohs(header->transaction_id);
            this->unit_id_ = header->unit_id;
            this->function_code_ = data[7]; // MBAP消息头后一位就是功能码

            // 提取内容
            size_t payloadLength = bodyLength - 1; // 减去功能码
            if (payloadLength > 0) {
                this->payload_.assign(data + 8, data + 8 + payloadLength); // 8 = 功能码 + MBAP消息头长度7
            } else {
                this->payload_.clear();
            }

            parsedLength = expectedLength;
            return true;
        }
    }
} // mnsx