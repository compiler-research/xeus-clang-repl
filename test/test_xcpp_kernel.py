#===- test_xpp_kernel.py - Collection of kernel tests for xeus-clang-repl -===//
#
# Licensed under the Apache License v2.0.
# SPDX-License-Identifier: Apache-2.0
#
# The full license is in the file LICENSE, distributed with this software.
#
#===-----------------------------------------------------------------------===//

import unittest
import jupyter_kernel_test


class XCppTests(jupyter_kernel_test.KernelTests):

    kernel_name = 'xcpp14'

    # language_info.name in a kernel_info_reply should match this
    language_name = 'c++'

    # Code in the kernel's language to write "hello, world" to stdout
    code_hello_world = 'extern "C" int printf(const char*,...);auto r1 = printf("hello, world");'

if __name__ == '__main__':
    unittest.main()
