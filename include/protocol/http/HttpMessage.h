/** 
 * @file HttpMessage.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/12
 * @description 封装Http消息请求和响应
 */
#ifndef MNSX_ACHILLESLINK_HTTPMESSAGE_H
#define MNSX_ACHILLESLINK_HTTPMESSAGE_H

#include <string>
#include <unordered_map>

namespace mnsx {
    namespace achilles {

        // HTTP请求的结构化数据
        struct HttpRequest {
            std::string method; // GET\POST
            std::string path;
            std::string version; // HTTP/1.1
            std::unordered_map<std::string, std::string> headers;
            std::string body;
        };

        // HTTP响应的结构化数据
        struct HttpResponse {
            std::string version = "HTTP/1.1";
            int statusCode = 200;
            std::string statusMessage = "OK";
            std::unordered_map<std::string, std::string> headers;
            std::string body;
        };

    }
}

#endif //MNSX_ACHILLESLINK_HTTPMESSAGE_H