

bin_PROGRAMS = sbatch_job_monitor.out

AM_CXXFLAGS = $(BOOST_CPPFLAGS) -ggdb -Wall -std=c++17

sbatch_job_monitor_out_SOURCES = sbatch_job_monitor/cargs.cc sbatch_job_monitor/cargs.h sbatch_job_monitor/main.cc sbatch_job_monitor/utilities.cc sbatch_job_monitor/utilities.h
sbatch_job_monitor_out_LDADD = $(BOOST_LDFLAGS) -lboost_program_options -lstdc++fs
dist_doc_DATA = README
ACLOCAL_AMFLAGS = -I m4
