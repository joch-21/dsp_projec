#pragma once
template<class DataType>
class DoublyLinkedNode
{
public:
    typedef DoublyLinkedNode<DataType> Node;
    static Node NIL;  

private:
    DataType value;
    Node* next;
    Node* previous;

public: 
    DoublyLinkedNode()
    {
        value = DataType();
        next = &NIL;
        previous = &NIL;
    }

    // Parameterized constructor for creating a new data node
    DoublyLinkedNode(const DataType& aValue)
    {
        value = aValue;
        next = &NIL;
        previous = &NIL;
    }

    void prepend(Node* newNode)
    {
        if (newNode == nullptr || newNode == this || this == &NIL)
        {
            return;
        }

        newNode->next = this;

        newNode->previous = this->previous;
        this->previous->next = newNode;

        this->previous = newNode;
    }

    void append(Node* newNode)
    {
        if (newNode == nullptr || newNode == this || this == &NIL)
        {
            return;
        }

        newNode->previous = this;

        newNode->next = this->next;
        this->next->previous = newNode;

        this->next = newNode;
    }

    void remove()
    {
        if (this == &NIL)
        {
            return;
        }

        if (this->previous != &NIL)
        {
            this->previous->next = this->next;
        }

        if (this->next != &NIL)
        {
            this->next->previous = this->previous;
        }

        delete this;
    }

    // Accessors
    DataType getValue() const
    {
        return value;
    }

    const DataType& getValueRef() const
    {
        return value;
    }

    Node* getNext() const
    {
        return next;
    }

    Node* getPrevious() const
    {
        return previous;
    }
};

//sentinel NIL initialization 
template<class DataType>
DoublyLinkedNode<DataType> DoublyLinkedNode<DataType>::NIL;
