from setuptools import setup, find_packages
from os import path
import shutil

with open(path.join("@CMAKE_SOURCE_DIR@", 'README.md'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name='scattering',
    version='@VERSION@',
    description='Automatic generation of Python modules for C++ code.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/simonpf/scattering',
    author='Simon Pfreundschuh',
    author_email='simon.pfreundschuh@chalmers.se',
    install_requires=[],
    packages=["scattering"],
    python_requires='>=3.6',
    project_urls={
        'Source': 'https://github.com/simonpf/scattering/',
    })
