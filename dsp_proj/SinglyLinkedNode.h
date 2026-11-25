#pragma once
template<class DataType>
class SinglyLinkedNode
{
public:
    typedef SinglyLinkedNode<DataType> Node;

private:
    DataType fValue; // Data stored in this node
    Node* fNext; // Pointer to the next node in the singly linked list

public:
    // Constructor initializes node value and optional next pointer
    SinglyLinkedNode(const DataType& aValue, Node* aNext = nullptr)
        : fValue(aValue), fNext(aNext)
    {
    }

    // Attach a node directly after this node
    void append(Node* aNewNode)
    {
        if (aNewNode != nullptr)
        {
            aNewNode->fNext = fNext;
            fNext = aNewNode;
        }
    }

    // Accessor for stored value (read-only)
    const DataType& getValue() const
    {
        return fValue;
    }

    // Mutator for updating stored value (e.g., updating resource count in Hash Table)
    void setValue(const DataType& aValue)
    {
        fValue = aValue;
    }

    // Accessor for the next node pointer
    Node* getNext() const
    {
        return fNext;
    }

    // Mutator for relinking the next pointer when removing a node
    void setNext(Node* aNext)
    {
        fNext = aNext;
    }
};