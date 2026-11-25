#pragma once
#include <stdexcept>
#include "DoublyLinkedNode.h"
#include "DoublyLinkedNodeIterator.h"
template<class T>
class List
{
private:
    typedef DoublyLinkedNode<T> Node;
    Node* first; // Pointer to the front/head node of the list
    Node* last; // Pointer to the back/tail node of the list
    int count; // Number of elements currently in the list

public:
    typedef DoublyLinkedNodeIterator<T> Iterator;

    // Default constructor: initializes an empty list pointing to NIL sentinel
    List() : first(&(Node::NIL)), last(&(Node::NIL)), count(0) {}

    // Copy constructor (Deep Copy for safe value semantics)
    List(const List<T>& other) : first(&(Node::NIL)), last(&(Node::NIL)), count(0)
    {
        for (Iterator it = other.getIteratorFront(); it != it.end(); ++it)
        {
            pushBack(*it);
        }
    }

    // Copy assignment operator
    List<T>& operator=(const List<T>& other)
    {
        if (this != &other)
        {
            clear();
            for (Iterator it = other.getIteratorFront(); it != it.end(); ++it)
            {
                pushBack(*it);
            }
        }
        return *this;
    }

    // Destructor: releases all dynamically allocated nodes
    ~List()
    {
        clear();
    }

    // Remove all nodes from the list
    void clear()
    {
        Iterator it = getIteratorFront();
        while (it != it.end())
        {
            Node* toDelete = it.getCurrent();
            ++it;
            toDelete->remove();
        }
        first = &(Node::NIL);
        last = &(Node::NIL);
        count = 0;
    }

    bool isEmpty() const { return count == 0; }
    int size() const { return count; }

    // Push element to the Front (Head) of the list
    void pushFront(const T& value)
    {
        Node* newNode = new Node(value);
        if (first != &(Node::NIL))
        {
            first->prepend(newNode);
        }
        else
        {
            last = newNode; // List was empty, so newNode is also the last node
        }
        first = newNode;
        ++count;
    }

    // Push element to the Back (Tail) of the list
    void pushBack(const T& value)
    {
        Node* newNode = new Node(value);
        if (last != &(Node::NIL))
        {
            last->append(newNode);
        }
        else
        {
            first = newNode; // List was empty, so newNode is also the first node
        }
        last = newNode;
        ++count;
    }

    // Lab 6 compatibility alias for pushBack
    void push(const T& value)
    {
        pushBack(value);
    }

    // Pop and return element from the Front (Head) of the list
    T popFront()
    {
        if (isEmpty())
        {
            throw std::underflow_error("List is empty!");
        }

        Node* toDelete = first;
        T val = toDelete->getValue();
        first = first->getNext();

        if (first == &(Node::NIL))
        {
            last = &(Node::NIL);
        }

        toDelete->remove();
        --count;
        return val;
    }

    // Pop and return element from the Back (Tail) of the list
    T popBack()
    {
        if (isEmpty())
        {
            throw std::underflow_error("List is empty!");
        }

        Node* toDelete = last;
        T val = toDelete->getValue();
        last = last->getPrevious();

        if (last == &(Node::NIL))
        {
            first = &(Node::NIL);
        }

        toDelete->remove();
        --count;
        return val;
    }

    // Peek at the front or back element without removing
    const T& front() const
    {
        if (isEmpty())
            throw std::underflow_error("List is empty!");
        return first->getValueRef();
    }

    const T& back() const
    {
        if (isEmpty())
            throw std::underflow_error("List is empty!");
        return last->getValueRef();
    }

    // Lab 7 Part C requirement: Random-access [] operator by index
    const T& operator[](int index) const
    {
        if (index < 0 || index >= count)
        {
            throw std::out_of_range("List index out of bounds!");
        }
        Node* curr = first;
        for (int i = 0; i < index; ++i)
        {
            curr = curr->getNext();
        }
        return curr->getValueRef();
    }

    // Lab 7 Part C requirement: Remove a target element using its 0-based index
    T removeAt(int index)
    {
        if (index < 0 || index >= count)
        {
            throw std::out_of_range("List index out of bounds!");
        }
        if (index == 0)
        {
            return popFront();
        }
        if (index == count - 1)
        {
            return popBack();
        }

        Node* curr = first;
        for (int i = 0; i < index; ++i)
        {
            curr = curr->getNext();
        }
        T val = curr->getValue();
        curr->remove();
        --count;
        return val;
    }

    // Forward iterator starting from the Front (Head)
    Iterator getIteratorFront() const
    {
        return Iterator(first);
    }

    // Reverse iterator starting from the Back (Tail) - matches Lab 6 getIterator()
    Iterator getIterator() const
    {
        return Iterator(last);
    }
};