/*!
  \file utilities.cc
  \brief implementations of nontemplated global utility functions
  \copyright Released under the MIT License.
  Copyright 2020 Cameron Palmer
 */

#include "sbatch_job_monitor/utilities.h"

void sbatch_job_monitor::splitline(const std::string &s,
                                   std::vector<std::string> *vec,
                                   const std::string &sep) {
  std::string::size_type loc = 0, cur = 0;
  if (!vec)
    throw std::domain_error("splitline: called with null vector pointer");
  vec->clear();
  while (true) {
    loc = s.find(sep, cur);
    if (loc == std::string::npos) {
      vec->push_back(s.substr(cur));
      break;
    }
    vec->push_back(s.substr(cur, loc - cur));
    cur = loc + sep.size();
  }
  return;
}

// from
// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po

int sbatch_job_monitor::exec(const char *cmd, std::string *screenoutput) {
  char buffer[128];
  if (!screenoutput)
    throw std::domain_error("exec: called with null string pointer");
  *screenoutput = "";
  FILE *pipe = 0;
  try {
    pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed");
    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
      *screenoutput += buffer;
    }
    return pclose(pipe);
  } catch (...) {
    pclose(pipe);
    throw;
  }
}

int sbatch_job_monitor::exec(const std::string &cmd,
                             std::string *screenoutput) {
  return exec(cmd.c_str(), screenoutput);
}

void sbatch_job_monitor::get_job_ids(const std::string &sjobs,
                                     std::map<unsigned, bool> *target) {
  if (!target)
    throw std::domain_error("get_job_ids: called with null target pointer");
  std::vector<std::string> lines;
  unsigned jobid = 0;
  std::string catcher = "", jobstat = "";
  target->clear();
  splitline(sjobs, &lines, "\n");
  std::vector<std::string>::const_iterator line = lines.begin();
  for (unsigned i = 0; i < 1; ++i, ++line) {
    if (line == lines.end())
      throw std::domain_error("inadequate total line count in sjobs data: \"" +
                              sjobs + "\"");
  }
  for (; line != lines.end(); ++line) {
    if (line->empty()) continue;
    std::istringstream strm1(*line);
    if (!(strm1 >> catcher >> jobid >> catcher >> catcher >> jobstat))
      throw std::domain_error("cannot parse sjobs line \"" + *line + "\"");
    // report if the job is present and whether it is in a valid run state
    (*target)[jobid] = jobstat.compare("E");
  }
}

unsigned sbatch_job_monitor::get_job_id(const std::string &echo_output) {
  std::istringstream strm1(echo_output);
  std::string catcher = "";
  unsigned res = 0;
  if (!(strm1 >> res))
    throw std::domain_error("cannot parse job id from echo output \"" +
                            echo_output + "\"");
  return res;
}

void sbatch_job_monitor::kill_job(unsigned jobid) {
  std::string command = "scancel " + std::to_string(jobid);
  if (system(command.c_str())) {
    throw std::runtime_error("kill command failed: \"" + command + "\"");
  }
}
