/*!
  \file utilities.h
  \brief general global utility functions in namespace
  \author Cameron Palmer
  \copyright Released under the MIT License.
  Copyright 2020 Cameron Palmer
*/

#ifndef SBATCH_JOB_MONITOR_UTILITIES_H_
#define SBATCH_JOB_MONITOR_UTILITIES_H_

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sbatch_job_monitor {
/*!
  \brief split a string into tokens based on arbitrary delimiter
  @param s string in need of parsing
  @param vec where the tokenized result will be stored
  @param sep arbitrary delimiter used to denote tokens
  \warning data present in vec will be purged before adding results
*/
void splitline(const std::string &s, std::vector<std::string> *vec,
               const std::string &sep);
// from
// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
int exec(const char *cmd, std::string *screenoutput);
int exec(const std::string &cmd, std::string *screenoutput);
void get_job_ids(const std::string &sjobs, std::map<unsigned, bool> *target);
unsigned get_job_id(const std::string &echo_output);
void kill_job(unsigned jobid);
}  // namespace sbatch_job_monitor

#endif  // SBATCH_JOB_MONITOR_UTILITIES_H_
