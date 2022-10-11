#############################################################################
# Copyright (c) 2016, Johan Mabille, Loic Gouarin, Sylvain Corlay           #
# Copyright (c) 2016, QuantStack                                            #
#                                                                           #
# Distributed under the terms of the BSD 3-Clause License.                  #
#                                                                           #
# The full license is in the file LICENSE, distributed with this software.  #
#############################################################################

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
