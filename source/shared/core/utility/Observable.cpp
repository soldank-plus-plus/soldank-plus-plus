module;

#include <list>
#include <memory>
#include <functional>

export module Shared.Core.Utility.Observable;

export namespace Soldank
{
template<typename... OnNotifyArgs>
class Observable
{
public:
    void AddObserver(const std::function<void(OnNotifyArgs...)>& observer_callback)
    {
        observers_.push_back(std::move(observer_callback));
    }

    void Notify(OnNotifyArgs... on_notify_args) const
    {
        for (const auto& observer_callback : observers_) {
            observer_callback(on_notify_args...);
        }
    }

private:
    std::list<std::function<void(OnNotifyArgs...)>> observers_;
};

} // namespace Soldank
