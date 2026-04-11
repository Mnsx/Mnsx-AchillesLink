/** 
 * @file ByteBuffer.cpp
 * @author Mnsx_x <xx1527030652@gmail.com>
 * @date 2026/4/12
 */
#include "ByteBuffer.h"

using namespace mnsx::achilles;

void ByteBuffer::makeSpace(size_t len) {
    // 计算真正缺少得空间，目标长度-当前可写空间
    if (writableBytes() + prependBytes() < len + PREPEND) {
        // 后方可以写 + 前方废弃 < 扩容长度 + 预留长度，只能通过resize扩容
        // resize之后，reader和writer不需要修改索引位置，但是vector会重新分配空间
        buffer_.resize(writer_index_ + len);
        LOG_DEBUG << "ByteBuffer expanded to size: " << buffer_.size();
    } else {
        // 空间够用，平移数据
        LOG_DEBUG << "ByteBuffer compaction: moving " << readableBytes() << " bytes to front.";

        size_t readable = readableBytes();
        // 可写数据得位置，拷贝到开头
        std::copy(begin() + reader_index_, begin() + writer_index_, begin() + PREPEND);

        // 重置索引
        reader_index_ = PREPEND;
        writer_index_ = reader_index_ + readable;

        // 校验结果
        if (readableBytes() != readable) {
            LOG_ERROR << "ByteBuffer::makeSpace error after compaction!";
        }
    }
}

ssize_t ByteBuffer::readFd(int fd, int *saveErrno) {

    // 每次读取前，确保有1024字节可写，如果不够调用makeSpace自动整理或扩容
    size_t initial_writable = writableBytes();
    if (initial_writable < 1024) {
        ensureWritableBytes(1024);
    }

    // 读取数据
    ssize_t read_count = ::read(fd, beginWrite(), writableBytes());
    if (read_count < 0) {
        *saveErrno = errno;
        // 只有不是EAGAIN/EWOULDBLOCK才算异常
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR << "ByteBuffer::readFd failed, errno: " << errno;
        }
    } else if (read_count > 0) {
        // 读取成功，移动索引
        writer_index_ += read_count;
        LOG_DEBUG << "ByteBuffer read " << read_count << " bytes from fd: " << fd;
    }

    return read_count;
}
