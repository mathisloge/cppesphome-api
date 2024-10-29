#pragma once
#include <memory>
#include <print>
#include <google/protobuf/message.h>
#include "api_options.pb.h"
#include "get_message_id.hpp"

namespace cppesphomeapi
{

class MessageWrapper
{
  public:
    template <typename TMsg>
        requires std::is_base_of_v<google::protobuf::Message, TMsg>
    MessageWrapper(std::shared_ptr<TMsg> message)
        : message_{std::move(message)}
        , message_id_{detail::get_message_id<TMsg>()}
    {}

    template <typename TMsg>
        requires std::is_base_of_v<google::protobuf::Message, TMsg>
    std::shared_ptr<TMsg> as() const
    {
        const auto requested_message = detail::get_message_id<TMsg>();
        if (requested_message == message_id_)
        {
            auto cnv_msg = std::dynamic_pointer_cast<TMsg>(message_);
            if (cnv_msg == nullptr)
            {
                std::println("Got null msg for {}, message is null=", message_id_, (message_ == nullptr));
            }
            return cnv_msg;
        }
        return nullptr;
    }

    template <typename TMsg>
        requires std::is_base_of_v<google::protobuf::Message, TMsg>
    bool holds_message() const
    {
        return detail::get_message_id<TMsg>() == message_id_;
    }

  private:
    std::uint32_t message_id_{};
    std::shared_ptr<google::protobuf::Message> message_;
};
} // namespace cppesphomeapi
