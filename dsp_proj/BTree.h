#pragma once
#include <stdexcept>
#include <string>

template<class T>
class TreeVisitor
{
public:
    virtual ~TreeVisitor() {}
    virtual void preVisit(const T& /*aKey*/) const {}
    virtual void inVisit(const T& /*aKey*/) const {}
    virtual void postVisit(const T& /*aKey*/) const {}
};

template<class T>
class BTree
{
private:
    T fKey;  // Stored value inside this tree node (owned by value for safe runtime use)
    BTree<T>* fLeft; // Left child branch
    BTree<T>* fRight; // Right child branch

    // Private default constructor used exclusively to initialize BTree<T>::NIL
    BTree() : fKey(T()), fLeft(&NIL), fRight(&NIL)
    {
    }

public:
    static BTree<T> NIL; // Sentinel representing an empty tree node

    // Public constructor: initializes node with key and sets children to NIL
    BTree(const T& aKey) : fKey(aKey), fLeft(&NIL), fRight(&NIL) {}

    // Destructor: recursively deletes attached non-NIL child subtrees
    ~BTree()
    {
        if (fLeft != &NIL)
        {
            delete fLeft;
        }
        if (fRight != &NIL)
        {
            delete fRight;
        }
    }

    // Check if this node is the NIL sentinel
    bool isEmpty() const
    {
        return this == &NIL;
    }

    // Retrieve stored key
    const T& key() const
    {
        if (isEmpty())
        {
            throw std::domain_error("Empty node!");
        }
        return fKey;
    }

    // Mutable access to key (e.g., unlocking a node in the Tech Tree)
    T& keyMutable()
    {
        if (isEmpty())
        {
            throw std::domain_error("Empty node!");
        }
        return fKey;
    }

    // Access left and right subtrees
    BTree& left() const
    {
        if (isEmpty())
        {
            throw std::domain_error("Empty node!");
        }
        return *fLeft;
    }

    BTree& right() const
    {
        if (isEmpty())
        {
            throw std::domain_error("Empty node!");
        }
        return *fRight;
    }

    // Attach left child node
    void attachLeft(BTree<T>* aBTree)
    {
        if (isEmpty())
        {
            throw std::domain_error("Empty BTree");
        }
        if (fLeft != &NIL)
        {
            throw std::domain_error("Non-empty sub tree");
        }
        fLeft = aBTree;
    }

    // Attach right child node
    void attachRight(BTree<T>* aBTree)
    {
        if (isEmpty())
        {
            throw std::domain_error("Empty BTree");
        }
        if (fRight != &NIL)
        {
            throw std::domain_error("Non-empty sub tree");
        }
        fRight = aBTree;
    }

    // Detach left child node and return pointer
    BTree* detachLeft()
    {
        if (isEmpty())
        {
            throw std::domain_error("Empty BTree");
        }
        BTree<T>& Result = *fLeft;
        fLeft = &NIL;
        return &Result;
    }

    // Detach right child node and return pointer
    BTree* detachRight()
    {
        if (isEmpty())
        {
            throw std::domain_error("Empty BTree");
        }
        BTree<T>& Result = *fRight;
        fRight = &NIL;
        return &Result;
    }

    // Depth-First Traversal using TreeVisitor 
    void traverseDepthFirst(const TreeVisitor<T>& aVisitor) const
    {
        if (!isEmpty())
        {
            aVisitor.preVisit(key());
            left().traverseDepthFirst(aVisitor);
            aVisitor.inVisit(key());
            right().traverseDepthFirst(aVisitor);
            aVisitor.postVisit(key());
        }
    }
};

// Sentinel definition
template<class T>
BTree<T> BTree<T>::NIL;