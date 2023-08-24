from distutils.core import setup, Extension

module1 = Extension('glebm',
                    sources = ['glebmmodule.c'])

setup (name = 'glEBM',
       version = '1.0',
       description = 'A package for using the glEBM model',
       ext_modules = [module1])

