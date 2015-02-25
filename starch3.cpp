#include "starch3.hpp"

const std::string starch::Starch::general_name = "starch3";
const std::string starch::Starch::version = S3_VERSION;
const std::string starch::Starch::authors = "Alex Reynolds";
const std::string starch::Starch::general_usage = "\n"                  \
    "  Usage:\n"                                                        \
    "\n"                                                                \
    "  $ starch3 [options] < input > output\n";
const std::string starch::Starch::general_description =                 \
    "  Compress sorted BED data to BEDOPS Starch archive format.\n";
const std::string starch::Starch::general_io_options = "";
const std::string starch::Starch::general_options = "";
const std::string starch::Starch::client_opt_string = "hv?";
struct option starch::Starch::client_long_options[] = {
    { "help",           no_argument,         NULL,    'h' },
    { "version",        no_argument,         NULL,    'w' },
    { NULL,             no_argument,         NULL,     0  }
};

int main(int argc, char** argv) 
{
    starch::Starch::init_command_line_options(argc, argv);

    return EXIT_SUCCESS;
}

void
starch::Starch::init_command_line_options(int argc, char **argv)
{
#ifdef DEBUG
    fprintf(stderr, "--- starch::Starch::init_command_line_options() - enter ---\n");
#endif

    int client_long_index;
    int client_opt = getopt_long(argc,
                                 argv,
                                 starch::Starch::client_opt_string.c_str(),
                                 starch::Starch::client_long_options,
                                 &client_long_index);

    opterr = 0; /* disable error reporting by GNU getopt */

    while (client_opt != -1) {
        switch (client_opt) 
            {
            case 'h':
                starch::Starch::print_usage(stdout);
                exit(EXIT_SUCCESS);
            case 'v':
                starch::Starch::print_version(stdout);
                exit(EXIT_SUCCESS);
            case '?':
                starch::Starch::print_usage(stdout);
                exit(EXIT_SUCCESS);
            }        
    }

#ifdef DEBUG
    fprintf(stderr, "--- starch::Starch::init_command_line_options() - exit  ---\n");
#endif
}

void
starch::Starch::print_usage(FILE *stream)
{
#ifdef DEBUG
    fprintf(stderr, "--- starch::Starch::print_usage() - enter ---\n");
#endif

    fprintf(stream,
            "%s\n"            \
            "  version: %s\n" \
            "  author:  %s\n" \
            "%s\n"            \
            "%s\n"            \
            "%s\n"            \
            "%s\n",
            starch::Starch::general_name.c_str(),
            starch::Starch::version.c_str(),
            starch::Starch::authors.c_str(),
            starch::Starch::general_usage.c_str(),
            starch::Starch::general_description.c_str(),
            starch::Starch::general_io_options.c_str(), 
            starch::Starch::general_options.c_str());

#ifdef DEBUG
    fprintf(stderr, "--- starch::Starch::print_usage() - exit  ---\n");
#endif
}

void
starch::Starch::print_version(FILE *stream)
{
#ifdef DEBUG
    fprintf(stderr, "--- starch::Starch::print_version() - enter ---\n");
#endif

    fprintf(stream,
            "%s\n"            \
            "  version: %s\n" \
            "  author:  %s\n",
            starch::Starch::general_name.c_str(),
            starch::Starch::version.c_str(),
            starch::Starch::authors.c_str());

#ifdef DEBUG
    fprintf(stderr, "--- starch::Starch::print_version() - exit  ---\n");
#endif
}
