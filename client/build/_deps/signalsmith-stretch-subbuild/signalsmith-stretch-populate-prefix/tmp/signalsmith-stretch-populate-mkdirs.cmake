# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-src")
  file(MAKE_DIRECTORY "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-build"
  "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-subbuild/signalsmith-stretch-populate-prefix"
  "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-subbuild/signalsmith-stretch-populate-prefix/tmp"
  "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-subbuild/signalsmith-stretch-populate-prefix/src/signalsmith-stretch-populate-stamp"
  "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-subbuild/signalsmith-stretch-populate-prefix/src"
  "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-subbuild/signalsmith-stretch-populate-prefix/src/signalsmith-stretch-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-subbuild/signalsmith-stretch-populate-prefix/src/signalsmith-stretch-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/peetb/Desktop/InGen/client/build/_deps/signalsmith-stretch-subbuild/signalsmith-stretch-populate-prefix/src/signalsmith-stretch-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
