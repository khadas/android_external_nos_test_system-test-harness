new_http_archive(
    name = "gtest",
    url = "https://github.com/google/googletest/archive/release-1.8.0.zip",
    build_file = "../../core/nugget/BUILD.gtest",
    sha256 = "f3ed3b58511efd272eb074a3a6d6fb79d7c2e6a0e374323d1e6bcbcc1ef141bf",
    strip_prefix = "googletest-release-1.8.0",
)

http_archive(
    name = "com_google_protobuf",
    url = "https://github.com/google/protobuf/archive/v3.3.2.zip",
    strip_prefix = "protobuf-3.3.2",
    sha256 = "c895ad9fd792532f233ced36969d9cc4daec5cb7de9db0d9b26cf06ccd0183c1",
)

http_archive(
    name = "com_google_protobuf_cc",
    url = "https://github.com/google/protobuf/archive/v3.3.2.zip",
    strip_prefix = "protobuf-3.3.2",
    sha256 = "c895ad9fd792532f233ced36969d9cc4daec5cb7de9db0d9b26cf06ccd0183c1",
)

http_archive(
    name = "com_github_gflags_gflags",
    url = "https://github.com/gflags/gflags/archive/v2.2.1.zip",
    strip_prefix = "gflags-2.2.1",
    sha256 = "4e44b69e709c826734dbbbd5208f61888a2faf63f239d73d8ba0011b2dccc97a",
)

http_archive(
    name = "boringssl",
    url = "https://boringssl.googlesource.com/boringssl/+archive/74ffd81aa7ec3d0aa3d3d820dbeda934958ca81a.tar.gz",
    # branch master-with-bazel
)

new_http_archive(
    name = "com_googlesource_android_platform_external_regex_re2",
    url = "https://android.googlesource.com/platform/external/regex-re2/+archive/79cce43a82abc1bc56c65de07a7df47d54e163a9.tar.gz",
    build_file_content = """
cc_library(
    name = "regex_re2",
    srcs = [
        "re2/bitstate.cc",
        "re2/compile.cc",
        "re2/dfa.cc",
        "re2/filtered_re2.cc",
        "re2/filtered_re2.h",
        "re2/mimics_pcre.cc",
        "re2/nfa.cc",
        "re2/onepass.cc",
        "re2/parse.cc",
        "re2/perl_groups.cc",
        "re2/prefilter.cc",
        "re2/prefilter.h",
        "re2/prefilter_tree.cc",
        "re2/prefilter_tree.h",
        "re2/prog.cc",
        "re2/prog.h",
        "re2/re2.cc",
        "re2/re2.h",
        "re2/regexp.cc",
        "re2/regexp.h",
        "re2/set.cc",
        "re2/set.h",
        "re2/simplify.cc",
        "re2/stringpiece.h",
        "re2/testing/exhaustive_tester.h",
        "re2/testing/regexp_generator.h",
        "re2/testing/string_generator.h",
        "re2/testing/tester.h",
        "re2/tostring.cc",
        "re2/unicode_casefold.cc",
        "re2/unicode_casefold.h",
        "re2/unicode_groups.cc",
        "re2/unicode_groups.h",
        "re2/variadic_function.h",
        "re2/walker-inl.h",
        "util/arena.cc",
        "util/arena.h",
        "util/atomicops.h",
        "util/benchmark.h",
        "util/flags.h",
        "util/hash.cc",
        "util/logging.h",
        "util/mutex.h",
        "util/pcre.h",
        "util/random.h",
        "util/rune.cc",
        "util/sparse_array.h",
        "util/sparse_set.h",
        "util/stringpiece.cc",
        "util/stringprintf.cc",
        "util/strutil.cc",
        "util/test.h",
        "util/utf.h",
        "util/util.h",
        "util/valgrind.cc",
        "util/valgrind.h",
    ],
    hdrs = [
        "re2/filtered_re2.h",
        "re2/re2.h",
        "re2/set.h",
        "re2/stringpiece.h",
        "re2/variadic_function.h",
    ],
    visibility = ["//visibility:public"],
)
""",
)

new_local_repository(
    name = "libmpsse",
    path = "../../third_party/libmpsse/src",
    build_file_content = """
cc_library(
    name = "libmpsse",
    srcs = [
        "fast.c",
        "mpsse.c",
        "support.c",
    ],
    hdrs = [
        "config.h",
        "mpsse.h",
        "support.h",
    ],
    copts = [
        "-Wall",
        "-fPIC",
        "-fno-strict-aliasing",
        "-g",
        "-O2",
    ],
    defines = [
        "LIBFTDI1=0",
    ],
    linkopts = [
        "-lftdi",
    ],
    visibility = ["//visibility:public"],
)
""",
)

## This is the rule for when this repository is outside of repo.
# new_git_repository(
#     name = "ahdlc",
#     remote = "https://nugget-os.googlesource.com/third_party/ahdlc",
# )

## Use this when a subproject of repo.
new_local_repository(
    name = "ahdlc",
    path = "../../third_party/ahdlc",
    build_file_content = """
cc_library(
    name = "ahdlc",
    srcs = [
        "src/lib/crc_16.c",
        "src/lib/frame_layer.c",
    ],
    hdrs = [
        "src/lib/inc/crc_16.h",
        "src/lib/inc/frame_layer.h",
        "src/lib/inc/frame_layer_types.h",
    ],
    visibility = ["//visibility:public"],
)
""",
)

local_repository(
    name = "nugget",
    path = "../../core/nugget",
)

local_repository(
    name = "proto",
    path = "proto",
)

local_repository(
    name = "tools",
    path = "tools",
)
