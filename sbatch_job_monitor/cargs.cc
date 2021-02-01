/*!
  \file cargs.cc
  \brief method implementation for command line argument parser class
  \copyright Released under the MIT License.
  Copyright 2021 Cameron Palmer
*/

#include "sbatch_job_monitor/cargs.h"

void sbatch_job_monitor::cargs::initialize_options() {
  _desc.add_options()("help,h", "emit this help message")(
      "output-prefix,o", boost::program_options::value<std::string>(),
      "prefix of all sbatch output files")(
      "job-name,j",
      boost::program_options::value<std::string>()->default_value(""),
      "name of submitted job (optional)")(
      "resources,r",
      boost::program_options::value<std::string>()->default_value(
          "--time=2:00:00 --mem=17g"),
      "resource request to sbatch")(
      "queue,q",
      boost::program_options::value<std::string>()->default_value("norm"),
      "sbatch queue")("command-script,c",
                      boost::program_options::value<std::string>(),
                      "name of script to run")(
      "sleep-time,t",
      boost::program_options::value<unsigned>()->default_value(10),
      "number of seconds to sleep between checks")(
      "crashcheck-interval,i",
      boost::program_options::value<unsigned>()->default_value(3600),
      "number of seconds to wait before checking queue for job existence")(
      "crashcheck-attempts,a",
      boost::program_options::value<unsigned>()->default_value(10),
      "number of times to attempt sjobs before killing process")(
      "error-resub-limit,e",
      boost::program_options::value<unsigned>()->default_value(3),
      "number of times to attempt resubbing borked job before killing process");
}
