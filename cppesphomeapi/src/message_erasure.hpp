#pragma once
#include <memory>
namespace cppesphomeapi
{

class MessageHandler
{
  public:
    template <typename T>
    MessageHandler(T message)
        : message_wrapper_{std::make_unique<MessageModel<std::remove_cvref_t<T>>>(std::forward<T>(message))}
    {}

  private:
    class MessageConcept
    {
      public:
        virtual ~MessageConcept() = default;
        virtual void handle() = 0;
    };

    template <typename TMsg>
    class MessageModel final : public MessageConcept
    {
      public:
        MessageModel(TMsg &&message)
            : message_{std::move(message)}
        {}

        void handle() override
        {}

      private:
        TMsg message_;
    };

  private:
    std::unique_ptr<MessageConcept> message_wrapper_;
};
} // namespace cppesphomeapi
