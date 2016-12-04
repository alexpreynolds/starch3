#ifndef STARCH3_H_
#define STARCH3_H_

#include <string>
#include <vector>
#include <new>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cinttypes>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include "bzlib.h"

#define S3_GENERAL_NAME "starch3"
#define S3_VERSION "0.1"
#define S3_AUTHORS "Alex Reynolds and Shane Neph"

#define S3_BUFFER_LINES 1
#define S3_BUFFER_SIZE 2048

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
            FILE* in_stream;                         // input file stream
        } shared_buffer_t;

        typedef enum bed_token {
            chromosome_token,
            start_token,
            stop_token,
            remainder_token
        } bed_token_t;
        
        typedef struct bed {
            char chr[S3_BUFFER_SIZE];
            char start_str[S3_BUFFER_SIZE];
            uint64_t start;
            char stop_str[S3_BUFFER_SIZE];
            uint64_t stop;
            char id[S3_BUFFER_SIZE];
            char rem[S3_BUFFER_SIZE];
            int token;
        } bed_t;

    private:
        std::string _input_fn;
        std::string _note;
        bz_stream* _bz_stream_ptr;
        FILE* _in_stream;

    public:
        Starch();
        ~Starch();

        pthread_t produce_bed_thread;
        pthread_t consume_bed_thread;
        shared_buffer_t bed_sb;

        void init_sb(starch3::Starch::shared_buffer_t* b);
        void delete_sb(starch3::Starch::shared_buffer_t* b);
        FILE* get_in_stream(void);
        void init_in_stream(void);
        void set_in_stream(FILE* ri_stream);
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
            int i = 0;
            int k = 0;
            shared_buffer_t* b = static_cast<shared_buffer_t*>( arg );
            
            pthread_mutex_lock(&b->lock);
            for (;;) {
                while (b->count == S3_BUFFER_LINES) {
                    pthread_cond_wait(&b->new_space_cond, &b->lock);
                }
                pthread_mutex_unlock(&b->lock);
                k = b->next_in;
                i = 0;
                /* read one line of data into the buffer slot */
                do {  
                    if ((b->c[k][i++] = static_cast<char>( getc(b->in_stream) )) == EOF) {
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
            int i = 0;
            int k = 0;
            int pos = 0;
            char delim = '\t';
            char newline = '\n';
            shared_buffer_t* b = static_cast<shared_buffer_t*>( arg );
            bed_t* bed = NULL;
            
            bed = static_cast<bed_t*>( malloc(sizeof(bed_t)) );
            if (!bed) {
                std::fprintf(stderr, "Error: Could not allocate space for buffer BED line components\n");
                std::exit(ENOMEM);
            }
            bed->token = chromosome_token;
            
            pthread_mutex_lock(&b->lock);
            for (;;) {
                while (b->count == 0)
                    pthread_cond_wait(&b->new_data_cond, &b->lock);
                pthread_mutex_unlock(&b->lock);
                k = b->next_out;
                i = 0;
                bed->chr[pos] = '\0';
                bed->start_str[pos] = '\0';
                bed->stop_str[pos] = '\0';
                bed->id[pos] = '\0';
                bed->rem[pos] = '\0';
                /* process next line of text from the buffer */
                do {
                    if ((b->c[k][i] == delim) && (bed->token != remainder_token)) {
                        pos = 0;
                        bed->token++;
                        i++;
                    }
                    if (b->c[k][i] == EOF) {
                        free(bed);
                        pthread_exit(NULL);
                    }
                    switch (bed->token) {
                    case chromosome_token:
                        bed->chr[pos] = b->c[k][i++];
                        bed->chr[++pos] = '\0';
                        break;
                    case start_token:
                        bed->start_str[pos] = b->c[k][i++];
                        bed->start_str[++pos] = '\0';
                        break;
                    case stop_token:
                        bed->stop_str[pos] = b->c[k][i++];
                        bed->stop_str[++pos] = '\0';
                        break;
                    case remainder_token:
                        bed->rem[pos] = b->c[k][i++];
                        bed->rem[++pos] = '\0';
                        break;
                    }
                } while ((b->c[k][i-1] != newline) && (i < S3_BUFFER_SIZE));
                switch (bed->token) {
                case stop_token:
                    bed->stop_str[--pos] = '\0';
                    break;
                case remainder_token:
                    bed->rem[--pos] = '\0';
                    break;
                }
                sscanf(bed->start_str, "%" SCNu64, &bed->start);
                sscanf(bed->stop_str, "%" SCNu64, &bed->stop);
                fprintf(stdout, "[%s] [%" PRIu64 "] [%" PRIu64 "] [%s]\n", bed->chr, bed->start, bed->stop, bed->rem);
                pos = 0;
                bed->token = chromosome_token;
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
        b->next_in = 0;
        b->next_out = 0;
        b->count = 0;
        pthread_mutex_init(&b->lock, NULL);
        pthread_cond_init(&b->new_data_cond, NULL);
        pthread_cond_init(&b->new_space_cond, NULL);
        b->in_stream = get_in_stream();
    }

    void Starch::delete_sb(starch3::Starch::shared_buffer_t* b) {
        pthread_mutex_destroy(&b->lock);
        pthread_cond_destroy(&b->new_data_cond);
        pthread_cond_destroy(&b->new_space_cond);
        fclose(b->in_stream);
    }

    FILE* Starch::get_in_stream(void) {
        return _in_stream;
    }

    void Starch::init_in_stream(void) {
        FILE* in_fp = NULL;
        in_fp = this->get_input_fn().empty() ? stdin : fopen(this->get_input_fn().c_str(), "r");
        if (!in_fp) {
            std::fprintf(stderr, "Error: Input file handle could not be created\n");
            std::exit(ENODATA); /* No message is available on the STREAM head read queue (POSIX.1) */
        }
        this->set_in_stream(in_fp);
    }

    void Starch::set_in_stream(FILE* ri_stream) {
        _in_stream = ri_stream;
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

    void Starch::bzip2_block_close_static_callback(void* s) {
        reinterpret_cast<starch3::Starch*>(s)->bzip2_block_close_callback();
    }
    
    void Starch::bzip2_block_close_callback(void) {
        std::fprintf(stderr, "callback -> starch3::Starch::bzip2_block_close_callback() called\n");
    }

    void Starch::test_stdin_availability(void) {
        struct stat stats;
        int stats_res;
        
        if ((stats_res = fstat(STDIN_FILENO, &stats)) == -1) {
            int errsv = errno;
            std::fprintf(stderr, "Error: fstat() call failed (%s)", (errsv == EBADF ? "EBADF" : (errsv == EIO ? "EIO" : "EOVERFLOW")));
            this->print_usage(stderr);
            std::exit(errsv);
        }
        if ((S_ISCHR(stats.st_mode) == true) && (S_ISREG(stats.st_mode) == false) && (this->get_input_fn().empty())) {
            std::fprintf(stderr, "Error: No input is specified; please redirect or pipe in formatted data, or specify filename\n");
            this->print_usage(stderr);
            std::exit(ENODATA); /* No message is available on the STREAM head read queue (POSIX.1) */
        }
    }

    Starch::Starch() {
    }

    Starch::~Starch() {
    }

    extern Starch* self;
}

#endif // STARCH3_H_
