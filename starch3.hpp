#ifndef STARCH3_HEADER
#define STARCH3_HEADER

#include <string>
#include <cstdio>
#include <cstdlib>
#include <GetOpt.h>

#define S3_VERSION "0.1"

namespace starch 
{
    class Starch 
    {
    private:
        static const std::string general_name;
        static const std::string version;
        static const std::string authors;
        static const std::string general_usage;
        static const std::string general_description;
        static const std::string general_io_options;
        static const std::string general_options;
        static struct option client_long_options[];
        static const std::string client_opt_string;
    public:
        static void init_command_line_options(int argc, char **argv);
        static void print_usage(FILE *stream);
        static void print_version(FILE *stream);
    };
}

#endif
