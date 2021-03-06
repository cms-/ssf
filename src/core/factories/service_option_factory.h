#ifndef SSF_CORE_FACTORIES_SERVICE_OPTION_FACTORY_H_
#define SSF_CORE_FACTORIES_SERVICE_OPTION_FACTORY_H_

#include <string>
#include <functional>
#include <map>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "common/error/error.h"
#include "services/user_services/base_user_service.h"

namespace ssf {

template <typename Demux>
class ServiceOptionFactory {
 private:
  using ServiceParserType =
      std::function<std::shared_ptr<ssf::services::BaseUserService<Demux>>(
          const std::string&, boost::system::error_code&)>;

  struct ParserDescriptor {
    ServiceParserType parser;
    std::string fullname;
    std::string value_name;
    std::string description;
  };

  using ServiceParserMap = std::map<std::string, ParserDescriptor>;

 public:
  static bool RegisterUserServiceParser(const std::string& index,
                                        const std::string& full_name,
                                        const std::string& value_name,
                                        const std::string& description,
                                        ServiceParserType parser) {
    boost::recursive_mutex::scoped_lock lock(service_options_mutex_);
    if (service_options_.count(index)) {
      return false;
    } else {
      service_options_[index] = {parser, full_name, value_name, description};
      return true;
    }
  }

  static bool UnregisterUserServiceParser(const std::string& index) {
    boost::recursive_mutex::scoped_lock lock(service_options_mutex_);
    auto it = service_options_.find(index);

    if (it != std::end(service_options_)) {
      service_options_.erase(it);
      return true;
    } else {
      return false;
    }
  }

  static boost::program_options::options_description GetOptionDescriptions() {
    boost::program_options::options_description desc(
        "Supported service commands");

    boost::recursive_mutex::scoped_lock lock(service_options_mutex_);
    for (auto& option : service_options_) {
      desc.add_options()(
          option.second.fullname.c_str(),
          boost::program_options::value<std::vector<std::string>>()->value_name(
              option.second.value_name.c_str()),
          option.second.description.c_str());
    }

    return desc;
  }

  static std::shared_ptr<ssf::services::BaseUserService<Demux>>
  ParseServiceLine(const std::string& option, const std::string& parameters,
                   boost::system::error_code& ec) {
    boost::recursive_mutex::scoped_lock lock(service_options_mutex_);
    auto it = service_options_.find(option);

    if (it != std::end(service_options_)) {
      return it->second.parser(parameters, ec);
    } else {
      ec.assign(::error::service_not_found, ::error::get_ssf_category());
      return std::shared_ptr<ssf::services::BaseUserService<Demux>>(nullptr);
    }
  }

 private:
  static boost::recursive_mutex service_options_mutex_;
  static ServiceParserMap service_options_;
};

template <typename Demux>
boost::recursive_mutex ServiceOptionFactory<Demux>::service_options_mutex_;

template <typename Demux>
typename ServiceOptionFactory<Demux>::ServiceParserMap
    ServiceOptionFactory<Demux>::service_options_;

}  // ssf

#endif  // SSF_CORE_FACTORIES_SERVICE_OPTION_FACTORY_H_
