/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// TODO(b/65356499) Replace this with the libnos generator if possible

#include <map>
#include <string>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/strutil.h>

#include "nugget/protobuf/options.pb.h"

using ::google::protobuf::FileDescriptor;
using ::google::protobuf::JoinStrings;
using ::google::protobuf::MethodDescriptor;
using ::google::protobuf::ServiceDescriptor;
using ::google::protobuf::SplitStringUsing;
using ::google::protobuf::StripSuffixString;
using ::google::protobuf::compiler::CodeGenerator;
using ::google::protobuf::compiler::OutputDirectory;
using ::google::protobuf::io::Printer;
using ::google::protobuf::io::ZeroCopyOutputStream;

using ::nugget::protobuf::app_id;
using ::nugget::protobuf::request_buffer_size;
using ::nugget::protobuf::response_buffer_size;

namespace {

std::string validateServiceOptions(const ServiceDescriptor &service) {
  if (!service.options().HasExtension(app_id)) {
    return "nugget.protobuf.app_id is not defined for service "
        + service.name();
  }
  if (!service.options().HasExtension(request_buffer_size)) {
    return "nugget.protobuf.request_buffer_size is not defined for service "
        + service.name();
  }
  if (!service.options().HasExtension(response_buffer_size)) {
    return "nugget.protobuf.response_buffer_size is not defined for service "
        + service.name();
  }
  return "";
}

template<typename Descriptor>
std::vector<std::string> Namespaces(const Descriptor &descriptor) {
  std::vector<std::string> namespaces;
  SplitStringUsing(descriptor.full_name(), ".", &namespaces);
  namespaces.pop_back(); // just take the package
  return namespaces;
}

template<typename Descriptor>
std::string FullyQualifiedIdentifier(const Descriptor &descriptor) {
  const auto namespaces = Namespaces(descriptor);
  if (namespaces.empty()) {
    return "::" + descriptor.name();
  } else {
    std::string namespace_path;
    JoinStrings(namespaces, "::", &namespace_path);
    return "::" + namespace_path + "::" + descriptor.name();
  }
}

template<typename Descriptor>
void OpenNamespaces(Printer &printer, const Descriptor &descriptor) {
  const auto namespaces = Namespaces(descriptor);
  for (const auto &ns : namespaces) {
    std::map<std::string, std::string> namespaceVars;
    namespaceVars["namespace"] = ns;
    printer.Print(namespaceVars,
                  "namespace $namespace$ {\n");
  }
}

template<typename Descriptor>
void CloseNamespaces(Printer &printer, const Descriptor &descriptor) {
  const auto namespaces = Namespaces(descriptor);
  for (auto it = namespaces.crbegin(); it != namespaces.crend(); ++it) {
    std::map<std::string, std::string> namespaceVars;
    namespaceVars["namespace"] = *it;
    printer.Print(namespaceVars,
                  "} // namespace $namespace$\n");
  }
}

void GenerateClientHeader(Printer &printer, const ServiceDescriptor &service) {
  std::map<std::string, std::string> vars;
  vars["include_guard"] = "PROTOC_GENERATED_" + service.name() + "_CLIENT_H";
  vars["protobuf_header"] =
      StripSuffixString(service.file()->name(), ".proto") + ".pb.h";
  vars["class"] = service.name();

  printer.Print(vars, R"(
#ifndef $include_guard$
#define $include_guard$

#include "$protobuf_header$"

#include "nugget_driver.h"
)");

  OpenNamespaces(printer, service);

  printer.Print(vars, R"(
class $class$ {
 public:
  $class$() {}
)");

  // Methods
  for (int i = 0; i < service.method_count(); ++i) {
    const MethodDescriptor &method = *service.method(i);
    std::map<std::string, std::string> methodVars;
    methodVars["method"] = method.name();
    methodVars["input_type"] = FullyQualifiedIdentifier(*method.input_type());
    methodVars["output_type"] = FullyQualifiedIdentifier(*method.output_type());

    printer.Print(methodVars, R"(
  uint32_t $method$(const $input_type$&, $output_type$&);)");
  }

  printer.Print(vars, R"(
};
)");

  CloseNamespaces(printer, service);

  printer.Print(vars, R"(
#endif
)");
}

void GenerateClientSource(Printer &printer, const ServiceDescriptor &service) {
  std::map<std::string, std::string> vars;
  vars["generated_header"] = service.name() + ".client.h";
  vars["class"] = service.name();

  const uint32_t
      max_request_size = service.options().GetExtension(request_buffer_size);
  const uint32_t
      max_response_size = service.options().GetExtension(response_buffer_size);
  vars["max_request_size"] = std::to_string(max_request_size);
  vars["max_response_size"] = std::to_string(max_response_size);

  printer.Print(vars,  R"(#include "$generated_header$"

#include <iostream>
)");

  OpenNamespaces(printer, service);

  // Methods
  for (int i = 0; i < service.method_count(); ++i) {
    const MethodDescriptor &method = *service.method(i);
    std::map<std::string, std::string> methodVars{vars};
    methodVars["method"] = method.name();
    methodVars["request_type"] = FullyQualifiedIdentifier(*method.input_type());
    methodVars["response_type"] =
        FullyQualifiedIdentifier(*method.output_type());
    methodVars["rpc_id"] = std::to_string(i);
    methodVars["app_id"] = "APP_ID_" + service.options().GetExtension(app_id);

    printer.Print(
        methodVars, R"(
uint32_t $class$::$method$(
    const $request_type$& request,
    $response_type$& response) {
  const size_t request_size = request.ByteSize();
  if (request_size > $max_request_size$) {
    return APP_ERROR_TOO_MUCH;
  }
  std::vector<uint8_t> buffer(request_size);
  buffer.reserve($max_response_size$);
  if (!request.SerializeToArray(buffer.data(), buffer.size())) {
    return APP_ERROR_RPC;
  }
  if (!nugget_driver::OpenDevice()) {
    return APP_ERROR_RPC;
  }
  uint32_t response_size = buffer.capacity();
  std::cout << "SENDING: app_id = " << $app_id$ << ", param = "
            << $rpc_id$ << ", length = " << request_size << "\n";
  const uint32_t appStatus =  call_application(
      $app_id$, $rpc_id$,
      buffer.data(), request_size, buffer.data(), &response_size);
  std::cout << "RECEIVED: " << response_size << " bytes\n";

  /* Prints the raw response to console to make debugging SPI app communication
   * easier. Ideally this generator will eventually be replaced by libnos.
   */
  std::cout << "0x";
  for (size_t x = 0; x < response_size; ++x) {
    if (buffer[x] < 16)
      std::cout << "0";
    std::cout << std::hex << (uint16_t) buffer[x];
  }

  std::cout << std::dec << "\n";
  std::cout.flush();
  if (!response.ParseFromArray(buffer.data(), response_size)) {
    return APP_ERROR_RPC;
  }
  return appStatus;
}
)");
  }

  printer.Print(vars,
                "\n");

  CloseNamespaces(printer, service);
}

// Generator for C++ Nugget service client
class CppNuggetServiceClientGenerator: public CodeGenerator {
 public:
  CppNuggetServiceClientGenerator() = default;
  ~CppNuggetServiceClientGenerator() override = default;

  bool Generate(const FileDescriptor *file,
                const std::string &parameter,
                OutputDirectory *output_directory,
                std::string *error) const override {
    for (int i = 0; i < file->service_count(); ++i) {
      const auto &service = *file->service(i);

      *error = validateServiceOptions(service);
      if (!error->empty()) {
        return false;
      }

      if (parameter == "header") {
        std::unique_ptr<ZeroCopyOutputStream> output{
            output_directory->Open(service.name() + ".client.h")};
        Printer printer(output.get(), '$');
        GenerateClientHeader(printer, service);
      } else if (parameter == "source") {
        std::unique_ptr<ZeroCopyOutputStream> output{
            output_directory->Open(service.name() + ".client.cc")};
        Printer printer(output.get(), '$');
        GenerateClientSource(printer, service);
      } else {
        *error = "Illegal parameter: must be header|source";
        return false;
      }
    }

    return true;
  }

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CppNuggetServiceClientGenerator);
};

} // namespace

int main(int argc, char *argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  CppNuggetServiceClientGenerator generator;
  return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
