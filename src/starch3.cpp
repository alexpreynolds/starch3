#include "starch3api.hpp"

const std::string starch3::Starch::client_name = "starch3";
const std::string starch3::Starch::client_version = "0.1";
const std::string starch3::Starch::client_authors = "Alex Reynolds and Shane Neph";
const starch3::Starch::compression_method_t starch3::Starch::client_starch_default_compression_method = k_bzip2;

// global pointer to state-maintaining instance

starch3::Starch* starch3::self = NULL;

// methods

int 
main(int argc, char** argv) 
{
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::main() - start ---\n");
#endif

    starch3::Starch starch;
    starch3::self = &starch;

    starch.initialize_command_line_options(argc, argv);
    
    starch.test_stdin_availability();
    
    starch.initialize_in_stream();

    starch.initialize_out_stream();

    starch.initialize_shared_buffer(&starch.bed_sb);
    
    pthread_create(&starch.produce_bed_thread, NULL, starch3::Starch::produce_bed, &starch.bed_sb);
    pthread_create(&starch.consume_bed_thread, NULL, starch3::Starch::consume_bed, &starch.bed_sb);
    pthread_create(&starch.chromosome_name_changed_thread, NULL, starch3::Starch::chromosome_name_changed, &starch.bed_sb);
    
    pthread_join(starch.produce_bed_thread, NULL); 
    pthread_join(starch.consume_bed_thread, NULL); 
    pthread_join(starch.chromosome_name_changed_thread, NULL); 
    
    starch.delete_shared_buffer(&starch.bed_sb);

    starch.delete_out_stream();

#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::main() - end ---\n");
#endif

    return EXIT_SUCCESS;
}

std::string
starch3::Starch::get_client_starch_opt_string(void) 
{
    static std::string _s("n:bghv?");
    return _s;
}

struct option* 
starch3::Starch::get_client_starch_long_options(void) 
{
    static struct option _n = { "note",     required_argument,         NULL,    'n' };
    static struct option _b = { "bzip2",          no_argument,         NULL,    'b' };
    static struct option _g = { "gzip",           no_argument,         NULL,    'g' };
    static struct option _h = { "help",           no_argument,         NULL,    'h' };
    static struct option _w = { "version",        no_argument,         NULL,    'w' };
    static struct option _0 = { NULL,             no_argument,         NULL,     0  };
    static std::vector<struct option> _s;
    _s.push_back(_n);
    _s.push_back(_b);
    _s.push_back(_g);
    _s.push_back(_h);
    _s.push_back(_w);
    _s.push_back(_0);
    return &_s[0];
}

void
starch3::Starch::initialize_command_line_options(int argc, char** argv)
{
#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::initialize_command_line_options() ---\n");
#endif

    int client_long_index;
    int client_opt = getopt_long(argc,
                                 argv,
                                 this->get_client_starch_opt_string().c_str(),
                                 this->get_client_starch_long_options(),
                                 &client_long_index);

    opterr = 0; /* disable error reporting by GNU getopt */
    int compression_methods_set = 0;

    while (client_opt != -1) {
        switch (client_opt) {
	    case 'n':
            this->set_note(optarg);
            break;
        case 'b':
            this->set_compression_method(k_bzip2);
            compression_methods_set++;
            break;
        case 'g':
            this->set_compression_method(k_gzip);
            compression_methods_set++;
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
                                 this->get_client_starch_opt_string().c_str(),
                                 this->get_client_starch_long_options(),
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

    if (compression_methods_set > 1) {
        std::fprintf(stderr, "Error: Only one compression method may be set\n");
        this->print_usage(stderr);
        std::exit(EXIT_FAILURE);
    }
    else if (compression_methods_set == 0) {
        this->set_compression_method(client_starch_default_compression_method);
    }
}

std::string 
starch3::Starch::get_client_starch_name(void) 
{
    static std::string _s(starch3::Starch::client_name);
    return _s;
}

std::string 
starch3::Starch::get_client_starch_version(void) 
{
    static std::string _s(starch3::Starch::client_version);
    return _s;
}

std::string 
starch3::Starch::get_client_starch_authors(void) 
{
    static std::string _s(starch3::Starch::client_authors);
    return _s;
}

std::string 
starch3::Starch::get_client_starch_usage(void) 
{
    static std::string _s("\n"                                  \
                          "  Usage:\n"                          \
                          "\n"                                  \
                          "  $ starch3 [options] < input > output\n" \
                          "\n"                                  \
                          "  Or:\n"                             \
                          "\n"                                  \
                          "  $ starch3 [options] input > output\n");
    return _s;
}

std::string 
starch3::Starch::get_client_starch_description(void) 
{
    static std::string _s("  Compress sorted BED data to BEDOPS Starch archive format.\n");
    return _s;
}

std::string
starch3::Starch::get_client_starch_io_options(void) 
{
    static std::string _s("  General Options:\n\n"      \
                          "  --note=\"foo bar...\"   Append note to output archive metadata (optional)\n");
    return _s; 
}
        
std::string
starch3::Starch::get_client_starch_general_options(void) 
{
    static std::string _s("  Process Flags:\n\n"        \
                          "  --help                  Show this usage message\n" \
                          "  --version               Show binary version\n");
    return _s;
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
                 this->get_client_starch_name().c_str(),
                 this->get_client_starch_version().c_str(),
                 this->get_client_starch_authors().c_str(),
                 this->get_client_starch_usage().c_str(),
                 this->get_client_starch_description().c_str(),
                 this->get_client_starch_io_options().c_str(), 
                 this->get_client_starch_general_options().c_str());

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
                 this->get_client_starch_name().c_str(),
                 this->get_client_starch_version().c_str(),
                 this->get_client_starch_authors().c_str());

#ifdef DEBUG
    std::fprintf(stderr, "--- starch3::Starch::print_version() - leave ---\n");
#endif
}
