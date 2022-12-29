/*
 * Copyright (c) 2021, Kyle Pereira <hey@xylepereira.me>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibCore/EventLoop.h>
#include <LibCore/Object.h>

namespace Core {

template<typename Result>
class Promise : public Object {
    C_OBJECT(Promise);

public:
    Function<void(Result&)> on_resolved;

    void resolve(Result&& result)
    {
        m_pending_or_error = move(result);

        if (on_resolved)
            on_resolved(m_pending_or_error.value());
    }

    void cancel(Error error)
    {
        m_pending_or_error = move(error);
    }

    bool is_canceled()
    {
        return m_pending_or_error.has_value() && m_pending_or_error->is_error();
    }

    bool is_resolved() const
    {
        return m_pending_or_error.has_value() && !m_pending_or_error->is_error();
    }

    ErrorOr<Result> await()
    {
        while (!m_pending_or_error.has_value())
            Core::EventLoop::current().pump();

        return m_pending_or_error.release_value();
    }

    // Converts a Promise<A> to a Promise<B> using a function func: A -> B
    template<typename T>
    RefPtr<Promise<T>> map(T func(Result&))
    {
        RefPtr<Promise<T>> new_promise = Promise<T>::construct();
        on_resolved = [new_promise, func](Result& result) {
            auto t = func(result);
            new_promise->resolve(move(t));
        };
        return new_promise;
    }

private:
    Promise() = default;

    Optional<ErrorOr<Result>> m_pending_or_error;
};

}
