#pragma once
#include <stdexcept>
#include "List.h"

template<class T>
class Stack
{
private:
    List<T> fElements;   // Underlying Doubly-Linked List ADT container
public:
    Stack() = default;

    bool isEmpty() const
    {
        return fElements.isEmpty();
    }

    int size() const
    {
        return fElements.size();
    }

    // Push item onto the top of the stack (back of the list)
    void push(const T& aItem)
    {
        fElements.pushBack(aItem);
    }

    // Pop and return item from the top of the stack (LIFO)
    T pop()
    {
        if (isEmpty())
        {
            throw std::underflow_error("Stack underflow: stack is empty!");
        }
        return fElements.popBack();
    }

    // Inspect the top item without removing it
    const T& top() const
    {
        if (isEmpty())
        {
            throw std::underflow_error("Stack empty!");
        }
        return fElements.back();
    }
};
