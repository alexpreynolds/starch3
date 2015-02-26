#include "starch3.hpp"

// methods

int 
main(int argc, char** argv) 
{
    starch3::Starch::init_command_line_options(argc, argv);

    return EXIT_SUCCESS;
}

void
starch3::Starch::init_command_line_options(int argc, char **argv)
{
#ifdef DEBUG
    fprintf(stderr, "--- starch3::Starch::init_command_line_options() - enter ---\n");
#endif

    int client_long_index;
    int client_opt = getopt_long(argc,
                                 argv,
                                 starch3::Starch::client_opt_string().c_str(),
                                 starch3::Starch::client_long_options(),
                                 &client_long_index);

    opterr = 0; /* disable error reporting by GNU getopt */

    while (client_opt != -1) {
        switch (client_opt) 
            {
            case 'h':
                starch3::Starch::print_usage(stdout);
                exit(EXIT_SUCCESS);
            case 'v':
                starch3::Starch::print_version(stdout);
                exit(EXIT_SUCCESS);
            case '?':
                starch3::Starch::print_usage(stdout);
                exit(EXIT_SUCCESS);
            }        
    }

#ifdef DEBUG
    fprintf(stderr, "--- Starch::init_command_line_options() - exit  ---\n");
#endif
}

void
starch3::Starch::print_usage(FILE *wo_stream)
{
#ifdef DEBUG
    fprintf(stderr, "--- starch3::Starch::print_usage() - enter ---\n");
#endif

    fprintf(wo_stream,
            "%s\n"            \
            "  version: %s\n" \
            "  author:  %s\n" \
            "%s\n"            \
            "%s\n"            \
            "%s\n"            \
            "%s\n",
            starch3::Starch::general_name().c_str(),
            starch3::Starch::version().c_str(),
            starch3::Starch::authors().c_str(),
            starch3::Starch::general_usage().c_str(),
            starch3::Starch::general_description().c_str(),
            starch3::Starch::general_io_options().c_str(), 
            starch3::Starch::general_options().c_str());

#ifdef DEBUG
    fprintf(stderr, "--- starch3::Starch::print_usage() - exit  ---\n");
#endif
}

void
starch3::Starch::print_version(FILE *wo_stream)
{
#ifdef DEBUG
    fprintf(stderr, "--- starch3::Starch::print_version() - enter ---\n");
#endif

    fprintf(wo_stream,
            "%s\n"            \
            "  version: %s\n" \
            "  author:  %s\n",
            starch3::Starch::general_name().c_str(),
            starch3::Starch::version().c_str(),
            starch3::Starch::authors().c_str());

#ifdef DEBUG
    fprintf(stderr, "--- starch3::Starch::print_version() - exit  ---\n");
#endif
}
