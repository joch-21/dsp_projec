#pragma once
#include <string>
#include "SinglyLinkedNode.h"
struct ResourceEntry
{
    std::string key;
    int amount;

    ResourceEntry() : key(""), amount(0) {}
    ResourceEntry(const std::string& aKey, int aAmount) : key(aKey), amount(aAmount) {}
};

class ResourceStore
{
private:
    static const int TABLE_SIZE = 7; // Prime bucket count to reduce collisions
    SinglyLinkedNode<ResourceEntry>* fBuckets[TABLE_SIZE];

    // Hash function: sums ASCII character values modulo TABLE_SIZE
    int hashFunction(const std::string& aKey) const;

public:
    ResourceStore();
    ~ResourceStore();

    // Disable accidental shallow copy (Rule of Three safety)
    ResourceStore(const ResourceStore&) = delete;
    ResourceStore& operator=(const ResourceStore&) = delete;

    // Clear all dynamically allocated SinglyLinkedNodes across all buckets
    void clear();

    // Set exact resource amount (inserts new node or updates existing key)
    void set(const std::string& aKey, int aAmount);

    // Add amount to a resource (creates entry if not present)
    void add(const std::string& aKey, int aAmount);

    // Attempt to spend amount; returns true if sufficient, false otherwise
    bool spend(const std::string& aKey, int aAmount);

    // Retrieve current amount of a resource (returns 0 if key not found)
    int get(const std::string& aKey) const;

    // Check whether a resource key exists in the hash table
    bool contains(const std::string& aKey) const;

    // Return a formatted string of buckets & chains for diagnostics/ConsoleUI
    std::string getDebugLayout() const;
};