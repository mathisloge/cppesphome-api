#pragma once
#include <memory>
#include <google/protobuf/message.h>
#include "api_options.pb.h"
#include "get_message_id.hpp"

namespace cppesphomeapi
{

class MessageWrapper
{
  public:
    MessageWrapper() = delete;
    ~MessageWrapper() = default;

    template <typename TMsg>
        requires std::is_base_of_v<google::protobuf::Message, TMsg>
    MessageWrapper(std::shared_ptr<TMsg> message)
        : message_{std::move(message)}
        , message_id_{detail::get_message_id<TMsg>()}
    {}

    MessageWrapper(MessageWrapper &&wrapper) noexcept
        : message_{std::exchange(wrapper.message_, message_)}
        , message_id_{std::exchange(wrapper.message_id_, message_id_)}
    {}

    MessageWrapper(const MessageWrapper &wrapper)
        : message_{wrapper.message_}
        , message_id_{wrapper.message_id_}
    {}

    MessageWrapper &operator=(MessageWrapper &&rhs) noexcept
    {
        message_ = std::exchange(rhs.message_, message_);
        message_id_ = std::exchange(rhs.message_id_, message_id_);
        return *this;
    }

    MessageWrapper &operator=(const MessageWrapper &rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }
        message_ = rhs.message_;
        message_id_ = rhs.message_id_;
        return *this;
    }

    template <typename TMsg>
        requires std::is_base_of_v<google::protobuf::Message, TMsg>
    std::shared_ptr<TMsg> as() const
    {
        const auto requested_message = detail::get_message_id<TMsg>();
        if (requested_message == message_id_)
        {
            return std::dynamic_pointer_cast<TMsg>(message_);
        }
        return nullptr;
    }

    template <typename TMsg>
        requires std::is_base_of_v<google::protobuf::Message, TMsg>
    bool holds_message() const
    {
        return detail::get_message_id<TMsg>() == message_id_;
    }

    const google::protobuf::Message &ref() const
    {
        return *message_;
    }

  private:
    std::uint32_t message_id_{};
    std::shared_ptr<google::protobuf::Message> message_;
};
} // namespace cppesphomeapi
