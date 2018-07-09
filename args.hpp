//
// Created by sileader on 18/07/07.
//

#ifndef CLPKG_ARGS_HPP
#define CLPKG_ARGS_HPP

#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <sstream>


namespace args {
    enum class arg_type {
        POSITIONAL, FLAG
    };

    namespace detail {
        template<class Container> std::string join(const Container& c, const std::string& delim) {
            if(c.empty())return "";

            std::stringstream ss;
            for(const auto& v : c) {
                ss<<v<<delim;
            }
            auto str = ss.str();
            str.resize(str.size() - delim.size());

            return str;
        }

        class _args_parser_impl {
        private:
            static inline constexpr std::size_t ARG_NAME = 0, ARG_TYPE = 1, ARG_DESC = 2, ARG_VALUE = 3;
            using args_type = std::tuple<
                    std::vector<std::string>, // flags or position name
                    arg_type, // argument type
                    std::string, // description
                    std::vector<std::string> // value
            >;

            static inline constexpr std::size_t SUB_PARSER = 0, SUB_NAME = 1, SUB_DESC = 2;
            using sub_command_type = std::tuple<
                    std::shared_ptr<_args_parser_impl>,
                    std::string,
                    std::string
            >;

        private:
            std::vector<args_type> _args;
            std::unordered_map<std::string, sub_command_type> _sub;
            std::vector<std::string> _parameters;
            bool _selected=false;

        public:
            template<class Container, class String> void add_flag(Container&& flags, String&& desc) {
                args_type arg;
                std::get<ARG_NAME>(arg).insert(std::begin(std::get<ARG_NAME>(arg)), std::begin(flags), std::end(flags));
                std::get<ARG_TYPE>(arg) = arg_type::FLAG;
                std::get<ARG_DESC>(arg) = std::forward<String>(desc);

                _args.emplace_back(arg);
            }

            template<class Container, class String> void add_positional(Container&& name, String&& desc) {
                args_type arg;
                std::get<ARG_NAME>(arg).insert(std::begin(std::get<ARG_NAME>(arg)), std::begin(name), std::end(name));
                std::get<ARG_TYPE>(arg) = arg_type::POSITIONAL;
                std::get<ARG_DESC>(arg) = std::forward<String>(desc);

                _args.emplace_back(arg);
            }

            std::shared_ptr<_args_parser_impl> add_subcommand(const std::string& name, const std::string& desc) {
                sub_command_type sub;
                std::get<SUB_NAME>(sub) = name;

                auto parser = std::make_shared<_args_parser_impl>();
                std::get<SUB_PARSER>(sub) = parser;
                std::get<SUB_DESC>(sub) = desc;

                _sub[name] = sub;

                return parser;
            }

        private:
            void _mark_selected() {
                _selected = true;
            }
            template<class Iterator> void _parse_args_impl(Iterator first, Iterator last) {
                for(auto itr = first; itr != last; ++itr) {
                    std::string value;
                    auto sitr = std::find_if(std::begin(_args), std::end(_args), [this, &itr, &value](const args_type& at) -> bool{
                        const auto& name = std::get<ARG_NAME>(at);
                        if(std::get<ARG_TYPE>(at) == arg_type::FLAG) {
                            value = "yes";
                            return std::find(std::begin(name), std::end(name), *itr) != std::end(name);
                        }

                        return std::find_if(std::begin(name), std::end(name), [&itr, &value](const std::string& s) -> bool{
                            if(itr->find("=") != std::string::npos) {
                                auto pos = itr->find(s);

                                if(pos != std::string::npos) {
                                    value = itr->substr(pos);
                                    return true;
                                }
                                return false;

                            }else{
                                ++itr;
                                value = *itr;
                                return true;
                            }
                        }) != std::end(name);
                    });

                    if(sitr != std::end(_args)) {
                        std::get<ARG_VALUE>(*sitr).emplace_back(value);
                    }else{
                        if(itr->find("--")==0) {
                            std::cerr<<"unknown option "<<(*itr)<<std::endl;
                            std::exit(1);
                        }
                        _parameters.emplace_back(*itr);
                    }
                }
            }
        public:
            void parse_args(const std::vector<std::string>& args) {
                if(args.size() > 1 && _sub.count(args[1]) != 0) {
                    auto& sub_parser = std::get<SUB_PARSER>(_sub[args[1]]);
                    sub_parser->_parse_args_impl(std::begin(args) + 2, std::end(args));
                    sub_parser->_mark_selected();
                    return;
                }

                _parse_args_impl(std::begin(args) + 1, std::end(args));
            }

            void parse_args(int argc, const char * const *argv) {
                std::vector<std::string> args(argv, argv + argc);
                parse_args(args);
            }

        public:
            bool is_selected()const noexcept {
                return _selected;
            }

