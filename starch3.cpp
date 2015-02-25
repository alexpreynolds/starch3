#include "starch3.hpp"

using namespace starch;

const std::string Starch::general_name = "starch3";
const std::string Starch::version = S3_VERSION;
const std::string Starch::authors = "Alex Reynolds";
const std::string Starch::general_usage = "\n"                  \
    "  Usage:\n"                                                        \
    "\n"                                                                \
    "  $ starch3 [options] < input > output\n";
const std::string Starch::general_description =                 \
    "  Compress sorted BED data to BEDOPS Starch archive format.\n";
const std::string Starch::general_io_options = "";
const std::string Starch::general_options = "";
const std::string Starch::client_opt_string = "hv?";
struct option Starch::client_long_options[] = {
    { "help",           no_argument,         NULL,    'h' },
    { "version",        no_argument,         NULL,    'w' },
    { NULL,             no_argument,         NULL,     0  }
};

int main(int argc, char** argv) 
{
    Starch::init_command_line_options(argc, argv);

    return EXIT_SUCCESS;
}

void
Starch::init_command_line_options(int argc, char **argv)
{
#ifdef DEBUG
    fprintf(stderr, "--- Starch::init_command_line_options() - enter ---\n");
#endif

    int client_long_index;
    int client_opt = getopt_long(argc,
                                 argv,
                                 Starch::client_opt_string.c_str(),
                                 Starch::client_long_options,
                                 &client_long_index);

    opterr = 0; /* disable error reporting by GNU getopt */

    while (client_opt != -1) {
        switch (client_opt) 
            {
            case 'h':
                Starch::print_usage(stdout);
                exit(EXIT_SUCCESS);
            case 'v':
                Starch::print_version(stdout);
                exit(EXIT_SUCCESS);
            case '?':
                Starch::print_usage(stdout);
                exit(EXIT_SUCCESS);
            }        
    }

#ifdef DEBUG
    fprintf(stderr, "--- Starch::init_command_line_options() - exit  ---\n");
#endif
}

void
Starch::print_usage(FILE *stream)
{
#ifdef DEBUG
    fprintf(stderr, "--- Starch::print_usage() - enter ---\n");
#endif

    fprintf(stream,
            "%s\n"            \
            "  version: %s\n" \
            "  author:  %s\n" \
            "%s\n"            \
            "%s\n"            \
            "%s\n"            \
            "%s\n",
            Starch::general_name.c_str(),
            Starch::version.c_str(),
            Starch::authors.c_str(),
            Starch::general_usage.c_str(),
            Starch::general_description.c_str(),
            Starch::general_io_options.c_str(), 
            Starch::general_options.c_str());

#ifdef DEBUG
    fprintf(stderr, "--- Starch::print_usage() - exit  ---\n");
#endif
}

void
Starch::print_version(FILE *stream)
{
#ifdef DEBUG
    fprintf(stderr, "--- Starch::print_version() - enter ---\n");
#endif

    fprintf(stream,
            "%s\n"            \
            "  version: %s\n" \
            "  author:  %s\n",
            Starch::general_name.c_str(),
            Starch::version.c_str(),
            Starch::authors.c_str());

#ifdef DEBUG
    fprintf(stderr, "--- Starch::print_version() - exit  ---\n");
#endif
}
