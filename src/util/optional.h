#pragma once

// Copyright 2015 Payshares Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

namespace payshares
{
template <typename T> using optional = std::shared_ptr<T>;

template <typename T, class... Args>
optional<T>
make_optional(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T>
optional<T>
nullopt()
{
    return std::shared_ptr<T>();
};
}
