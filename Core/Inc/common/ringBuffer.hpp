/** @file ringBuffer.hpp
 *  @brief Fixed-size FIFO ring buffer.
 *  @details Lightweight queue used by planner/control pipeline.
 */

#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include "Common/systemError.hpp"

namespace Planner
{

/**
 * @brief Ring buffer template.
 * @tparam T Element type.
 * @tparam size Buffer capacity.
 * @details Single-producer/single-consumer style FIFO storage.
 */
template <typename T, size_t size> class ringBuffer
{
  public:
        /**
         * @brief Push item to buffer.
         * @param item Element to enqueue.
         * @return ErrorCode::Ok or ErrorCode::System_QueueFull.
         */
    ErrorCode push(const T& item)
    {
        size_t next_head = (head + 1) % size;

        if (next_head == tail)
        {
            return ErrorCode::System_QueueFull;
        }

        buffer[head] = item;
        head = next_head;

        return ErrorCode::Ok;
    }

    /**
     * @brief Pop oldest item from buffer.
     * @param item Output element.
     * @return ErrorCode::Ok or ErrorCode::System_QueueEmpty.
     */
    ErrorCode pop(T& item)
    {
        if (head == tail)
        {
            return ErrorCode::System_QueueEmpty;
        }

        item = buffer[tail];
        tail = (tail + 1) % size;

        return ErrorCode::Ok;
    }

    /**
     * @brief Get pointer to queued item by offset.
     * @param offset Offset from current tail.
     * @return Pointer to item or nullptr when out of range.
     */
    T* peek(uint8_t offset)
    {
        size_t item_count = (head >= tail) ? (head - tail) : (size - (tail - head));

        if (offset >= item_count)
        {
            return nullptr;
        }

        size_t index = (tail + offset) % size;
        return &buffer[index];
    }

    /** @brief Check whether buffer is empty. */
    bool isEmpty() const
    {
        return head == tail;
    }

    /** @brief Check whether buffer is full. */
    bool isFull() const
    {
        return (head + 1) % size == tail;
    }

  private:
    /// Ring storage.
    std::array<T, size> buffer;

    /// Tail index.
    volatile size_t tail = 0;

    /// Head index.
    volatile size_t head = 0;
};

} // namespace Planner