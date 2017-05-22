/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#ifdef KL_WINDOWS
#   define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan.h>
#include <functional>

namespace vk
{
    template <class T>
    class Resource
    {
    public:
        Resource(): Resource([](T, VkAllocationCallbacks*) {})
        {
        }

        Resource(const Resource<T> &other) = delete;
        Resource(Resource<T> &&other) noexcept
        {
            swap(other);
        }

        explicit Resource(std::function<void(T, VkAllocationCallbacks*)> del)
        {
            this->del = [=](T handle) { del(handle, nullptr); };
        }

        Resource(VkInstance instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> del)
        {
            this->del = [instance, del](T obj) { del(instance, obj, nullptr); };
        }

        Resource(VkDevice device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> del)
        {
            this->del = [device, del](T obj) { del(device, obj, nullptr); };
        }

        ~Resource()
        {
            cleanup();
        }

        auto operator=(Resource<T> other) noexcept -> Resource<T>&
        {
            swap(other);
            return *this;
        }

        void operator=(T other)
        {
            if (other != handle)
            {
                cleanup();
                handle = other;
            }
        }

        auto operator&() const -> const T*
        {
            return &handle;
        }

        auto cleanRef() -> T*
        {
            cleanup();
            return &handle;
        }

        operator T() const
        {
            return handle;
        }

        operator bool() const
        {
            return handle != VK_NULL_HANDLE;
        }

        template<typename V>
        bool operator==(V rhs)
        {
            return handle == T(rhs);
        }

    private:
        T handle = VK_NULL_HANDLE;
        std::function<void(T)> del;

        void cleanup()
        {
            if (handle != VK_NULL_HANDLE)
                del(handle);
            handle = VK_NULL_HANDLE;
        }

        void swap(Resource<T> &other) noexcept
        {
            std::swap(handle, other.handle);
            std::swap(del, other.del);
        }
    };
}
