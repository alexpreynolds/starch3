#include "starch3.h"

// methods

int 
main(int argc, char** argv) 
{
    starch3::Starch starch;

    starch3::Starch::init_command_line_options(argc, argv, starch);
    starch3::Starch::test_stdin_availability(starch);

    std::fprintf(stderr, "note: [%s]\n", starch.get_note().c_str());
    std::fprintf(stderr, "inputFn: [%s]\n", starch.get_input_fn().c_str());

    return EXIT_SUCCESS;
}

void
starch3::Starch::bzip2_block_close_callback(void)
{
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::bzip2_block_close_callback() - enter ---\n");
#endif

    std::fprintf(stderr, "callback -> starch3::Starch::bzip2_block_close_callback() called\n");

#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::bzip2_block_close_callback() - leave ---\n");
#endif
}

void
starch3::Starch::test_stdin_availability(starch3::Starch& starch)
{
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::test_stdin_availability() - enter ---\n");
#endif

    struct stat stats;
    int stats_res;

    if ((stats_res = fstat(STDIN_FILENO, &stats)) == -1) {
        int errsv = errno;
        std::fprintf(stderr, "Error: fstat() call failed (%s)", (errsv == EBADF ? "EBADF" : (errsv == EIO ? "EIO" : "EOVERFLOW")));
	starch3::Starch::print_usage(stderr);
	std::exit(errsv);
    }
    if ((S_ISCHR(stats.st_mode) == true) && (S_ISREG(stats.st_mode) == false) && (starch.get_input_fn().empty())) {
        std::fprintf(stderr, "Error: No input is specified; please redirect or pipe in formatted data, or specify filename\n");
	starch3::Starch::print_usage(stderr);
	std::exit(ENODATA); /* No message is available on the STREAM head read queue (POSIX.1) */
    }

#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::test_stdin_availability() - leave ---\n");
#endif
}

void
starch3::Starch::init_command_line_options(int argc, char** argv, starch3::Starch& starch)
{
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::init_command_line_options() - enter ---\n");
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
                starch.set_note(optarg);
		break;
            case 'h':
                starch3::Starch::print_usage(stdout);
                std::exit(EXIT_SUCCESS);
            case 'v':
                starch3::Starch::print_version(stdout);
                std::exit(EXIT_SUCCESS);
            case '?':
                starch3::Starch::print_usage(stdout);
                std::exit(EXIT_SUCCESS);
	    default:
		break;
            }        
	client_opt = getopt_long(argc,
                                 argv,
                                 starch3::Starch::client_opt_string().c_str(),
                                 starch3::Starch::client_long_options(),
                                 &client_long_index);
    }

    if (optind < argc) {
        do {
            if (starch.get_input_fn().empty()) {
                starch.set_input_fn(argv[optind]);
            }
            else {
                std::fprintf(stderr, "Warning: Ignoring additional input file [%s]\n", argv[optind]);
            }
        }
        while (++optind < argc);
    }
    
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::init_command_line_options() - leave ---\n");
#endif
}

void
starch3::Starch::print_usage(FILE* wo_stream)
{
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::print_usage() - enter ---\n");
#endif

    std::fprintf(wo_stream,
                 "%s\n"       \
                 "  version: %s\n"              \
                 "  author:  %s\n"              \
                 "%s\n"                         \
                 "%s\n"                         \
                 "%s\n"                         \
                 "%s\n",
                 starch3::Starch::general_name().c_str(),
                 starch3::Starch::version().c_str(),
                 starch3::Starch::authors().c_str(),
                 starch3::Starch::general_usage().c_str(),
                 starch3::Starch::general_description().c_str(),
                 starch3::Starch::general_io_options().c_str(), 
                 starch3::Starch::general_options().c_str());

#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::print_usage() - leave ---\n");
#endif
}

void
starch3::Starch::print_version(FILE* wo_stream)
{
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::print_version() - enter ---\n");
#endif

    std::fprintf(wo_stream,
                 "%s\n"       \
                 "  version: %s\n"              \
                 "  author:  %s\n",
                 starch3::Starch::general_name().c_str(),
                 starch3::Starch::version().c_str(),
                 starch3::Starch::authors().c_str());
    
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::print_version() - leave ---\n");
#endif
}
