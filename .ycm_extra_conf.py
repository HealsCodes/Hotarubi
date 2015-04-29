import os

flags = [
    '-Wall',
    '-Wextra',
    '-Wpointer-arith',
    '-Wcast-align',
    '-Wwrite-strings',
    '-Wno-long-long',
    '-Wno-variadic-macros',
    '-std=gnu++11',
    '-ffreestanding',
    '-DKERNEL',
    '-D__UINT8_TYPE__=unsigned __INT8_TYPE__',
    '-D__UINT16_TYPE__=unsigned __INT16_TYPE__',
    '-D__UINT32_TYPE__=unsigned __INT32_TYPE__',
    '-D__UINT64_TYPE__=unsigned __INT64_TYPE__',
    '-D__INT_LEAST8_TYPE__=__INT8_TYPE__',
    '-D__INT_LEAST16_TYPE__=__INT16_TYPE__',
    '-D__INT_LEAST32_TYPE__=__INT32_TYPE__',
    '-D__INT_LEAST64_TYPE__=__INT64_TYPE__',
    '-D__INT_FAST8_TYPE__=__INT8_TYPE__',
    '-D__INT_FAST16_TYPE__=__INT16_TYPE__',
    '-D__INT_FAST32_TYPE__=__INT32_TYPE__',
    '-D__INT_FAST64_TYPE__=__INT64_TYPE__',
    '-D__UINT_LEAST8_TYPE__=__UINT8_TYPE__',
    '-D__UINT_LEAST16_TYPE__=__UINT16_TYPE__',
    '-D__UINT_LEAST32_TYPE__=__UINT32_TYPE__',
    '-D__UINT_LEAST64_TYPE__=__UINT64_TYPE__',
    '-D__UINT_FAST8_TYPE__=__UINT8_TYPE__',
    '-D__UINT_FAST16_TYPE__=__UINT16_TYPE__',
    '-D__UINT_FAST32_TYPE__=__UINT32_TYPE__',
    '-D__UINT_FAST64_TYPE__=__UINT64_TYPE__',
    '-D__UINTPTR_TYPE__=unsigned __INTPTR_TYPE__',
    '-I%(project)s/toolchain/include/c++',
    '-I%(project)s/toolchain/lib/gcc/x86_64-elf/4.9.2/include',
    '-I%(project)s/kernel/include',
    '-I.'
]

test_flags = [
    '-I%(project_tests)s',
    '-DRUN_TESTS=1',
    '-x', 'c++',
    '-std=gnu++11'
]


def ProjectPath():
    return os.path.dirname(os.path.abspath(__file__))


def TestsPath():
    return os.path.join(ProjectPath(), 'scripts', 'test')


def FlagsForFile(filename, **kwargs):
    if os.path.splitext(filename)[-1] == '.tt':
        return {
            'flags': [x % {'project_tests': TestsPath()} for x in test_flags],
            'do_cache': True
        }
    return {
        'flags': [x % {'project': ProjectPath()} for x in flags],
        'do_cache': True
    }
