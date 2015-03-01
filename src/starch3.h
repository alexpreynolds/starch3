#ifndef STARCH3_H_
#define STARCH3_H_

#include <string>
#include <vector>
#include <new>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include "bzip2/bzlib.h"

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
        void init_bz_stream_ptr(void);
        void delete_bz_stream_ptr(void);

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
	static void bzip2_block_close_callback(void);
	static void test_stdin_availability(starch3::Starch& starch);
        static void init_command_line_options(int argc, char** argv, starch3::Starch& starch);
        static void print_usage(FILE* wo_stream);
        static void print_version(FILE* wo_stream);
    };

    std::string Starch::get_input_fn(void) {
        return _input_fn;
    }
    
    void Starch::set_input_fn(std::string s) {
        struct stat buf;
        if (stat(s.c_str(), &buf) == 0) {
            _input_fn = s;
        }
        else {
            std::fprintf(stderr, "Error: Input file does not exist (%s)\n", s.c_str());
            std::exit(ENODATA); /* No message is available on the STREAM head read queue (POSIX.1) */
        }
    }
    
    std::string Starch::get_note(void) {
        return _note;
    }
    
    void Starch::set_note(std::string s) {
        _note = s;
    }

    void Starch::init_bz_stream_ptr(void) { 
        try {
            _bz_stream_ptr = new bz_stream; 
        }
        catch (std::bad_alloc& ba) {
            std::fprintf(stderr, "Error: Could not allocate space for bz_stream pointer (%s)\n", ba.what());
        }
        // set callback function pointer
        _bz_stream_ptr->block_close_functor = bzip2_block_close_callback;
        // using standard malloc / free routines in bz2 library 
        // cf. http://www.bzip.org/1.0.3/html/low-level.html
        _bz_stream_ptr->bzalloc = NULL;
        _bz_stream_ptr->bzfree = NULL;
        _bz_stream_ptr->opaque = NULL;
        // blockSize100k - 9 (900 kB)
        // verbosity - 4 (most verbose)
        // workFactor - 30 (default)
        BZ2_bzCompressInit(_bz_stream_ptr, 9, 4, 30);
    }

    void Starch::delete_bz_stream_ptr(void) { 
        // release all memory associated with the compression stream
        if (BZ2_bzCompressEnd(_bz_stream_ptr) == BZ_OK) {
            delete _bz_stream_ptr; 
        }
        else {
            std::fprintf(stderr, "Error: Could not release internals of bz_stream pointer\n");
            std::exit(EINVAL);
        }
    }

    Starch::Starch() {
        init_bz_stream_ptr();
    }

    Starch::~Starch() {
        delete_bz_stream_ptr();
    }
}

#endif // STARCH3_H_
