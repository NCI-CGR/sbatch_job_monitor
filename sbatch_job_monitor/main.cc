/*!
  \file main.cc
  \brief main entry/exit for software. interprets command line arguments,
  dispatches tasks, exits \copyright Released under the MIT License. Copyright
  2021 Cameron Palmer
 */

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "sbatch_job_monitor/cargs.h"
#include "sbatch_job_monitor/utilities.h"

/*!
  \brief main program implementation
  @param argc number of command line entries, including program name
  @param argv array of command line entries
 */
int main(int argc, char **argv) {
  sbatch_job_monitor::cargs ap(argc, argv);
  if (ap.help() || argc == 1) {
    ap.print_help(std::cout);
    return 1;
  }
  std::string logging_prefix = ap.get_output_prefix();
  std::string resources = ap.get_resources();
  std::string sbatch_queue = ap.get_queue();
  std::string command_script = ap.get_command_script();
  std::string job_name = ap.get_job_name();
  if (job_name.empty()) {
    if (logging_prefix.rfind("/") != std::string::npos) {
      job_name = logging_prefix.substr(logging_prefix.rfind("/") + 1);
    } else {
      job_name = logging_prefix;
    }
  }
  // for reasons, the job name can't start with a digit
  job_name = job_name.substr(job_name.find_first_not_of("0123456789"));
  if (job_name.empty()) job_name = "bash";
  unsigned sleep_in_seconds = ap.get_sleep_time();
  unsigned crashcheck_interval_in_seconds = ap.get_crashcheck_interval();
  unsigned crashcheck_attempts = ap.get_crashcheck_attempts();
  unsigned eqw_resub_limit = ap.get_eqw_resub_limit();
  unsigned eqw_resubs = 0;
  std::string sbatch_command = "sbatch --output " + logging_prefix +
                               ".output -error " + logging_prefix +
                               ".error --partition " + sbatch_queue + " " +
                               resources + " --no-requeue " + command_script;
  std::string sbatch_screen_output = "";
  std::time_t my_time;
  my_time = std::time(NULL);
  std::cout << "start: " << std::asctime(std::localtime(&my_time)) << std::endl;
  // if this is running, we've made it through a dependency tracker. So any
  // existing success or fail indicator file should be purged
  if (std::filesystem::exists(logging_prefix + ".success")) {
    std::filesystem::remove(logging_prefix + ".success");
  }
  if (std::filesystem::exists(logging_prefix + ".fail")) {
    std::filesystem::remove(logging_prefix + ".fail");
  }
  if (sbatch_job_monitor::exec(sbatch_command, &sbatch_screen_output)) {
    throw std::runtime_error("unable to execute sbatch command: \"" +
                             sbatch_command + "\"");
  }
  std::cout << sbatch_screen_output << std::endl;
  unsigned job_id = sbatch_job_monitor::get_job_id(sbatch_screen_output);
  unsigned seconds_elapsed_since_crashcheck = 0;
  // now, for reasons, wait a random (small) amount of time to try to relieve
  // burden on the system
  //   from multiple concurrent sjobs pings
  std::random_device r;
  std::default_random_engine dre(r());
  std::uniform_int_distribution<int> uid(1, 30);
  sleep(uid(dre));
  // monitoring loop
  while (true) {
    if (std::filesystem::exists(logging_prefix + ".success")) {
      my_time = std::time(NULL);
      std::cout << "end (standard): " << std::asctime(std::localtime(&my_time))
                << std::endl;
      return 0;
    } else if (std::filesystem::exists(logging_prefix + ".fail")) {
      return 2;
    } else {
      sleep(sleep_in_seconds);
    }
    seconds_elapsed_since_crashcheck += sleep_in_seconds;
    // after a certain interval has elapsed, check to be sure the job still
    // exists, and die if not
    if (seconds_elapsed_since_crashcheck >= crashcheck_interval_in_seconds) {
      std::string sjobs_log = "";
      std::map<unsigned, bool> current_job_ids;
      std::map<unsigned, bool>::const_iterator finder;
      unsigned n_crashcheck_retries = 0;
      for (; n_crashcheck_retries < crashcheck_attempts;
           ++n_crashcheck_retries) {
        if (sbatch_job_monitor::exec("sjobs", &sjobs_log)) {
          // wait a while, schedulers tend to have intermittent access issues
          sleep(60);
        } else {
          // parse the output into active job ids
          sbatch_job_monitor::get_job_ids(sjobs_log, &current_job_ids);
          // if the job is still running
          if ((finder = current_job_ids.find(job_id)) !=
                  current_job_ids.end() &&
              finder->second) {
            break;
          } else if (finder != current_job_ids.end()) {
            // the job is still running but Eqw
            sbatch_job_monitor::kill_job(job_id);
            if (eqw_resubs >= eqw_resub_limit) {
              std::cout
                  << "ERROR: job \"" << sbatch_command << "\" (id " << job_id
                  << ") has Eqw, Eqw resubmission limit reached, terminating"
                  << std::endl;
            } else {
              ++eqw_resubs;
              std::cout << "WARNING: job \"" << sbatch_command << "\" (id "
                        << job_id << ") has Eqw, killing and resubmitting"
                        << std::endl;
              if (sbatch_job_monitor::exec(sbatch_command,
                                           &sbatch_screen_output)) {
                throw std::runtime_error(
                    "unable to relaunch "
                    "sbatch command: \"" +
                    sbatch_command + "\"");
              }
              std::cout << "resub (Eqw): " << sbatch_screen_output << std::endl;
              job_id = sbatch_job_monitor::get_job_id(sbatch_screen_output);
              seconds_elapsed_since_crashcheck = 0;
              break;
            }
          } else {  // the job is finished
            // there is a minor possibility that the job finished between when
            // we started the crashcheck and now. check that
            if (std::filesystem::exists(logging_prefix + ".success")) {
              my_time = std::time(NULL);
              std::cout << "end (within crashcheck): "
                        << std::asctime(std::localtime(&my_time)) << std::endl;
              return 0;
            } else if (std::filesystem::exists(logging_prefix + ".fail")) {
              return 2;
            } else {
              // note that there is some degree of desync
              // between jobs finishing and tracking files becoming
              // available there isn't a perfect general purpose
              // solution to this problem...
              std::cout << "warning: job \"" << sbatch_command << "\" (id "
                        << job_id
                        << ") is missing from queue but tracking files "
                        << "have not been written. this is possibly due to "
                        << "filesystem desync..."
                           " waiting to see if the file becomes available"
                        << std::endl;
              sleep(120);
              if (std::filesystem::exists(logging_prefix + ".success")) {
                std::cout << "resolution: job \"" << sbatch_command << "\" (id "
                          << job_id
                          << ") resolved missing tracking files by having "
                             "success appear, exiting normally"
                          << std::endl;
                my_time = std::time(NULL);
                std::cout << "end (within crashcheck): "
                          << std::asctime(std::localtime(&my_time))
                          << std::endl;
                return 0;
              } else if (std::filesystem::exists(logging_prefix + ".fail")) {
                std::cout << "resolution: job \"" << sbatch_command << "\" (id "
                          << job_id
                          << ") resolved missing tracking files by having "
                          << "fail appear, exiting normally "
                          << "(though in failure)" << std::endl;
                return 2;
              } else {  // it crashed without indicating why. resub
                std::cout << "WARNING: job \"" << sbatch_command << "\" (id "
                          << job_id << ") has detected crash, auto-resubmitting"
                          << std::endl;
                if (sbatch_job_monitor::exec(sbatch_command,
                                             &sbatch_screen_output)) {
                  throw std::runtime_error(
                      "unable to relaunch sbatch command: \"" + sbatch_command +
                      "\"");
                }
                std::cout << "resub (job crashed): " << sbatch_screen_output
                          << std::endl;
                job_id = sbatch_job_monitor::get_job_id(sbatch_screen_output);
                seconds_elapsed_since_crashcheck = 0;
                break;
              }
            }
          }
        }
      }
      if (n_crashcheck_retries >= crashcheck_attempts) {
        throw std::runtime_error(
            "in crashcheck, failed sjobs attempts exceeded acceptable "
            "threshold");
      }
    }
  }
}
