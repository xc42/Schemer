#pragma once

#include <string_view>
#include <unordered_map>
#include <memory>
#include "fmt/format.h"

template<class V>
class Environment {
public:
    using Ptr = std::shared_ptr<Environment>;

    void bind(const std::string_view name, V val) {
        _bindings.emplace(name, std::move(val)); 
    }

	V& find(std::string_view var) {
        auto it = _bindings.find(var);
        if( it != _bindings.end()) {
            return it->second;
        } else if(_outer) {
            return _outer->find(var);
        }else {
            throw std::runtime_error(fmt::format("`{}` undefined", var));
        }
    }

    static Ptr extend(Ptr old) {
        auto ret    = std::make_shared<Environment<V>>();
        ret->_outer = old;
        return ret;
    }

private:
    std::unordered_map<std::string_view, V> _bindings;
    Ptr                                     _outer;
};
