/** 
 * @file InetAddress.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装不同操作系统的socketaddr结构体
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
             * 根据IP地址和端口实现
             * @param port 端口
             * @param ip IP地址，默认为ANY
             */
            explicit InetAddress(uint16_t port, std::string ip = "0.0.0.0");

            /**
             * 根据socket结构体生成
             * @param addr socket结构体
             */
            explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {};

            /**
             * 获取IP地址
             * @return IP
             */
            std::string toIp() const;

            /**
             * 获取端口
             * @return 端口
             */
            uint16_t toIpPort() const;

            // Getter
            const sockaddr* getSockAddr() const {
                return reinterpret_cast<const sockaddr*>(&addr_);
            }

            // Setter
            void setSockAddr(const sockaddr_in &addr) {
                this->addr_ = addr;
            }
        private:
            sockaddr_in addr_{}; // 存储端口和IP的结构体
        };
    }
}

#endif //MNSX_ACHILLESLINK_INETADDRESS_H