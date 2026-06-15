#include "ScanBuffer.h"

ScanBuffer::ScanBuffer(std::size_t capacity)
    : m_capacity(capacity)
{}

void ScanBuffer::push(ScanFrame frame) {
    std::unique_lock lock(m_mutex);
    if (m_buf.size() >= m_capacity)
        m_buf.pop_front();          // drop oldest
    m_buf.push_back(std::move(frame));
    lock.unlock();
    m_cv.notify_one();
}

std::optional<ScanFrame> ScanBuffer::latest() const {
    std::lock_guard lock(m_mutex);
    if (m_buf.empty()) return std::nullopt;
    return m_buf.back();
}

std::optional<ScanFrame> ScanBuffer::waitForNext(int timeout_ms) {
    std::unique_lock lock(m_mutex);
    std::size_t sizeBefore = m_buf.size();
    bool gotNew = m_cv.wait_for(lock,
        std::chrono::milliseconds(timeout_ms),
        [&]{ return m_buf.size() > sizeBefore; });
    if (!gotNew || m_buf.empty()) return std::nullopt;
    return m_buf.back();
}

std::vector<ScanFrame> ScanBuffer::drainAll() {
    std::lock_guard lock(m_mutex);
    std::vector<ScanFrame> out(m_buf.begin(), m_buf.end());
    m_buf.clear();
    return out;
}

std::size_t ScanBuffer::size() const {
    std::lock_guard lock(m_mutex);
    return m_buf.size();
}

void ScanBuffer::clear() {
    std::lock_guard lock(m_mutex);
    m_buf.clear();
}
