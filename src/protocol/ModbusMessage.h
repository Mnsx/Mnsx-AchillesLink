/** 
 * @file ModbusMessage.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description Modbus协议报文定义，负责处理底层字节流序列化与反序列化解析
 */
#ifndef MNSX_ACHILLESLINK_MODBUSMESSAGE_H
#define MNSX_ACHILLESLINK_MODBUSMESSAGE_H

#include <cstdint>
#include <vector>
#include <arpa/inet.h>

namespace mnsx {
    namespace achilles {

        // 强制编译器按 1 字节对齐，防止内存填充导致解析错位
#pragma pack(push, 1)
        struct MbapHeader {
            uint16_t transaction_id; // 事务处理标识符
            uint16_t protocol_id; // 协议标识符
            uint16_t length; // 长度
            uint8_t unit_id; // 单元标识
        };
#pragma pack(pop)

        class ModbusMessage {
        public:
            /**
             * @brief 构造函数
             */
            ModbusMessage() = default;

            /**
             * @brief 从原始字节流中解析出一个完整的Modbus消息
             * @param data 接收到的原始字节流指针
             * @param len 当前缓冲区的字节数
             * @param parsedLength 返回，如果成功解析，返回该报文消耗的总字节数
             * @return true 表示成功解析，false表示数据不够或格式错误
             */
            bool parseFromRaw(const uint8_t* data, size_t len, size_t& parsedLength);

            /**
             * @brief Getter
             * @return
             */
            uint16_t getTransactionId() const {
                return this->transaction_id_;
            }
            /**
             * @brief Getter
             * @return
             */
            uint8_t getUnitId() const {
                return this->unit_id_;
            }
            /**
             * @brief Getter
             * @return
             */
            uint8_t getFunctionCode() const {
                return this->function_code_;
            }
            /**
             * @brief Getter
             * @return
             */
            const std::vector<uint8_t>& getPayload() const {
                return this->payload_;
            }

        private:
            uint16_t transaction_id_ = 0; // 事务标识
            uint8_t unit_id_ = 0; // 单元标识
            uint8_t function_code_ = 0; // 功能码
            std::vector<uint8_t> payload_; // 数据部分
        };
    }
} // mnsx

#endif //MNSX_ACHILLESLINK_MODBUSMESSAGE_H