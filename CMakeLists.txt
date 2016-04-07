cmake_minimum_required(VERSION 2.8)
project(freud)

# include common cmake definitions
include(cmake/common.cmake)
include(cmake/GetGitRevisionDescription.cmake)

### versioning information
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING "${GIT_SHA1}" 0 8 GIT_SHA1_SHORT)
git_describe(GIT_DESCRIPTION)
git_describe(GIT_TAG_LAST "--abbrev=0")
git_get_exact_tag(GIT_TAG_EXACT)
if(GIT_TAG_EXACT)
  message(STATUS "Currently on Git tag ${GIT_TAG_EXACT}")
else ()
  message(STATUS "Latest recognized Git tag is ${GIT_TAG_LAST}")
  message(STATUS "Current version is ${GIT_DESCRIPTION}")
  set(GIT_TAG_EXACT "")
endif()
message(STATUS "Git HEAD is ${GIT_SHA1}")
################################

add_subdirectory(src)
