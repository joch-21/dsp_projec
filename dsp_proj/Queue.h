#pragma once
#include <stdexcept>
#include "List.h"
template<class T>
class Queue
{
private:
    List<T> fElements; // Underlying Doubly-Linked List ADT container
    int fMaxLimit; // Maximum capacity limit 

public:
    // Constructor with default max element limit
    Queue(int aMaxLimit = 50) : fMaxLimit(aMaxLimit)
    {
        if (aMaxLimit <= 0)
        {
            throw std::invalid_argument("Illegal queue size.");
        }
    }

    bool isEmpty() const
    {
        return fElements.isEmpty();
    }

    bool isFull() const
    {
        return fElements.size() >= fMaxLimit;
    }

    int size() const
    {
        return fElements.size();
    }

    // Enqueue item at the back of the queue
    void enqueue(const T& aItem)
    {
        if (isFull())
        {
            throw std::overflow_error("Queue overflow: max limit reached!");
        }
        fElements.pushBack(aItem);
    }

    // Dequeue and return item from the front of the queue (FIFO)
    T dequeue()
    {
        if (isEmpty())
        {
            throw std::underflow_error("Queue underflow: queue is empty!");
        }
        return fElements.popFront();
    }

    // Inspect the front item without removing it
    const T& front() const
    {
        if (isEmpty())
        {
            throw std::underflow_error("Queue is empty!");
        }
        return fElements.front();
    }
};