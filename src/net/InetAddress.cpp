/** 
 * @file InetAddress.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/8
 * @description 封装底层网络地址，提供网络字节序转换和地址解析功能
 */
#include "InetAddress.h"
#include "Logger.h"

#include <arpa/inet.h>

using namespace mnsx::achilles;

InetAddress::InetAddress(uint16_t port, const std::string &ip) {
    // 设置协议族
    addr_.sin_family = AF_INET; // AF_INET表示IP协议族
    // 设置端口
    addr_.sin_port = htons(port);
    // 设置IP地址，防止错误格式设置默认IP，1成功，0不合法，-1协议族不支持
    if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) != 1) {
        LOG_ERROR << "Invalid IP address format: '" << ip
                  << "'. Falling back to INADDR_ANY (0.0.0.0).";
        // 错误输入将IP设置为INADDR_ANY
        addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    }
}

std::string InetAddress::getIp() const {
    // 使用字符数组存放IP地址
    char buf[64] = {0};

    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));

    return buf;
}

uint16_t InetAddress::getPort() const {
    return ntohs(addr_.sin_port);
}