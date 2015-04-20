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
#include <pthread.h>
#include "bzip2/bzlib.h"

#define S3_GENERAL_NAME "starch3"
#define S3_VERSION "0.1"
#define S3_AUTHORS "Alex Reynolds and Shane Neph"

#define S3_BUFFER_LINES 4
#define S3_BUFFER_SIZE 512

namespace starch3
{
    class Starch 
    {
        // cf. http://www.cs.fsu.edu/~baker/opsys/examples/prodcons/prodcons3.c
        typedef struct shared_buffer {
            pthread_mutex_t lock;                    // protects the buffer
            pthread_cond_t new_data_cond;            // to wait when the buffer is empty
            pthread_cond_t new_space_cond;           // to wait when the buffer is full
            char c[S3_BUFFER_LINES][S3_BUFFER_SIZE]; // an array of lines, to hold the text
            int next_in;                             // next available line for input
            int next_out;                            // next available line for output
            int count;                               // the number of lines occupied
        } shared_buffer_t;
        
    private:
        std::string _input_fn;
	std::string _note;
	bz_stream* _bz_stream_ptr;

    public:
	Starch();
	~Starch();

        pthread_t produce_bed_thread;
        pthread_t consume_bed_thread;
        shared_buffer_t sb;

        void init_sb(starch3::Starch::shared_buffer_t* sb);
        std::string get_input_fn(void);
        void set_input_fn(std::string s);
        std::string get_note(void);
        void set_note(std::string s);
        void init_bz_stream_ptr(void);
        void setup_bz_stream_callbacks(starch3::Starch* h);
        void delete_bz_stream_ptr(void);
        void bzip2_block_close_callback(void);
        void init_command_line_options(int argc, char** argv);
	void test_stdin_availability(void);
        void print_usage(FILE* wo_stream);
        void print_version(FILE* wo_stream);
        
        static void* produce_bed(void* arg) {
            int i,k = 0;
            shared_buffer_t *b = static_cast<shared_buffer_t *>( arg );
            
            pthread_mutex_lock(&b->lock);
            for (;;) {
                while (b->count == S3_BUFFER_LINES)
                    pthread_cond_wait(&b->new_space_cond, &b->lock);
                pthread_mutex_unlock(&b->lock);
                k = b->next_in;
                i = 0;
                do {  /* read one line of data into the buffer slot */
                    if ((b->c[k][i++] = static_cast<char>( getc(stdin) )) == EOF) {
                        b->next_in = (b->next_in + 1) % S3_BUFFER_LINES;
                        pthread_mutex_lock(&b->lock);
                        b->count++;
                        pthread_mutex_unlock(&b->lock);
                        pthread_cond_signal(&b->new_data_cond);
                        pthread_exit(NULL);
                    }
                } while ((b->c[k][i-1] != '\n') && (i < S3_BUFFER_SIZE));
                b->next_in = (b->next_in + 1) % S3_BUFFER_LINES;
                pthread_mutex_lock(&b->lock);
                b->count++;
                pthread_cond_signal(&b->new_data_cond);
            }
        }
        
        static void* consume_bed(void* arg) { 
            int i, k = 0;
            shared_buffer_t *b = static_cast<shared_buffer_t *>( arg );
            
            pthread_mutex_lock(&b->lock);
            for (;;) {
                while (b->count == 0)
                    pthread_cond_wait(&b->new_data_cond, &b->lock);
                pthread_mutex_unlock(&b->lock);
                k = b->next_out;
                i = 0;
                do { /* process next line of text from the buffer */
                    if (b->c[k][i] == EOF) {
                        putc('\n', stdout);
                        pthread_exit(NULL);
                    }
                    putc(toupper(b->c[k][i++]), stdout);
                } while ((b->c[k][i-1] != '\n') && (i < S3_BUFFER_SIZE));
                b->next_out = (b->next_out + 1) % S3_BUFFER_LINES;
                pthread_mutex_lock(&b->lock);
                b->count--;
                pthread_cond_signal(&b->new_space_cond);
            }
        }
    
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

	static void bzip2_block_close_static_callback(void* s);
    };

    void Starch::init_sb(starch3::Starch::shared_buffer_t* b) {
        b->next_in = b->next_out = b->count = 0;
        pthread_mutex_init(&b->lock, NULL);
        pthread_cond_init(&b->new_data_cond, NULL);
        pthread_cond_init(&b->new_space_cond, NULL);
    }

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
        // using standard malloc / free routines in bz2 library 
        // cf. http://www.bzip.org/1.0.3/html/low-level.html
        _bz_stream_ptr->bzalloc = NULL;
        _bz_stream_ptr->bzfree = NULL;
        _bz_stream_ptr->opaque = NULL;
        // blockSize100k - 9 (900 kB)
        // verbosity - 0 or 4 (silent or most verbose)
        // workFactor - 30 (default)
#ifdef DEBUG
        int init_res = BZ2_bzCompressInit(_bz_stream_ptr, 9, 4, 30);
#else
        int init_res = BZ2_bzCompressInit(_bz_stream_ptr, 9, 0, 30);
#endif
        switch (init_res) {
        case BZ_CONFIG_ERROR:
            std::fprintf(stderr, "Error: bzip2 initialization failed - library was miscompiled\n");
            std::exit(EINVAL);
        case BZ_PARAM_ERROR:
            std::fprintf(stderr, "Error: bzip2 initialization failed - incorrect parameters\n");
            std::exit(EINVAL);
        case BZ_MEM_ERROR:
            std::fprintf(stderr, "Error: bzip2 initialization failed - insufficient memory\n");
            std::exit(EINVAL);
        case BZ_OK:
#ifdef DEBUG
            std::fprintf(stderr, "--- starch3::Starch::init_bz_stream_ptr() - bz_stream initialized ---\n");
#endif
            break;
        }
    }

    void Starch::setup_bz_stream_callbacks(starch3::Starch* h) {
        _bz_stream_ptr->handler = &h;
        _bz_stream_ptr->block_close_functor = bzip2_block_close_static_callback;
    }

    void Starch::delete_bz_stream_ptr(void) { 
        // release all memory associated with the compression stream before ptr deletion
        int end_res = BZ2_bzCompressEnd(_bz_stream_ptr);
        switch (end_res) {
        case BZ_PARAM_ERROR:
            std::fprintf(stderr, "Error: Could not release internals of bz_stream pointer\n");
            std::exit(EINVAL);
        case BZ_OK:
#ifdef DEBUG
            std::fprintf(stderr, "--- starch3::Starch::delete_bz_stream_ptr() - bz_stream released ---\n");
#endif
            delete _bz_stream_ptr; 
            break;
        }
    }

    Starch::Starch() {
    }

    Starch::~Starch() {
    }

    extern Starch* self;
}

#endif // STARCH3_H_
