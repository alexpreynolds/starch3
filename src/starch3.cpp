#include "starch3.h"

// global pointer to state-maintaining instance

starch3::Starch* starch3::self = NULL;

// methods

int 
main(int argc, char** argv) 
{
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::main() - enter ---\n");
#endif

    starch3::Starch starch;
    starch3::self = &starch;

    starch.init_command_line_options(argc, argv);
    starch.test_stdin_availability();
    starch.init_in_stream();

    //std::fprintf(stderr, "note: [%s]\n", starch.get_note().c_str());
    //std::fprintf(stderr, "inputFn: [%s]\n", starch.get_input_fn().c_str());

    starch.init_bz_stream_ptr();
    starch.setup_bz_stream_callbacks(starch3::self);

    starch.init_sb(&starch.bed_sb);
    pthread_create(&starch.produce_bed_thread, NULL, starch3::Starch::produce_bed, &starch.bed_sb);
    pthread_create(&starch.consume_bed_thread, NULL, starch3::Starch::consume_bed, &starch.bed_sb);
    pthread_join(starch.produce_bed_thread, NULL); 
    pthread_join(starch.consume_bed_thread, NULL); 
    starch.delete_sb(&starch.bed_sb);

    starch.delete_bz_stream_ptr();

#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::main() - leave ---\n");
#endif

    return EXIT_SUCCESS;
}

void
starch3::Starch::init_command_line_options(int argc, char** argv)
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
                this->set_note(optarg);
		break;
            case 'h':
                this->print_usage(stdout);
                std::exit(EXIT_SUCCESS);
            case 'v':
                this->print_version(stdout);
                std::exit(EXIT_SUCCESS);
            case '?':
                this->print_usage(stdout);
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
            if (this->get_input_fn().empty()) {
                this->set_input_fn(argv[optind]);
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
