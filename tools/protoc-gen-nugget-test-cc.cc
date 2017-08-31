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

std::string validateServiceOptions(const ServiceDescriptor& service) {
    if (!service.options().HasExtension(app_id)) {
        return "nugget.protobuf.app_id is not defined for service " + service.name();
    }
    if (!service.options().HasExtension(request_buffer_size)) {
        return "nugget.protobuf.request_buffer_size is not defined for service " + service.name();
    }
    if (!service.options().HasExtension(response_buffer_size)) {
        return "nugget.protobuf.response_buffer_size is not defined for service " + service.name();
    }
    return "";
}

template <typename Descriptor>
std::vector<std::string> Namespaces(const Descriptor& descriptor) {
    std::vector<std::string> namespaces;
    SplitStringUsing(descriptor.full_name(), ".", &namespaces);
    namespaces.pop_back(); // just take the package
    return namespaces;
}

template <typename Descriptor>
std::string FullyQualifiedIdentifier(const Descriptor& descriptor) {
    const auto namespaces = Namespaces(descriptor);
    if (namespaces.empty()) {
        return "::" + descriptor.name();
    } else {
        std::string namespace_path;
        JoinStrings(namespaces, "::", &namespace_path);
        return "::" + namespace_path + "::" + descriptor.name();
    }
}

template <typename Descriptor>
void OpenNamespaces(Printer& printer, const Descriptor& descriptor) {
    const auto namespaces = Namespaces(descriptor);
    for (const auto& ns : namespaces) {
        std::map<std::string, std::string> namespaceVars;
        namespaceVars["namespace"] = ns;
        printer.Print(namespaceVars,
                "namespace $namespace$ {\n");
    }
}

template <typename Descriptor>
void CloseNamespaces(Printer& printer, const Descriptor& descriptor) {
    const auto namespaces = Namespaces(descriptor);
    for (auto it = namespaces.crbegin(); it != namespaces.crend(); ++it) {
        std::map<std::string, std::string> namespaceVars;
        namespaceVars["namespace"] = *it;
        printer.Print(namespaceVars,
                "} // namespace $namespace$\n");
    }
}

void GenerateClientHeader(Printer& printer, const ServiceDescriptor& service) {
    std::map<std::string, std::string> vars;
    vars["include_guard"] = "PROTOC_GENERATED_" + service.name() + "_CLIENT_H";
    vars["protobuf_header"] = StripSuffixString(service.file()->name(), ".proto") + ".pb.h";
    vars["class"] = service.name();
    vars["app_id"] = "APP_ID_" + service.options().GetExtension(app_id);

    printer.Print(vars,
            "#ifndef $include_guard$\n"
            "#define $include_guard$\n"
            "\n"
            "#include <application.h>\n"
            "#include <nos/AppClient.h>\n"
            "#include <nos/NuggetClient.h>\n"
            "\n"
            "#include <$protobuf_header$>\n"
            "\n");

    OpenNamespaces(printer, service);

    printer.Print(vars,
            "\n"
            "class $class$ {\n"
            "::nos::AppClient _app;\n"
            "public:\n"
            "$class$(::nos::NuggetClient& client) : _app{client, $app_id$} {}\n");

    // Methods
    for (int i = 0; i < service.method_count(); ++i) {
        const MethodDescriptor& method = *service.method(i);
        std::map<std::string, std::string> methodVars;
        methodVars["method"] = method.name();
        methodVars["input_type"] = FullyQualifiedIdentifier(*method.input_type());
        methodVars["output_type"] = FullyQualifiedIdentifier(*method.output_type());

        printer.Print(methodVars,
                "uint32_t $method$(const $input_type$&, $output_type$&);\n");
    }

    printer.Print(vars,
            "};\n"
            "\n");

    CloseNamespaces(printer, service);

    printer.Print(vars,
            "\n"
            "#endif\n");
}

void GenerateClientSource(Printer& printer, const ServiceDescriptor& service) {
    std::map<std::string, std::string> vars;
    vars["generated_header"] = service.name() + ".client.h";
    vars["class"] = service.name();

    const uint32_t max_request_size = service.options().GetExtension(request_buffer_size);
    const uint32_t max_response_size = service.options().GetExtension(response_buffer_size);
    vars["max_request_size"] = std::to_string(max_request_size);
    vars["max_response_size"] = std::to_string(max_response_size);
    // TODO: how does the libnos API deal with the size of the response vector?

    printer.Print(vars,
            "#include <$generated_header$>\n"
            "\n"
            "#include <application.h>\n"
            "\n");

    OpenNamespaces(printer, service);

    // Methods
    for (int i = 0; i < service.method_count(); ++i) {
        const MethodDescriptor& method = *service.method(i);
        std::map<std::string, std::string> methodVars{vars};
        methodVars["method"] = method.name();
        methodVars["request_type"] = FullyQualifiedIdentifier(*method.input_type());
        methodVars["response_type"] = FullyQualifiedIdentifier(*method.output_type());
        methodVars["rpc_id"] = std::to_string(i);

        printer.Print(methodVars,
                "\n"
                "uint32_t $class$::$method$(const $request_type$& request,\n"
                "        $response_type$& response) {\n"
                "const size_t request_size = request.ByteSize();\n"
                "if (request_size > $max_request_size$) {\n"
                "return APP_ERROR_TOO_MUCH;\n"
                "}\n"
                "std::vector<uint8_t> buffer(request_size);\n"
                "buffer.reserve($max_response_size$);\n"
                "if (!request.SerializeToArray(buffer.data(), buffer.size())) {\n"
                "return 3;\n" // TODO: not initialized or buffer too small, but we checked that
                "}\n"
                "const uint32_t appStatus =  _app.call($rpc_id$, buffer, buffer);\n"
                "if (!response.ParseFromArray(buffer.data(), buffer.size())) {\n"
                "return 3;\n" // TODO: most likely, it was a badly formed response
                "}\n"
                "return appStatus;\n"
                "}\n");
    }

    printer.Print(vars,
            "\n");

    CloseNamespaces(printer, service);
}

// Generator for C++ Nugget service client
class CppNuggetServiceClientGenerator : public CodeGenerator {
public:
    CppNuggetServiceClientGenerator() = default;
    ~CppNuggetServiceClientGenerator() override = default;

    bool Generate(const FileDescriptor* file,
                  const std::string& parameter,
                  OutputDirectory* output_directory,
                  std::string* error) const override {
        for (int i = 0; i < file->service_count(); ++i) {
            const auto& service = *file->service(i);

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
                        output_directory->Open(service.name() + ".client.cpp")};
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

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    CppNuggetServiceClientGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}