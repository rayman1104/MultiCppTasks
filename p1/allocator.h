#include <stdexcept>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <memory>

enum class AllocErrorType {
    InvalidFree,
    NoMemory
};

class AllocError: std::runtime_error {
private:
    AllocErrorType type;

public:
    AllocError(AllocErrorType _type, std::string message):
            runtime_error(message),
            type(_type)
    {}

    AllocErrorType getType() const { return type; }
};

class Allocator;

class Storage {
    void *data;
    size_t size;

public:
    Storage(void *_data, size_t _size) : data(_data), size(_size) {}

    friend class Allocator;
    friend class Pointer;

    bool operator< (Storage &st) {
        return data < st.data;
    }
    bool operator== (const Storage &st) {
        return data == st.data;
    }
};

class Pointer {
    std::list<Storage>::iterator it;
    bool correct = true;

    Pointer(decltype(it) _it) : it(_it) {}

public:
    Pointer() : it(), correct(false) {}

    void *get() const {
        if (!correct)
            return nullptr;
        return it->data;
    }

    friend class Allocator;

    bool operator< (Pointer &p) {
        return it->data < p.it->data;
    }
    bool operator== (const Pointer &p) {
        return it->data == p.it->data;
    }
};

class Allocator {
    void *base;
    size_t capacity;
    size_t offset = 0;
    std::list<Storage> area = {};

    void *findSpace(size_t space);
    bool hasEnoughSpace(Storage &st, size_t space);

public:
    Allocator(void *_base, size_t _capacity) : base(_base), capacity(_capacity) {
        if(!base || !capacity) {
            throw AllocError(AllocErrorType::NoMemory, "No base");
        }
    }

    Pointer alloc(size_t N);

    void realloc(Pointer &p, size_t N);

    void free(Pointer &p);

    void defrag();

    friend class Pointer;

    std::string dump();
};
