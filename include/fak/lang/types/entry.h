#pragma once

namespace fk::lang {

template <class T>
struct Entry {
    T key, value;

    Entry(T key, T value)
        : key(std::move(key)), value(std::move(value)) {}
};

}