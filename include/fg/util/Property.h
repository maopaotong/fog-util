/*
 * SPDX-FileCopyrightText: 2025 Mao-Pao-Tong Workshop
 * SPDX-License-Identifier: MPL-2.0
 */
#pragma once
#include "Common.h"
#include "Options.h"

namespace fog
{
    class Property
    {

    public:
        struct LaterBind
        {
            std::any ptr;

            template <typename T>
            T *getPtr()
            {
                if (!ptr.has_value())
                {
                    return nullptr;
                }
                return std::any_cast<T *>(ptr);
            }

            template <typename T>
            void setPtr(T *ptr_)
            {
                this->ptr = ptr_;
            }
        };

        template <typename T>
        class Ref
        {
            std::string name;
            T *ptr_ = nullptr;
            std::shared_ptr<LaterBind> later;

            void doBind(T &ref)
            {
                doBind(std::addressof(ref));
            }
            void doBind(T *ptr)
            {
                ptr_ = ptr;
            }

            void doSet(T value)
            {
                *ptr_ = value;
            }

        public:
            Ref() : later(nullptr)
            {
            }
            Ref(std::shared_ptr<LaterBind> later) : later(later) {

                                                    };

            Ref(T &&) = delete;
            Ref(T &ref)
            {
                ptr_ = std::addressof(ref);
            };
            Ref(const Ref &other) : ptr_(other.ptr_) {};
            Ref(Ref &&other) : ptr_(other.ptr_) {};
            Ref(T *ptr) : ptr_(ptr)
            {
            }
            std::string getName()
            {
                return name;
            }

            void set(T value)
            {
                assertBind() = value;
            }

            void set(T &&value)
            {
                assertBind() = value;
            }

            Ref &operator=(const Ref &other)
            {
                this->ptr_ = other.ptr_;
                this->later = other.later;
                return *this;
            }

            Ref &operator=(Ref &&other)
            {
                this->ptr_ = other.ptr_;
                this->later = other.later;
                return *this;
            }

            Ref &operator=(T &&value)
            {
                assertBind() = value;
                return *this;
            }

            Ref &operator=(const T value)
            {
                assertBind() = value;
                return *this;
            }

            bool empty() const { return ptr_ == nullptr; }

            bool emptyAllTime()
            {
                return ptr_ == nullptr && !later;
            }

            operator T &() &
            {
                return assertBind();
            }

            operator const T &() const &
            {
                if (ptr_)
                {
                    return *ptr_;
                }
                LaterBind *lb = this->later.get();
                if (lb)
                {
                    T *ptr2 = lb->getPtr<T>();
                    if (!ptr2)
                    {
                        throw std::runtime_error("cannot resolve Property::Ref for now.");
                    }
                    return *ptr2;//resolve from later bind.
                }

                throw std::runtime_error("cannot bind and no later bind.");
            }

            T &assertBind()
            {
                if (ptr_)
                {
                    return *ptr_;
                }
                LaterBind *lb = this->later.get();
                if (lb)
                {
                    T *ptr2 = lb->getPtr<T>();
                    if (ptr2)
                    {

                        this->ptr_ = ptr2;
                        later = nullptr; // release shared ptr.
                        return *ptr_;
                    }
                }

                throw std::runtime_error("property ref is empty or not available for now.");
            }
        };

        class Bag
        {
            Options options;
            std::unordered_map<std::string, std::shared_ptr<LaterBind>> laterBinds;

        public:
            template <typename T>
            Property::Ref<T> createProperty(std::string name, T defaultValue)
            {
                return createProperty<T>(name, defaultValue, false);
            }

            template <typename T>
            Property::Ref<T> createProperty(std::string name, T defaultValue, bool allowLater)
            {

                Options::Option *opt = options.tryAdd<T>(name, defaultValue);
                if (!opt)
                {
                    if (!allowLater)
                    {
                        throw std::runtime_error("cannot create property ref with name:" + name);
                    }
                    return makeLater<T>(name);
                    //
                }
                else
                {

                    // notify other later ref.
                    auto it = laterBinds.find(name);
                    if (it != laterBinds.end())
                    {
                        T* ptr = &opt->getValueRef<T>();

                        laterBinds[name]->setPtr<T>(ptr);
                        laterBinds.erase(name); // release the later bind.
                    }

                    return Property::Ref<T>(opt->getValueRef<T>()); // bind this ref for now, no need to bind later.
                }
            }
            template <typename T>
            Property::Ref<T> makeLater(std::string name)
            {
                auto it = laterBinds.find(name);
                if (it == laterBinds.end())
                {
                    auto later = std::make_shared<LaterBind>();
                    laterBinds[name] = later;
                    // empty and later bind.
                    return Property::Ref<T>(later);
                }
                else
                {
                    return Property::Ref<T>(it->second);
                }
            }

            template <typename T>
            Property::Ref<T> getProperty(std::string name, bool requiredNow = true)
            {
                Options::Option *opt = options.getOption(name);
                if (opt)
                {
                    T &ref = opt->getValueRef<T>();
                    return Property::Ref<T>(ref);
                }
                else
                {
                    if (requiredNow)
                    {
                        throw std::runtime_error("property ref not exists by name:" + name);
                    }

                    return makeLater<T>(name);
                }
            }
            template <typename F>
            void forEach(F &&func)
            {
                options.forEach(std::forward<F>(func));
            }
        };
    };
}; //
