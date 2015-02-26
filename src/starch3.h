#ifndef STARCH3_HEADER
#define STARCH3_HEADER

#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <GetOpt.h>
#include "bzlib.h"
#include <sys/stat.h>

#define S3_GENERAL_NAME "starch3"
#define S3_VERSION "0.1"
#define S3_AUTHORS "Alex Reynolds and Shane Neph"

namespace starch3
{
    class Starch 
    {
    private:
    public:
	static std::string note;

        static const std::string& general_name() {
            static std::string _s(S3_GENERAL_NAME);
            return _s;
        }
        static const std::string& version() {
            static std::string _s(S3_VERSION);
            return _s;
        }
        static const std::string& authors() {
            static std::string _s(S3_AUTHORS);
            return _s;
        }
        static const std::string& general_usage() {
            static std::string _s("\n"                                  \
                                  "  Usage:\n"                          \
                                  "\n"                                  \
                                  "  $ starch3 [options] < input > output\n");
            return _s;
        }
        static const std::string& general_description() {
            static std::string _s(                                      \
                                  "  Compress sorted BED data to BEDOPS Starch archive format.\n");
            return _s;
        }
        static const std::string& general_io_options() {
            static std::string _s("  General Options:\n\n"		\
				  "  --note=\"foo bar...\"   Append note to output archive metadata (optional)\n");
            return _s; 
        }
        static const std::string& general_options() {
            static std::string _s("  Process Flags:\n\n"		\
				  "  --help                  Show this usage message\n" \
				  "  --version               Show binary version\n");
            return _s;
        }
        static const std::string& client_opt_string() {
            static std::string _s("n:hv?");
            return _s;
        }
        static const struct option* client_long_options() {
            static struct option _n = { "note",     required_argument,         NULL,    'n' };
            static struct option _h = { "help",           no_argument,         NULL,    'h' };
            static struct option _w = { "version",        no_argument,         NULL,    'w' };
            static struct option _0 = { NULL,             no_argument,         NULL,     0  };
            static std::vector<struct option> _s;
            _s.push_back(_n);
            _s.push_back(_h);
            _s.push_back(_w);
            _s.push_back(_0);
            return &_s[0];
        }
	static void test_stdin_availability();
        static void init_command_line_options(int argc, char **argv);
        static void print_usage(FILE *wo_stream);
        static void print_version(FILE *wo_stream);
    };
    
    std::string Starch::note = "";
}

#endif
