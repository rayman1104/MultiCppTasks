#include "allocator.h"

void *Allocator::findSpace(size_t space) {
    std::list<Storage> area_copy = area;
    area_copy.sort();
    auto it = area_copy.begin();
    for (; std::next(it) != area_copy.end(); ++it) {
        if (it->data == nullptr)
            continue;
        auto tmp = (char *)(std::next(it)->data) - (char *)(it->data);
        if (tmp - it->size >= space)
            return (char *)( it->data ) + it->size;
    }
    auto tmp = (char *)(it->data) - (char *)(base) + it->size;
    if (capacity - tmp >= space)
        return (char *)( it->data ) + it->size;
    return nullptr;
}

bool Allocator::hasEnoughSpace(Storage &st, size_t space) {
    std::list<Storage> area_copy = area;
    area_copy.sort();
    auto it = std::find(area_copy.begin(), area_copy.end(), st);
    if ( it == area_copy.end() )
        return false;
    if ( it == std::prev(area_copy.end())) {
        auto tmp = (char *)(it->data) - (char *)(area_copy.begin()->data);
        return (capacity - tmp >= space);
    }
    auto tmp = (char *)(std::next(it)->data) - (char *)(it->data);
    return (tmp - it->size >= space);
}

Pointer Allocator::alloc(size_t N) {
    size_t newOffset = offset + N;
    if (newOffset <= capacity) {
        Storage st((char *)base + offset, N);
        area.push_back(st);
        auto iter = std::prev(area.end());
        offset = newOffset;
        return Pointer(iter);
    }
    else if (void *ptr = findSpace(N)) {
        Storage st(ptr, N);
        area.push_back(st);
        return Pointer(std::prev(area.end()));
    }
    throw AllocError(AllocErrorType::NoMemory, "No memory");
}

void Allocator::realloc(Pointer &p, size_t N) {
    if (p.get() == nullptr) {
        p = alloc(N);
        return;
    }
    if (N <= p.it->size || hasEnoughSpace(*p.it, N)) {
        p.it->size = N;
        auto tmp = (char *)(p.it->data) - (char *)base;
        if (tmp + N > offset)
            offset = tmp + N;
        return;
    }
    auto new_p = alloc(N);
    memcpy(new_p.it->data, p.it->data, p.it->size);
    free(p);
    p = new_p;
}

void Allocator::free(Pointer &p) {
    if (!p.correct) {
        throw AllocError(AllocErrorType::InvalidFree, "Incorrect Pointer");
    }
    p.it->data = nullptr;
    p.it->size = 0;
}

void Allocator::defrag() {
    std::list<Pointer> area_map;
    for (auto it = area.begin(); it != area.end(); ++it)
        if (it->data != nullptr)
            area_map.push_back(Pointer(it));
    area_map.sort();
    auto iter = area_map.begin();
    for (++iter; iter != area_map.end(); ++iter) {
        void *data = (char *)( std::prev(iter)->it->data ) + std::prev(iter)->it->size;
        memcpy(data, iter->it->data, iter->it->size);
        iter->it->data = data;
    }
    offset = (char *)(std::prev(iter)->it->data) - (char *)base;
}

std::string Allocator::dump() {
    size_t used = 0;
    for (auto &p: area) {
        used += p.size;
    }
    return std::to_string(used) + '/' + std::to_string(capacity) + " bytes";
}
