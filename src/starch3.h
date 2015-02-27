#ifndef STARCH3_HEADER
#define STARCH3_HEADER

#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <getopt.h>
#include <unistd.h>
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
        std::string _input_fn;
	std::string _note;
	bz_stream* _bz_stream_ptr;
    public:
	Starch();
	~Starch();
        std::string get_input_fn(void);
        void set_input_fn(std::string s);
	std::string get_note(void);
	void set_note(std::string s);
	bz_stream* get_bz_stream_ptr(void);
	void set_bz_stream_ptr(bz_stream** ptr);
	void free_bz_stream_ptr(void);

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
                                  "  $ starch3 [options] < input > output\n" \
                                  "\n"                                  \
                                  "  Or:\n"                             \
                                  "\n"                                  \
                                  "  $ starch3 [options] input > output\n");
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
	static void test_stdin_availability(starch3::Starch& starch);
        static void init_command_line_options(int argc, char** argv, starch3::Starch& starch);
        static void print_usage(FILE* wo_stream);
        static void print_version(FILE* wo_stream);
    };

    std::string Starch::get_input_fn(void) { return _input_fn; }
    void Starch::set_input_fn(std::string s) { _input_fn = s; }
    std::string Starch::get_note(void) { return _note; }
    void Starch::set_note(std::string s) { _note = s; }
    bz_stream* Starch::get_bz_stream_ptr(void) { return _bz_stream_ptr; }
    void Starch::set_bz_stream_ptr(bz_stream **ptr) { _bz_stream_ptr = *ptr; }
    void Starch::free_bz_stream_ptr(void) { free(_bz_stream_ptr); _bz_stream_ptr = NULL; }

    Starch::Starch() {
        std::string _default_input_fn;
        set_input_fn(_default_input_fn);
	std::string _default_note;
	set_note(_default_note);
	bz_stream* bz_stream_ptr = NULL;
	bz_stream_ptr = static_cast<bz_stream *>( malloc(sizeof(bz_stream)) );
	if (!bz_stream_ptr) {
	    fprintf(stderr, "Error: Could not allocate space for bzip2 stream\n");
	    exit(ENOMEM);
	}
	set_bz_stream_ptr(&bz_stream_ptr);
    }

    Starch::~Starch() {
	if (get_bz_stream_ptr())
	    free_bz_stream_ptr();
    }
}

#endif
