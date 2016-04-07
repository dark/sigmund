/*
 *  SIGnatures Monitor and UNifier Daemon
 *  Copyright (C) 2016  Marco Leogrande
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

namespace freud {
namespace version {

/**
 * Stores the refspec of the current branch.
 * Usually "refs/heads/master".
 */
extern const char g_GIT_REFSPEC[];

/**
 * Stores the full SHA-1 hash (40 characters) of the current Git
 * commit.
 */
extern const char g_GIT_SHA1[];

/**
 * Stores the SHA-1 hash of the current Git commit, shortened to 8
 * characters (as git-log's --abbrev-commit would do).
 */
extern const char g_GIT_SHA1_SHORT[];

/**
 * Stores the result of git-describe. See 'man git-describe' for more
 * details.
 */
extern const char g_GIT_DESCRIPTION[];

/**
 * Stores the name of the first annotated tag that is found by walking
 * back the history of current HEAD (in other words, this is the most
 * recent annotated tag that appears in the history of the current
 * branch).
 */
extern const char g_GIT_TAG_LAST[];

/**
 * If the current commit is pointed at by an annotated tag, then this
 * constant stores the name of that tag. Otherwise, this constant is
 * empty ("").
 *
 * Please note that if this constant is non-empty, then it is also
 * equivalent to g_GIT_TAG_LAST and g_GIT_DESCRIPTION.
 */
extern const char g_GIT_TAG_EXACT[];

}
}
