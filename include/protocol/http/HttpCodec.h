/** 
 * @file HttpCodec.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/12
 * @description HTTP协议编码解码器
 */
#ifndef MNSX_ACHILLESLINK_HTTPCODEC_H
#define MNSX_ACHILLESLINK_HTTPCODEC_H

#include "HttpMessage.h"
#include "../src/net/ByteBuffer.h"

namespace mnsx {
    namespace achilles {

        class HttpCodec {
        public:
            static bool decode(ByteBuffer* buffer, HttpRequest& request) {
                const char CRLF[] = "\r\n";
                const char CRLFCRLF[] = "\r\n\r\n";

                const char* begin = buffer->peek();
                const char* end = begin + buffer->readableBytes();
                const char* header_end = std::search(begin, end, CRLFCRLF, CRLFCRLF + 4);

                if (header_end == end) {
                    return false;
                }

                size_t header_length = header_end - begin + 4;

                std::string header_str(begin, header_length - 4);

                std::stringstream stream(header_str);
                std::string line;

                if (std::getline(stream, line) && !line.empty()) {
                    if (line.back() == '\r') {
                        line.pop_back();
                    }
                    std::stringstream line_stream(line);
                    line_stream >> request.method >> request.path >> request.version;
                }

                int content_length = 0;
                while (std::getline(stream, line) && !line.empty()) {
                    if (line.back() == '\r') {
                        line.pop_back();
                    }
                    size_t colon_pos = line.find(':');
                    if (colon_pos != std::string::npos) {
                        std::string key = line.substr(0, colon_pos);
                        size_t value_start = line.find_first_not_of(' ', colon_pos + 1);
                        std::string value = line.substr(value_start);

                        request.headers[key] = value;

                        if (key == "Content-Length" | key == "content-length") {
                            content_length = std::stoi(value);
                        }
                    }
                }

                size_t total_length = header_length + content_length;
                if (buffer->readableBytes() < total_length) {
                    request = HttpRequest();
                    return false; // 半包
                }

                buffer->retrieve(header_length);
                if (content_length > 0) {
                    request.body = buffer->retrieveAsString(content_length);
                }

                return true;
            }

            static void encode(const HttpResponse& resp, ByteBuffer* out_buf) {
                std::string header_data = resp.version + " " +
                                          std::to_string(resp.statusCode) + " " +
                                          resp.statusMessage + "\r\n";

                auto headers = resp.headers;
                headers["Content-Length"] = std::to_string(resp.body.size());
                headers["Server"] = "AchillesLink/1.0";

                for (const auto& pair : headers) {
                    header_data += pair.first + ": " + pair.second + "\r\n";
                }

                header_data += "\r\n";

                out_buf->append(header_data);
                if (!resp.body.empty()) {
                    out_buf->append(resp.body);
                }
            }
        };

    }
}

#endif //MNSX_ACHILLESLINK_HTTPCODEC_H