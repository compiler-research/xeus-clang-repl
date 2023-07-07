# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Xeus-Clang-REPL'
copyright = '2023,Compiler Research'
author = 'Xeus-Clang-REPL Contributors'
release = 'Dev'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = []

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'alabaster'

html_theme_options = {
    "github_user": "Xeus-Clang-REPL Contributors",
    "github_repo": "Xeus-Clang-REPL",
    "github_banner": True,
    "fixed_sidebar": True,
}

highlight_language = "C++"

todo_include_todos = True

mathjax_path = "https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"

# Add latex physics package
mathjax3_config = {
    "loader": {"load": ["[tex]/physics"]},
    "tex": {"packages": {"[+]": ["physics"]}},
}

import os
XEUS_CLANG_REPL_ROOT = os.path.abspath('..')
html_extra_path = [XEUS_CLANG_REPL_ROOT + '/build/docs/']

import subprocess
command = 'mkdir {0}/build; cd {0}/build; cmake ../ -DClang_DIR=/usr/lib/llvm-14/build/lib/cmake/clang\
-DLLVM_DIR=/usr/lib/llvm-14/build/lib/cmake/llvm -DXEUS_CLANG_REPL_INCLUDE_DOCS=ON'.format(XEUS_CLANG_REPL_ROOT)
subprocess.call(command, shell=True)
subprocess.call('doxygen {0}/build/docs/doxygen.cfg'.format(XEUS_CLANG_REPL_ROOT), shell=True)