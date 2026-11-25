#include "ResourceStore.h"
#include <limits>
#include <sstream>

ResourceStore::ResourceStore()
{
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        fBuckets[i] = nullptr;
    }
}

ResourceStore::~ResourceStore()
{
    clear();
}

void ResourceStore::clear()
{
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        SinglyLinkedNode<ResourceEntry>* curr = fBuckets[i];
        while (curr != nullptr)
        {
            SinglyLinkedNode<ResourceEntry>* toDelete = curr;
            curr = curr->getNext();
            delete toDelete;
        }
        fBuckets[i] = nullptr;
    }
}

int ResourceStore::hashFunction(const std::string& aKey) const
{
    int sum = 0;
    for (size_t i = 0; i < aKey.length(); ++i)
    {
        sum += static_cast<unsigned char>(aKey[i]);
    }
    return sum % TABLE_SIZE;
}

void ResourceStore::set(const std::string& aKey, int aAmount)
{
    int index = hashFunction(aKey);
    SinglyLinkedNode<ResourceEntry>* curr = fBuckets[index];

    // Search chain for existing key
    while (curr != nullptr)
    {
        if (curr->getValue().key == aKey)
        {
            curr->setValue(ResourceEntry(aKey, aAmount < 0 ? 0 : aAmount));
            return;
        }
        curr = curr->getNext();
    }

    // Key not found: insert new SinglyLinkedNode at head of bucket chain
    SinglyLinkedNode<ResourceEntry>* newNode =
        new SinglyLinkedNode<ResourceEntry>(ResourceEntry(aKey, aAmount < 0 ? 0 : aAmount), fBuckets[index]);
    fBuckets[index] = newNode;
}

void ResourceStore::add(const std::string& aKey, int aAmount)
{
    if (aAmount == 0)
    {
        return;
    }
    int current = get(aKey);
    if (aAmount < 0)
    {
        long long updated = static_cast<long long>(current) + aAmount;
        set(aKey, updated < 0 ? 0 : static_cast<int>(updated));
        return;
    }
    if (aAmount > std::numeric_limits<int>::max() - current)
    {
        set(aKey, std::numeric_limits<int>::max());
        return;
    }
    set(aKey, current + aAmount);
}

bool ResourceStore::spend(const std::string& aKey, int aAmount)
{
    if (aAmount <= 0)
    {
        return false;
    }
    int current = get(aKey);
    if (current < aAmount)
    {
        return false;
    }
    set(aKey, current - aAmount);
    return true;
}

int ResourceStore::get(const std::string& aKey) const
{
    int index = hashFunction(aKey);
    SinglyLinkedNode<ResourceEntry>* curr = fBuckets[index];

    while (curr != nullptr)
    {
        if (curr->getValue().key == aKey)
        {
            return curr->getValue().amount;
        }
        curr = curr->getNext();
    }
    return 0;
}

bool ResourceStore::contains(const std::string& aKey) const
{
    int index = hashFunction(aKey);
    SinglyLinkedNode<ResourceEntry>* curr = fBuckets[index];

    while (curr != nullptr)
    {
        if (curr->getValue().key == aKey)
        {
            return true;
        }
        curr = curr->getNext();
    }
    return false;
}

std::string ResourceStore::getDebugLayout() const
{
    std::ostringstream oss;
    for (int i = 0; i < TABLE_SIZE; ++i)
    {
        oss << "Bucket[" << i << "]: ";
        SinglyLinkedNode<ResourceEntry>* curr = fBuckets[i];
        if (curr == nullptr)
        {
            oss << "(empty)";
        }
        while (curr != nullptr)
        {
            oss << "[" << curr->getValue().key << "=" << curr->getValue().amount << "]";
            if (curr->getNext() != nullptr)
            {
                oss << " -> ";
            }
            curr = curr->getNext();
        }
        oss << "\n";
    }
    return oss.str();
}
