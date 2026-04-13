/** 
 * @file ByteBuffer.h
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/11
 * @description 自实现缓冲区，提供自动扩容、数据平移以及非阻塞I/O分散读功能
 */
#ifndef MNSX_ACHILLESLINK_BYTEBUFFER_H
#define MNSX_ACHILLESLINK_BYTEBUFFER_H

#include "Logger.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <unistd.h>

namespace mnsx {
    namespace achilles {

        class ByteBuffer {
        public:
            constexpr static const size_t PREPEND = 16; // 初始预留头部空间，方便协议头打包
            constexpr static const size_t INITIAL_SIZE = 1024; // 初始缓冲区大小 1KB

            /**
             * @brief 构造函数
             * @param initial_size
             */
            explicit ByteBuffer(size_t initial_size = INITIAL_SIZE) :
                buffer_(initial_size + PREPEND),
                reader_index_(PREPEND),
                writer_index_(PREPEND) {

                LOG_DEBUG << "ByteBuffer initialized with capacity: " << initial_size;
            }
            /**
             * @brief 析构函数
             */
            ~ByteBuffer() = default;

            /**
             * @brief delete
             */
            ByteBuffer(const ByteBuffer &) = delete;
            /**
             * @brief delete
             */
            ByteBuffer &operator=(const ByteBuffer &) = delete;

            /**
             * @brief 查看可读数据得长度
             * @return
             */
            size_t readableBytes() const { return writer_index_ - reader_index_; }
            /**
             * @brief 查看科协数据得长度
             * @return
             */
            size_t writableBytes() const { return buffer_.size() - writer_index_; }
            /**
             * @brief 返回消息头占有得长度
             * @return
             */
            size_t prependBytes() const { return reader_index_;}

            /**
             * @brief 获取开始读取位置得指针
             * @return
             */
            const char* peek() const { return begin() + reader_index_; }

            /**
             * @brief 消费指定长度得数据
             * @param len
             */
            void retrieve(size_t len) {
                // 希望获取得长度超过了可读范围
                if (len > readableBytes()) {
                    LOG_ERROR << "CharonBuffer::retrieve() - Invalid length! "
                              << "Request: " << len << ", Available: " << readableBytes();

                    return;
                }

                if (len < readableBytes()) {
                    // 正常情况，消费了一部分数据指针往后移
                    reader_index_ += len;
                } else {
                    // 刚好可以消费所有数据，两个指针都回到PREPEND
                    retrieveAll();
                }
            }
            /**
             * @brief 消费所有数据
             */
            void retrieveAll() {
                reader_index_ = PREPEND;
                writer_index_ = PREPEND;
            }

            std::string retrieveAsString(size_t len) {
                if (len > readableBytes()) {
                    LOG_ERROR << "ByteBuffer::retrieveAsString() - Invalid length!";
                    return "";
                }

                // 将数据拷贝到std::string
                std::string res(peek(), len);

                // 消费
                retrieve(len);

                return res;
            }
            /**
             * @brief 消费所有数据，将数据转换为string
             * @return
             */
            std::string retrieveAllAsString() {
                return retrieveAsString(readableBytes());
            }

            /**
             * @brief 将数据添加到缓冲区
             * @param data
             * @param len
             */
            void append(const char* data, size_t len) {
                // 判断长度是否能够写入
                ensureWritableBytes(len);
                // 通过copy将数据转移
                std::copy(data, data + len, beginWrite());
                writer_index_ += len;
            }
            /**
             * @brief 针对string得类型，转换为char数组，抛给append处理
             * @param data
             */
            void append(const std::string& data) {
                append(data.data(), data.size());
            }
            /**
             * @brief
             * @param str
             * @param header_len
             */
            void prepend(char * data, size_t len) {
                if (len > prependBytes()) {
                    return;
                }

                reader_index_ -= len;

                const char* src = static_cast<const char*>(data);
                std::memcpy(begin() + reader_index_, src, len);
            }

            /**
             * @brief 根据提供得size判断，当前可写空间是否能够承载len大小的数据
             * @param len
             */
            void ensureWritableBytes(size_t len) {
                // 如果可写空间小于需求，那么扩容
                if (writableBytes() < len) {
                    makeSpace(len);
                }

                // 扩容后再判断，如果还是失败，那么说明扩容过程中产生问题
                if (writableBytes() < len) {
                    LOG_ERROR << "CharonBuffer::ensureWritableBytes() - Failed to allocate enough space! "
                              << "Required: " << len << ", Available: " << writableBytes();
                }
            }

            /**
             * @brief 返回可以开始写的地址
             * @return
             */
            char* beginWrite() { return begin() + writer_index_; }
            /**
             * @brief 返回可以开始写的地址
             * @return
             */
            const char* beginWrite() const { return begin() + writer_index_; }

            /**
             * @brief 系统I/O操作
             * @param fd
             * @param saveErrno
             * @return
             */
            ssize_t readFd(int fd, int* saveErrno);

        private:
            std::vector<char> buffer_;
            size_t reader_index_;
            size_t writer_index_;

            /**
             * @brief 获取缓冲区开头得指针
             * @return
             */
            char* begin() { return &*buffer_.begin(); }
            /**
             * @brief 获取缓冲区开头得指针
             * @return
             */
            const char* begin() const { return &*buffer_.begin(); }

            /**
             * @brief 扩容机制，前方使用过得区域+后方空闲区域<所需长度，resize扩容，如果足够，但是都在前面，将数据平移到起始位置
             * @param len
             */
            void makeSpace(size_t len);
        };

    }
}

#endif //MNSX_ACHILLESLINK_BYTEBUFFER_H