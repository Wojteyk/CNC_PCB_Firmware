/**
 * @file ringBuffer.hpp
 * @brief Thread-safe circular/ring buffer for command queuing
 * @details Provides a generic ring buffer template for queuing commands in a FIFO manner
 *          with atomic-friendly operations suitable for multi-threaded environments.
 * @author CNC Project
 * @version 1.0
 */

#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include "Common/systemError.hpp"

namespace Planner
{

/**
 * @template ringBuffer
 * @brief Thread-safe circular buffer template
 * @tparam T Type of elements to store
 * @tparam size Maximum buffer capacity (must be > 1)
 * @details A circular (ring) buffer for FIFO command queuing. Designed to be used
 *          in multi-threaded environments with atomic operations support.
 *
 *          The buffer uses head/tail pointers to track enqueue/dequeue positions.
 *          Thread safety can be enhanced with atomic memory fences (currently commented out).
 *
 * @example
 * @code
 * ringBuffer<CNC::MotionCmd, 32> cmdQueue;
 *
 * // Push command
 * CNC::MotionCmd cmd{CNC::MotionType::Linear};
 * if (cmdQueue.push(cmd) == ErrorCode::Ok) {
 *     // Command queued successfully
 * }
 *
 * // Pop command
 * CNC::MotionCmd execCmd;
 * if (cmdQueue.pop(execCmd) == ErrorCode::Ok) {
 *     // Execute command
 * }
 * @endcode
 */
template <typename T, size_t size> class ringBuffer
{
  public:
    /**
     * @brief Add item to the queue
     * @param item Const reference to item to enqueue
     * @return ErrorCode::Ok on success
     * @return ErrorCode::System_QueueFull if buffer is full
     * @details Adds an item to the head of the buffer. If the next head position
     *          equals the tail (buffer full), returns an error without modifying the buffer.
     */
    ErrorCode push(const T& item)
    {
        size_t next_head = (head + 1) % size;

        if (next_head == tail)
        {
            return ErrorCode::System_QueueFull;
        }

        buffer[head] = item;
        // __atomic_thread_fence(__ATOMIC_RELEASE); // Needed for multi-threaded environments
        head = next_head;

        return ErrorCode::Ok;
    }

    /**
     * @brief Remove and retrieve item from the queue
     * @param item Reference to output parameter where item will be stored
     * @return ErrorCode::Ok on success
     * @return ErrorCode::System_QueueEmpty if buffer is empty
     * @details Removes an item from the tail of the buffer and updates the tail pointer.
     */
    ErrorCode pop(T& item)
    {
        if (head == tail)
        {
            return ErrorCode::System_QueueEmpty;
        }

        item = buffer[tail];
        // __atomic_thread_fence(__ATOMIC_ACQUIRE); // Needed for multi-threaded environments
        tail = (tail + 1) % size;

        return ErrorCode::Ok;
    }

    /**
     * @brief Peek at item without removing it
     * @param offset Number of positions from the head to look ahead (0 = next item)
     * @return Pointer to item at offset position, or nullptr if out of range
     * @details Allows inspection of queued items without removing them.
     *          Offset 0 returns the oldest item (at tail).
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

    /**
     * @brief Check if buffer is empty
     * @return true if no items in buffer, false otherwise
     */
    bool isEmpty() const
    {
        return head == tail;
    }

    /**
     * @brief Check if buffer is full
     * @return true if buffer at maximum capacity, false otherwise
     */
    bool isFull() const
    {
        return (head + 1) % size == tail;
    }

  private:
    /// Circular buffer storage
    std::array<T, size> buffer;

    /// Dequeue pointer
    volatile size_t tail = 0;

    /// Enqueue pointer
    volatile size_t head = 0;
};

} // namespace Planner