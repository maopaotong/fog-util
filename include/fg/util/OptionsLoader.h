/*
 * SPDX-FileCopyrightText: 2025 Mao-Pao-Tong Workshop
 * SPDX-License-Identifier: MPL-2.0
 */
#pragma once

#include <fstream>
#include <filesystem>
#include "Options.h"
#include "Range2.h"
namespace fog
{

    struct OptionsLoader
    {

        void load(std::vector<std::string> files, Options &opts, std::string group, bool strict)
        {
            std::unordered_map<std::string, Options> optsMap;
            load(files, optsMap, strict);
            opts.merge(optsMap[group]);
        }

        void load(std::vector<std::string> files, Options::Groups &optMap, bool strict)
        {
            load(files, optMap.groups, strict);
        }
        
        void load(std::vector<std::string> files, std::unordered_map<std::string, Options> &optMap, bool strict)
        {
            doLoad(files, optMap, strict);

            // resolve refs
            for (auto it = optMap.begin(); it != optMap.end(); ++it)
            {
                std::string group = it->first;
                Options &opts = optMap[group];
                Options resolvedOpts;
                opts.forEach([this, &group, &resolvedOpts, &optMap](std::string key, Options::Option *optPtr)
                             {
                                 Options::Option &opt = *optPtr;
                                 std::unordered_set<std::string> processedFKeys;
                                 if (opt.isType<Options::Ref>())
                                 {
                                     Options::Ref &refV = opt.getValueRef<Options::Ref>();
                                     Options::Option &opt2 = resolveRef(optMap, refV.group, refV.key, processedFKeys);
                                     Options::Option opt3 = opt2;
                                     opt3.name = key;
                                     resolvedOpts.add(opt3);
                                     //std::cout << "Resolved ref option: [" << group << "] " << key << " -> " << refV.group << "." << refV.key << std::endl;
                                 } //
                             });
                opts.replaceAll(resolvedOpts);
            }
        }

    private:
        Options::Option &resolveRef(std::unordered_map<std::string, Options> &optMap, std::string group, std::string key, std::unordered_set<std::string> &processedFKeys)
        {
            std::string fkey = group + "." + key;
            if (processedFKeys.find(fkey) != processedFKeys.end())
            {
                throw std::runtime_error("cannot resolve ref:" + fkey + " (circular reference detected)");
            }
            processedFKeys.insert(fkey);
            if (auto it = optMap.find(group); it != optMap.end())
            {
                Options &opts = it->second;
                Options::Option *optPtr = opts.getOption(key);
                if (!optPtr)
                {
                    throw std::runtime_error("cannot resolve ref:" + fkey);
                }
                if (optPtr->isType<Options::Ref>())
                {
                    Options::Ref &refV = optPtr->getValueRef<Options::Ref>();
                    return resolveRef(optMap, refV.group, refV.key, processedFKeys);
                }
                else
                {
                    return *optPtr;
                }
            }
            else
            {
                throw std::runtime_error("cannot resolve ref:" + fkey + " (group not found)");
            }
        }

        void doLoad(std::vector<std::string> files, std::unordered_map<std::string, Options> &optMap, bool strict)
        {
            for (const std::string &file : files)
            {
                doLoad(file, optMap, strict);
            }
        }

