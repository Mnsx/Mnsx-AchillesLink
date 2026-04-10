/** 
 * @file InetAddress.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装底层网络地址，提供网络字节序转换和地址解析功能
 */
#ifndef MNSX_ACHILLESLINK_INETADDRESS_H
#define MNSX_ACHILLESLINK_INETADDRESS_H

#include <string>
#include <netinet/in.h>

namespace mnsx {
    namespace achilles {

        class InetAddress {
        public:
            /**
             * @brief 构造函数
             * @param port
             * @param ip
             */
            explicit InetAddress(uint16_t port, const std::string& ip = "0.0.0.0");
            /**
             * @brief 构造函数，将sockaddr_in转换为InetAddress
             * @param addr 使用常量左值引用作为形参，这样可以避免按值传递，且const可以保证可读性，还能同时接收左值和右值
             */
            explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr) {}
            /**
             * @brief 析构函数
             */
            ~InetAddress() = default;

            /**
             * @brief 获取存储在addr_中的IP地址
             * @return 因为内部使用临时变量存放IP地址，所以只能通过拷贝，不能使用引用
             */
            std::string getIp() const;
            /**
             * @brief
             * @return 获取存储在addr_中的端口号
             */
            uint16_t getPort() const;

            /**
             * @brief 成员变量addr_的Getter，方便后续原生接口使用应该返回指针
             * @return
             */
            const struct sockaddr* getAddr() const {
                return reinterpret_cast<const struct sockaddr*>(&addr_);
            }

            /**
             * @brief 成员变量addr_的Setter
             * @param addr
             */
            void setAddr(const struct sockaddr_in& addr) { this->addr_ = addr; }

        private:
            struct sockaddr_in addr_{}; // 使用初始化列表赋初值，只保留结构体，其大小为16字节，在网络编程中，可以急速拷贝
        };

    }
}

#endif //MNSX_ACHILLESLINK_INETADDRESS_H