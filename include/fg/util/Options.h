/*
 * SPDX-FileCopyrightText: 2025 Mao-Pao-Tong Workshop
 * SPDX-License-Identifier: MPL-2.0
 */
#pragma once
#include "Common.h"

namespace fog
{

    struct Options
    {
        struct Ref
        {
            std::string group;
            std::string key;
            Ref(std::string group, std::string key) : group(group), key(key)
            {
            }
            Ref(const Ref &) = default;
            Ref(Ref &&) = default;
            Ref &operator=(const Ref &) = default;
            Ref &operator=(Ref &&) = default;
        };

        struct Option
        {
            std::string name;
            std::any valuePtr;
            std::function<void(std::any)> destructor;
            std::function<std::any()> copy;
            std::function<std::any()> value;
            std::type_index valueType;
            ~Option()
            {
                destructor(this->valuePtr);
            }
            template <typename T>
            Option(std::string name, T defaultV) : name(name),valueType(typeid(T))
            {
                this->copy = [defaultV]()
                {
                    T *valueP = new T(defaultV);
                    return std::make_any<T *>(valueP);
                };
                this->valuePtr = copy();
                this->destructor = [](std::any valuePtr)
                {
                    T *valueP = std::any_cast<T *>(valuePtr);
                    delete valueP;
                };
                this->value = [this]()
                {
                    T *valueP = std::any_cast<T *>(valuePtr);
                    return std::make_any<T>(*valueP);
                };
                this->valueType = typeid(T);
            }

            Option(const Option &opt) : name(opt.name),valueType(opt.valueType)
            {
                doCopy(opt);
            }; // copy

            Option(Option &&opt) : name(opt.name),valueType(opt.valueType)
            {
                doMove(opt);
                
            }; // move
            private:
            void doCopy(const Option &opt){

                this->copy = opt.copy;
                this->destructor = opt.destructor;
                this->valuePtr = this->copy();
                this->value = opt.value;
            }

            void doMove(Option &opt){
                this->copy = opt.copy;
                this->destructor = opt.destructor;
                this->valuePtr = opt.valuePtr;
                opt.destructor = [](std::any) {}; //
                opt.copy = []() -> std::any
                { return nullptr; };
                opt.valuePtr = opt.copy();
                opt.value = []() -> std::any
                { throw std::runtime_error("empty value."); };
            }
            
            public:
            
            Option &operator=(const Option &opt)
            {
                this->name = opt.name;
                this->valueType = opt.valueType;
                //
                doCopy(opt);
                return *this;
            }; // copy assign

            Option &operator=(Option &&opt)
            {
                this->name = opt.name;
                this->valueType = opt.valueType;
                //
                doMove(opt);
                return *this;
            }; // move assign

            template <typename T>
            bool isType() const
            {
                return valuePtr.type() == typeid(T *);
            }
            std::type_index getType(){
                return valueType;
            }

            template <typename T>
            T &getValueRef() const
            {
                T *p = std::any_cast<T *>(valuePtr);
                return *p;
            }

            std::any getValue(){
                return this->value();
            }
        };

        struct Groups
        {
            std::unordered_map<std::string, Options> groups;
        };

    public:
        template <typename T>
        static T get(Options &opts, std::string name, T defValue)
        {
            Option *opt = opts.getOption(name);
            if (!opt)
            {
                return defValue;
            }
            if (opt->isType<T>())
            {
                return opt->getValueRef<T>();
            }
            throw std::runtime_error(fmt::format("type of option:{} is mismatch.", name));
        }

    protected:
        std::unordered_map<std::string, std::unique_ptr<Option>> options;

    public:
    public:
        ~Options()
        {
        }
        Option *getOption(std::string name)
        {
            const auto &it = options.find(name);
            if (it == options.end())
            {
                return nullptr;
            }
            return it->second.get();
        }

        template <typename T>
        Option *add(std::string name, T defaultValue)
        {
            Option *ret = tryAdd(name, defaultValue);

            if (ret)
            {
                return ret;
            }
            throw new std::runtime_error("cannot create options with name:" + name);
        }

        template <typename T>
        Option *tryAdd(std::string name, T defaultValue)
        {
            Option *opt = nullptr;
            auto it = options.find(name);
            if (it == options.end())
            {
                std::unique_ptr<Option> optionPtr = std::make_unique<Option>(name, defaultValue);
                //<std::unique_ptr<Option>>
                opt = optionPtr.get();
                options[name] = std::move(optionPtr);
            }

            return opt;
        }

        void add(Option &opt)
        {
            if (auto it = options.find(opt.name); it != options.end())
            {
                throw std::runtime_error("option with name:" + opt.name + " already exists.");
            }
            options[opt.name] = std::make_unique<Option>(opt);
        }

        template <typename F>
        void forEach(F &&visit)
        {
            for (const auto &pair : options)
            {
                visit(pair.first, pair.second.get());
            }
        }
        void merge(Options &opts)
        {
            for (auto it = opts.options.begin(); it != opts.options.end(); it++)
            {

                std::string key = it->first;
                if (auto it2 = this->options.find(key); it2 == this->options.end())
                {

                    this->options.emplace(it->first, std::make_unique<Option>(*it->second));
                } //
                // ignore duplicated entry.
            }
        }
        void replaceAll(Options &opts)
        {
            for (auto it = opts.options.begin(); it != opts.options.end(); it++)
            {

                std::string key = it->first;

                this->options[key] = std::make_unique<Option>(*it->second);
            }
        }
    };

};