            std::vector<std::string> values(const std::string& key)const {
                auto itr = std::find_if(std::begin(_args), std::end(_args), [this, &key](const args_type& at) -> bool {
                    const auto& name = std::get<ARG_NAME>(at);
                    return std::find(std::begin(name), std::end(name), key) != std::end(name);
                });
                if(itr != std::end(_args)) {
                    return std::get<ARG_VALUE>(*itr);
                }
                throw std::runtime_error("key is not exists");
            }

            std::string value(const std::string& key)const {
                auto val = values(key);
                if(!val.empty()) {
                    return val[0];
                }
                throw std::runtime_error("value is not exists");
            }

            const std::vector<std::string>& parameters()const {
                return _parameters;
            }
        public:
            std::string help()const {
                std::stringstream ss;

                if(!_sub.empty()) {
                    ss<<"sub commands\n";
                    for(const auto& s : _sub) {
                        ss<<"  "<<s.first<<" : "<<std::get<SUB_DESC>(s.second)<<'\n';
                    }
                    ss<<'\n';
                }

                if(!_args.empty()) {
                    std::stringstream fss, pss;
                    fss<<"flags:\n";
                    pss<<"positional arguments:\n";

                    bool has_positional=false, has_flags=false;
                    for(const auto& a : _args) {
                        switch(std::get<ARG_TYPE>(a)) {
                            case arg_type::FLAG:
                                fss<<"  "<<join(std::get<ARG_NAME>(a), ", ")<<" : "<<std::get<ARG_DESC>(a)<<'\n';
                                has_flags = true;
                                break;

                            case arg_type::POSITIONAL:
                                pss<<"  "<<join(std::get<ARG_NAME>(a), ", ")<<" : "<<std::get<ARG_DESC>(a)<<'\n';
                                has_positional = true;
                                break;
                        }
                    }

                    if(has_flags) {
                        ss<<fss.str();
                    }
                    if(has_positional) {
                        ss<<"\n\n"<<pss.str();
                    }
                }

                return ss.str();
            }
        };
    } /* detail */

    class argument_parser {
    private:
        std::shared_ptr<detail::_args_parser_impl> _parser;
        std::string _program_name, _usage, _epilog;

    private:
        explicit argument_parser(std::shared_ptr<detail::_args_parser_impl> parser,
                                 const std::string& name, const std::string& usage, const std::string& epilog="")
                : _parser(std::move(parser)) ,
                  _program_name(name),
                  _usage(usage),
                  _epilog(epilog){
            add_flag({"--help", "-h"}, "show this help.");
        }

    public:
        argument_parser(const std::string& name, const std::string& usage, const std::string& epilog="")
                : argument_parser(std::make_shared<detail::_args_parser_impl>(), name, usage, epilog) {
        }
        argument_parser(const argument_parser&)=default;
        argument_parser(argument_parser&&)=default;
        argument_parser& operator=(const argument_parser&)=default;
        argument_parser& operator=(argument_parser&&)=default;

        ~argument_parser()=default;
    public:
        template<class String> void add_flag(const std::vector<std::string>& flags, String&& desc) {
            _parser->add_flag(flags, std::forward<String>(desc));
        }
        template<class Container, class String> void add_flag(Container&& flags, String&& desc) {
            _parser->add_flag(std::forward<Container>(flags), std::forward<String>(desc));
        }

        template<class String> void add_positional(const std::vector<std::string>& flags, String&& desc) {
            _parser->add_flag(flags, std::forward<String>(desc));
        }
        template<class Container, class String> void add_positional(Container&& name, String&& desc) {
            _parser->add_positional(std::forward<Container>(name), std::forward<String>(desc));
        }

        argument_parser add_subcommand(const std::string& name, const std::string& desc) {
            return argument_parser(_parser->add_subcommand(name, desc), _program_name, _usage, _epilog);
        }

    public:
        void show_help() {
            std::cout
                    <<_program_name<<'\n'
                    <<_usage<<"\n\n";

            std::cout<<_parser->help()<<std::endl;

            std::cout<<_epilog<<std::endl;
        }
    public:
        void parse_args(const std::vector<std::string>& args, bool auto_help=true) {
            _parser->parse_args(args);

            if(auto_help && exists("-h")) {
                show_help();
                std::exit(0);
            }
        }

        void parse_args(int argc, const char * const *argv, bool auto_help=true) {
            _parser->parse_args(argc, argv);

            if(auto_help && exists("-h")) {
                show_help();
                std::exit(0);
            }
        }

    public:
        bool is_selected()const noexcept {
            return _parser->is_selected();
        }
    public:
        std::vector<std::string> values(const std::string& key)const {
            return _parser->values(key);
        }

        std::string value(const std::string& key)const {
            return _parser->value(key);
        }

        const std::vector<std::string>& parameters()const {
            return _parser->parameters();
        }

        bool exists(const std::string& key)const {
            try {
                return !value(key).empty();
            }catch(const std::runtime_error&) {
                return false;
            }
        }
    };
} /* args */

#endif //CLPKG_ARGS_HPP
