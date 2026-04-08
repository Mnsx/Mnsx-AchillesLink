/** 
 * @file InetAddress.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 
 */
#include "InetAddress.h"

namespace mnsx {
    namespace achilles {

        InetAddress::InetAddress(uint16_t port, std::string ip) {
            // 指定协议类型 IPv4
            addr_.sin_family = AF_INET;
            // 端口号
            addr_.sin_port = htons(port);
            // 设置IP，防止错误输入
            if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) != 1) {
                addr_.sin_addr.s_addr = htonl(INADDR_ANY);
            }
        }

        std::string InetAddress::toIp() const {
            char buf[64] = {0};

            // 将网络字节的整数转换为字符串
            inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));

            return {buf};
        }

        uint16_t InetAddress::toIpPort() const {
            return ntohs(addr_.sin_port);
        }
    }
}