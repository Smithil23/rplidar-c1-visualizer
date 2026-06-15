#pragma once
#include "ScanPoint.h"

#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>

// Thread-safe ring buffer that sits between the LidarDriver (producer)
// and the GUI/exporter (consumers).
//
// The producer calls push() from the lidar thread.
// The GUI calls latest() on the render timer (non-blocking).
// The exporter calls pop() to drain stored frames.
class ScanBuffer {
public:
    explicit ScanBuffer(std::size_t capacity = 64);

    // Called from the lidar thread: add a completed scan frame.
    // If the buffer is full the oldest frame is silently dropped.
    void push(ScanFrame frame);

    // Non-blocking: returns the most recent frame, or nullopt if empty.
    std::optional<ScanFrame> latest() const;

    // Blocking: waits up to timeout_ms milliseconds for a new frame.
    // Returns the frame, or nullopt on timeout.
    std::optional<ScanFrame> waitForNext(int timeout_ms = 200);

    // Drain all stored frames (for exporting a recording).
    std::vector<ScanFrame> drainAll();

    // Number of frames currently buffered.
    std::size_t size() const;

    void clear();

private:
    mutable std::mutex          m_mutex;
    std::condition_variable     m_cv;
    std::deque<ScanFrame>       m_buf;
    std::size_t                 m_capacity;
};
