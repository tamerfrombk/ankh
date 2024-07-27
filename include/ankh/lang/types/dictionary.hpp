#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <ankh/lang/types/entry.hpp>

namespace ankh::lang {

template <class T> class Dictionary {
    using ElementType = Entry<T>;
    using DictionaryType = std::vector<ElementType>;
    using DictionaryIterator = typename std::shared_ptr<DictionaryType>::element_type::iterator;

  public:
    Dictionary() : Dictionary(DictionaryType{}) {}
    Dictionary(DictionaryType dict) : dict_(std::make_shared<DictionaryType>(std::move(dict))) {}

    bool empty() const noexcept { return dict_->empty(); }

    size_t size() const noexcept { return dict_->size(); }

    DictionaryIterator begin() const noexcept { return dict_->begin(); }

    DictionaryIterator end() const noexcept { return dict_->end(); }

    DictionaryIterator begin() noexcept { return dict_->begin(); }

    DictionaryIterator end() noexcept { return dict_->end(); }

    bool insert(const T &key, const T &value) noexcept {
        if (!this->value(key)) {
            dict_->push_back(ElementType{key, value});

            return true;
        }

        return false;
    }

    std::optional<ElementType> value(const T &key) const noexcept {
        auto it = std::find_if(dict_->cbegin(), dict_->cend(), [&](const Entry<T> &entry) { return entry.key == key; });

        return it == dict_->cend() ? std::nullopt : std::optional<ElementType>{*it};
    }

    std::optional<ElementType> value(const std::string &key) const noexcept { return value(T(key)); }

  private:
    std::shared_ptr<DictionaryType> dict_;
};
} // namespace ankh::lang
