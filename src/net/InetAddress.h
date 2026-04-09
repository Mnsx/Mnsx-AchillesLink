/** 
 * @file InetAddress.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装底层网络地址，提供网络字节序转换和地址解析功能
 */
#ifndef MNSX_ACHILLESLINK_INETADDRESS_H
#define MNSX_ACHILLESLINK_INETADDRESS_H

#include <string>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace mnsx {
    namespace achilles {

        class InetAddress {
        public:
            /**
             * @brief 构造函数
             * @param port
             * @param ip 默认使用 "0.0.0.0"
             */
            explicit InetAddress(uint16_t port, std::string ip = "0.0.0.0");

            /**
             * @brief 构造函数
             * @param addr
             */
            explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

            /**
             * @brief 将网络字节序整数IP转换为字符串
             * @return
             */
            std::string toIp() const;

            /**
             * @brief 将网络字节序整数端口号转换为无符号整型
             * @return
             */
            uint16_t toIpPort() const;

            /**
             * @brief Getter
             * @return
             */
            const sockaddr* getSockAddr() const {
                return reinterpret_cast<const sockaddr*>(&addr_);
            }

            /**
             * @brief setter
             * @param addr
             */
            void setSockAddr(const sockaddr_in &addr) {
                this->addr_ = addr;
            }
        private:
            sockaddr_in addr_{}; // 存储端口和IP的结构体
        };
    }
}

#endif //MNSX_ACHILLESLINK_INETADDRESS_H