#pragma once
#include "DoublyLinkedNode.h"
template<class DataType>
class DoublyLinkedNodeIterator
{
private:
    typedef DoublyLinkedNode<DataType> Node;
    Node* current;

public:
    // Construct iterator pointing at a starting node pointer
    DoublyLinkedNodeIterator(Node* start) : current(start) {}

    // Construct iterator from a node reference (used for Node::NIL in end())
    DoublyLinkedNodeIterator(Node& node) : current(&node) {}

    // Pre-increment (++it): move forward to next node
    DoublyLinkedNodeIterator& operator++()
    {
        current = current->getNext();
        return *this;
    }

    // Post-increment (it++): return copy of current state, then move forward
    DoublyLinkedNodeIterator operator++(int)
    {
        DoublyLinkedNodeIterator temp = *this;
        ++(*this);
        return temp;
    }

    // Pre-decrement (--it): move backward to previous node
    DoublyLinkedNodeIterator& operator--()
    {
        current = current->getPrevious();
        return *this;
    }

    // Post-decrement (it--): return copy of current state, then move backward
    DoublyLinkedNodeIterator operator--(int)
    {
        DoublyLinkedNodeIterator temp = *this;
        --(*this);
        return temp;
    }

    // Equality comparison
    bool operator==(const DoublyLinkedNodeIterator& other) const
    {
        return current == other.current;
    }

    // Inequality comparison
    bool operator!=(const DoublyLinkedNodeIterator& other) const
    {
        return current != other.current;
    }

    // Dereference operator: retrieve value stored in current node
    DataType operator*() const
    {
        return current->getValue();
    }

    // Direct pointer access to current node (used by List for safe node removal)
    Node* getCurrent() const
    {
        return current;
    }

    // Returns iterator at current starting position
    DoublyLinkedNodeIterator begin() const
    {
        return DoublyLinkedNodeIterator(current);
    }

    // Returns iterator positioned at the sentinel Node::NIL boundary
    DoublyLinkedNodeIterator end() const
    {
        return DoublyLinkedNodeIterator(Node::NIL);
    }
};