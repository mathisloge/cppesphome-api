/**
 * FROM https://github.com/DanielaE/CppInAction
 * with adaptions for this project
 */
#pragma once
#include <stop_token>
#include <type_traits>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/is_executor.hpp>

#include <print>

namespace cppesphomeapi::executor
{
namespace asio = boost::asio;

template <typename T>
constexpr inline bool Unfortunate = false;
template <typename T>
constexpr inline bool isExecutionContext = std::is_base_of_v<asio::execution_context, T>;
template <typename T>
constexpr inline bool isExecutor = asio::is_executor<T>::value or asio::execution::is_executor<T>::value;
template <typename T>
constexpr inline bool hasExecutor = requires(T t) {
    { t.get_executor() };
};

auto onException(std::stop_source stop_source)
{
    return [stop = std::move(stop_source)](std::exception_ptr exception) mutable {
        if (exception)
        {
            stop.request_stop();
        }
    };
}
using ServiceBase = asio::execution_context::service;
struct StopService : ServiceBase
{
    using key_type = StopService;

    static asio::io_context::id id;

    using ServiceBase::ServiceBase;
    StopService(asio::execution_context &Ctx, std::stop_source Stop)
        : ServiceBase(Ctx)
        , stop_(std::move(Stop))
    {}

    std::stop_source get() const
    {
        return stop_;
    }
    bool isStopped() const
    {
        return stop_.stop_requested();
    }
    void requestStop()
    {
        stop_.request_stop();
    }

  private:
    void shutdown() noexcept override
    {}
    std::stop_source stop_;
};

void addStopService(asio::execution_context &Executor, std::stop_source &Stop)
{
    asio::make_service<StopService>(Executor, Stop);
}

// precondition: 'Context' provides a 'StopService'

std::stop_source getStop(asio::execution_context &Context)
{
    if (not asio::has_service<StopService>(Context))
    {
        std::println("Context does not have a stop service :/");
    }
    return asio::use_service<StopService>(Context).get();
}

template <typename T>
asio::execution_context &getContext(T &Object) noexcept
{
    if constexpr (isExecutionContext<T>)
    {
        return Object;
    }
    else if constexpr (isExecutor<T>)
    {
        return Object.context();
    }
    else if constexpr (hasExecutor<T>)
    {
        return Object.get_executor().context();
    }
    else
    {
        static_assert(Unfortunate<T>, "Please give me an execution context");
    }
}

// return the stop_source that is 'wired' to the given 'Object'

/*export*/ [[nodiscard]] auto stopAssetOf(auto &Object)
{
    return getStop(getContext(Object));
}

template <typename T>
constexpr inline bool isAwaitable = false;
template <typename T>
constexpr inline bool isAwaitable<asio::awaitable<T>> = true;

// A compile-time function that answers the questions if a given callable 'Func'
//  - can be called with a set of argument types 'Ts'
//  - returns an awaitable
//  - must be called synchronously or asynchronously

template <typename Func, typename... Ts>
struct IsCallable
{
    using ReturnType = std::invoke_result_t<Func, Ts...>;
    static constexpr bool invocable = std::is_invocable_v<Func, Ts...>;
    static constexpr bool returnsAwaitable = isAwaitable<ReturnType>;
    static constexpr bool synchronously = invocable and not returnsAwaitable;
    static constexpr bool asynchronously = invocable and returnsAwaitable;
};

// initiate independent asynchronous execution of a piece of work on a given executor.
// signal stop if an exception escapes that piece of work.

#define WORKITEM std::invoke(std::forward<Func>(work), std::forward<Ts>(args)...)

/*export*/ template <typename Func, typename... Ts>
    requires(IsCallable<Func, Ts...>::asynchronously)
void commission(auto &&executor, Func &&work, Ts &&...args)
{
    auto stop = stopAssetOf(executor);
    asio::co_spawn(executor, WORKITEM, onException(stop));
}

// abort operation of a given object depending on its capabilities.
// the close() operation is customizable to cater for more involved closing requirements.

#define THIS_WORKS(x)                                                                                                  \
    (requires(T object) {                                                                                              \
        { x };                                                                                                         \
    })(x);

// clang-format off
template <typename T>
void abort_impl(T & object) {
	if      constexpr THIS_WORKS( close(object)   )
	else if constexpr THIS_WORKS( object.close()  )
	else if constexpr THIS_WORKS( object.cancel() )
	else
		static_assert(Unfortunate<T>, "Please tell me how to abort on this 'Object'");
}
// clang-format on

// create an object that is wired up to abort the operation of all given objects
// whenever a stop is indicated.

/*export*/ [[nodiscard]] auto abort(auto &object, auto &...more_objects)
{
    return std::stop_callback{stopAssetOf(object).get_token(),
                              [&] { (abort_impl(object), ..., abort_impl(more_objects)); }};
}

} // namespace cppesphomeapi::executor
