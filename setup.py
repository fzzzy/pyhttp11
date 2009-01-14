

from distutils.core import setup, Extension

setup(
    name='pyhttp11',
    version='1.0',
    ext_modules=[
        Extension(
            'pyhttp11',
            ['pyhttp11.c', 'http11_parser.c'],
        )
    ],
)
