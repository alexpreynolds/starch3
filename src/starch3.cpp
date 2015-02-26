#include "starch3.h"

// methods

int 
main(int argc, char** argv) 
{
    starch3::Starch::init_command_line_options(argc, argv);
    starch3::Starch::test_stdin_availability();

    return EXIT_SUCCESS;
}

void
starch3::Starch::test_stdin_availability()
{
#ifdef DEBUG
    fprintf(stderr, "--- starch3::Starch::test_stdin_availability() - enter ---\n");
#endif

    struct stat stats;
    int stats_res;

    if ((stats_res = fstat(STDIN_FILENO, &stats)) == -1) {
        int errsv = errno;
        fprintf(stderr, "Error: fstat() call failed (%s)", (errsv == EBADF ? "EBADF" : (errsv == EIO ? "EIO" : "EOVERFLOW")));
	starch3::Starch::print_usage(stderr);
	std::exit(errsv);
    }
    if ((S_ISCHR(stats.st_mode) == true) && (S_ISREG(stats.st_mode) == false)) {
        fprintf(stderr, "Error: No input is specified; please redirect or pipe in formatted data\n");
	starch3::Starch::print_usage(stderr);
	std::exit(ENODATA); /* No message is available on the STREAM head read queue (POSIX.1) */
    }

#ifdef DEBUG
    fprintf(stderr, "--- starch3::Starch::test_stdin_availability() - leave ---\n");
#endif
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
	    case 'n':
		starch3::Starch::note = optarg;
		break;
            case 'h':
                starch3::Starch::print_usage(stdout);
                exit(EXIT_SUCCESS);
            case 'v':
                starch3::Starch::print_version(stdout);
                exit(EXIT_SUCCESS);
            case '?':
                starch3::Starch::print_usage(stdout);
                exit(EXIT_SUCCESS);
	    default:
		break;
            }        
	client_opt = getopt_long(argc,
                                 argv,
                                 starch3::Starch::client_opt_string().c_str(),
                                 starch3::Starch::client_long_options(),
                                 &client_long_index);
    }

#ifdef DEBUG
    fprintf(stderr, "--- starch3::Starch::init_command_line_options() - leave ---\n");
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
            "%s\n"	      \
            "%s\n"	      \
            "%s\n"	      \
            "%s\n",
            starch3::Starch::general_name().c_str(),
            starch3::Starch::version().c_str(),
            starch3::Starch::authors().c_str(),
            starch3::Starch::general_usage().c_str(),
            starch3::Starch::general_description().c_str(),
            starch3::Starch::general_io_options().c_str(), 
            starch3::Starch::general_options().c_str());

#ifdef DEBUG
    fprintf(stderr, "--- starch3::Starch::print_usage() - leave ---\n");
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
    fprintf(stderr, "--- starch3::Starch::print_version() - leave ---\n");
#endif
}