        void doLoad(std::string file, std::unordered_map<std::string, Options> &optMap, bool strict)
        {
            std::filesystem::path fpath(file.c_str());
            if (!std::filesystem::exists(fpath))
            {
                throw std::runtime_error(std::string("no such file:" + file));
            }
            std::ifstream f(fpath);

            if (!f.is_open())
            {
                throw std::runtime_error("failed to open file for read: " + file);
            }
            std::string line;
            int lNum = 0;
            std::string group;
            while (std::getline(f, line))
            {
                lNum++;

                // skip space
                while (!line.empty() && line[0] == ' ')
                {
                    line = line.substr(1);
                }

                if (line.empty() || line[0] == '#' || line[0] == '/' && line.length() > 1 && line[1] == '/')
                {
                    continue;
                }

                if (line[0] == '[')
                {
                    // process group
                    auto b2 = line.find(']');
                    if (b2 == std::string::npos)
                    {
                        throw std::runtime_error("group format error.");
                    }
                    group = line.substr(1, b2 - 1);
                    continue;
                }

                Options &opts = optMap[group];

                auto eq = line.find('=');
                if (eq == std::string::npos)
                {
                    // ignore
                    continue;
                }
                std::string type = "string";

                auto typeLeft = line.find("<");
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);
                if (typeLeft != std::string::npos)
                {
                    bool typeInV = (eq < typeLeft);
                    if (typeInV)
                    {
                        typeLeft = typeLeft - eq - 1;
                    }
                    auto typeRight = (typeInV ? value : key).find_last_of(">");

                    if (typeRight == std::string::npos)
                    {
                        throw std::runtime_error(std::string("config format error for lineNum:") + std::to_string(lNum));
                    }

                    if (typeInV)
                    {
                        type = value.substr(typeLeft + 1, typeRight - typeLeft - 1);
                        value = value.substr(typeRight + 1);
                    }
                    else
                    {
                        type = key.substr(typeLeft + 1, typeRight - typeLeft - 1);
                        key = key.substr(0, typeLeft);
                    }
                }

                bool ok;
                if (type == "string")
                {
                    ok = opts.tryAdd<std::string>(key, value);
                }
                else if (type == "float")
                {
                    ok = opts.tryAdd<float>(key, std::stof(value));
                }
                else if (type == "int")
                {
                    ok = opts.tryAdd<int>(key, std::stoi(value));
                }
                else if (type == "bool")
                {
                    ok = opts.tryAdd<bool>(key, (value == "true" || value == "yes" || value == "Y" || value == "1"));
                }
                else if (type == "range2<int>")
                {
                    Range2<int> v = parseValueOfRange2Int(value);
                    ok = opts.tryAdd<Range2<int>>(key, v);
                }
                else if (type == "unsigned int")
                {
                    ok = opts.tryAdd<unsigned int>(key, static_cast<unsigned int>(std::stoul(value)));
                }
                else if (type == "ref")
                {
                    auto point = value.find_first_of('.');
                    std::string group;
                    std::string key2 = value;
                    if (point != std::string::npos)
                    {
                        group = value.substr(0, point);
                        key2 = value.substr(point + 1);
                    }

                    ok = opts.tryAdd<Options::Ref>(key, Options::Ref(group, key2));
                }
                else
                {
                    throw std::runtime_error(std::string("not supported type:") + type);
                }
                if (!ok && strict)
                {
                    throw std::runtime_error(std::string("options loading is in strict mode and item already exists:") + key);
                }
                if (ok)
                {
                    //std::cout << "Loaded option: [" << group << "] " << key << "=" << "<" << type << ">" << value << std::endl;
                }

            } // end while.

        } // end load
    private:
        Range2<int> parseValueOfRange2Int(std::string string)
        {
            std::vector<int> xyxy;
            int p1 = 0;
            std::string str = string;
            while (str.length() > 0)
            {
                std::string vS;
                if (xyxy.size() < 3)
                {

                    auto p2 = str.find_first_of(",");

                    if (p2 == std::string::npos)
                    {
                        vS = str;
                        str = ""; // empty string left
                    }
                    else
                    {
                        vS = str.substr(0, p2);
                        str = str.substr(p2 + 1, str.length() - p2 - 1);
                    }
                }
                else
                {
                    vS = str;
                    str = ""; // empty string left
                }

                int v = std::stoi(vS);
                xyxy.push_back(v);
            }

            if (xyxy.size() == 1)
            {
                return Range2<int>(xyxy[0]);
            }
            if (xyxy.size() == 2)
            {
                return Range2<int>(xyxy[0], xyxy[1]);
            }

            if (xyxy.size() == 3)
            {
                return Range2<int>(xyxy[0], xyxy[1], xyxy[2], xyxy[2]);
            }
            if (xyxy.size() == 4)
            {
                return Range2<int>(xyxy[0], xyxy[1], xyxy[2], xyxy[3]);
            }
            throw std::runtime_error("cannot parse value with type of range<int>");
        }
    };

};
