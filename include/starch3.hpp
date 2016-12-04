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
#include "jansson.h"

#define S3_GENERAL_NAME "starch3"
#define S3_VERSION "0.1"
#define S3_AUTHORS "Alex Reynolds and Shane Neph"

namespace starch3
{
    class Starch 
    {
        typedef struct bed {
            char* chr;
            size_t chr_capacity;
            char* start_str;
            size_t start_str_capacity;
            uint64_t start;
            char* stop_str;
            size_t stop_str_capacity;
            uint64_t stop;
            char* rem;
            size_t rem_capacity;
            int token;
        } bed_t;

        // cf. http://www.cs.fsu.edu/~baker/opsys/examples/prodcons/prodcons3.c
        typedef struct shared_buffer {
            pthread_mutex_t lock;                    // protects the buffer
            pthread_cond_t new_data_cond;            // to wait when the buffer is empty
            pthread_cond_t new_space_cond;           // to wait when the buffer is full
            char* line;                              // line text
            size_t line_capacity;
            int next_in;                             // next available line for input
            int next_out;                            // next available line for output
            int count;                               // the number of lines occupied
            FILE* in_stream;                         // input file stream
            bed_t* bed;                              // bed
        } shared_buffer_t;

        typedef enum bed_token {
            chromosome_token,
            start_token,
            stop_token,
            remainder_token
        } bed_token_t;

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

        static const int line_min_length = 1024;
        static const int field_min_length = 128;
        static const char field_delimiter = '\t';
        static const char line_delimiter = '\n';
        
        static void* produce_bed(void* arg) {
            size_t line_pos = 0;
            char* new_line = NULL;
            shared_buffer_t* sb = static_cast<shared_buffer_t*>( arg );
            
            pthread_mutex_lock(&sb->lock);
            for (;;) {
                while (sb->count == 1) {
                    pthread_cond_wait(&sb->new_space_cond, &sb->lock);
                }
                pthread_mutex_unlock(&sb->lock);
                line_pos = 0;
                /* read a line of data into the buffer slot */
                do {  
                    if ((line_pos + 1) == sb->line_capacity) {
                        new_line = NULL;
                        new_line = static_cast<char*>( realloc(sb->line, sb->line_capacity * 2) );
                        if (!new_line) {
                            std::fprintf(stderr, "Error: Not enough memory for reallocation of shared_buffer_t line character buffer\n");
                            std::exit(ENOMEM);
                        }
                        sb->line = new_line;
                        sb->line_capacity *= 2;
                    }
                    if ((sb->line[line_pos++] = static_cast<char>( getc(sb->in_stream) )) == EOF) {
                        sb->next_in = (sb->next_in == 0) ? 1 : 0;
                        pthread_mutex_lock(&sb->lock);
                        sb->count++;
                        pthread_mutex_unlock(&sb->lock);
                        pthread_cond_signal(&sb->new_data_cond);
                        pthread_exit(NULL);
                    }
                } while ((sb->line[line_pos-1] != line_delimiter));
                sb->next_in = (sb->next_in == 0) ? 1 : 0;
                pthread_mutex_lock(&sb->lock);
                sb->count++;
                pthread_cond_signal(&sb->new_data_cond);
            }
        }
        
        static void* consume_bed(void* arg) { 
            size_t line_pos = 0;
            size_t elem_pos = 0;
            shared_buffer_t* sb = static_cast<shared_buffer_t*>( arg );
            char* new_field = NULL;
            
            sb->bed->token = chromosome_token;
            
            pthread_mutex_lock(&sb->lock);
            for (;;) {
                while (sb->count == 0) {
                    pthread_cond_wait(&sb->new_data_cond, &sb->lock);
                }
                pthread_mutex_unlock(&sb->lock);
                line_pos = 0;
                sb->bed->chr[elem_pos] = '\0';
                sb->bed->start_str[elem_pos] = '\0';
                sb->bed->stop_str[elem_pos] = '\0';
                sb->bed->rem[elem_pos] = '\0';
                /* process next line of text from the buffer */
                do {
                    if ((sb->line[line_pos] == field_delimiter) && (sb->bed->token != remainder_token)) {
                        elem_pos = 0;
                        sb->bed->token++;
                        line_pos++;
                    }
                    if (sb->line[line_pos] == EOF) {
                        pthread_exit(NULL);
                    }
                    switch (sb->bed->token) {
                    case chromosome_token:
                        if ((elem_pos + 1) == sb->bed->chr_capacity) {
                            new_field = NULL;
                            new_field = static_cast<char*>( realloc(sb->bed->chr, sb->bed->chr_capacity * 2) );
                            if (!new_field) {
                                std::fprintf(stderr, "Error: Not enough memory for reallocation of buffer BED line chromosome field component\n");
                                std::exit(ENOMEM);
                            }
                            sb->bed->chr = new_field;
                            sb->bed->chr_capacity *= 2;
                        }
                        sb->bed->chr[elem_pos] = sb->line[line_pos++];
                        sb->bed->chr[++elem_pos] = '\0';
                        break;
                    case start_token:
                        if ((elem_pos + 1) == sb->bed->start_str_capacity) {
                            new_field = NULL;
                            new_field = static_cast<char*>( realloc(sb->bed->start_str, sb->bed->start_str_capacity * 2) );
                            if (!new_field) {
                                std::fprintf(stderr, "Error: Not enough memory for reallocation of buffer BED line start field component\n");
                                std::exit(ENOMEM);
                            }
                            sb->bed->start_str = new_field;
                            sb->bed->start_str_capacity *= 2;
                        }
                        sb->bed->start_str[elem_pos] = sb->line[line_pos++];
                        sb->bed->start_str[++elem_pos] = '\0';
                        break;
                    case stop_token:
                        if ((elem_pos + 1) == sb->bed->stop_str_capacity) {
                            new_field = NULL;
                            new_field = static_cast<char*>( realloc(sb->bed->stop_str, sb->bed->stop_str_capacity * 2) );
                            if (!new_field) {
                                std::fprintf(stderr, "Error: Not enough memory for reallocation of buffer BED line stop field component\n");
                                std::exit(ENOMEM);
                            }
                            sb->bed->stop_str = new_field;
                            sb->bed->stop_str_capacity *= 2;
                        }
                        sb->bed->stop_str[elem_pos] = sb->line[line_pos++];
                        sb->bed->stop_str[++elem_pos] = '\0';
                        break;
                    case remainder_token:
                        if ((elem_pos + 1) == sb->bed->rem_capacity) {
                            new_field = NULL;
                            new_field = static_cast<char*>( realloc(sb->bed->rem, sb->bed->rem_capacity * 2) );
                            if (!new_field) {
                                std::fprintf(stderr, "Error: Not enough memory for reallocation of buffer BED line remainder field component\n");
                                std::exit(ENOMEM);
                            }
                            sb->bed->rem = new_field;
                            sb->bed->rem_capacity *= 2;
                        }
                        sb->bed->rem[elem_pos] = sb->line[line_pos++];
                        sb->bed->rem[++elem_pos] = '\0';
                        break;
                    }
                } while (sb->line[line_pos-1] != line_delimiter);
                switch (sb->bed->token) {
                case stop_token:
                    sb->bed->stop_str[--elem_pos] = '\0';
                    break;
                case remainder_token:
                    sb->bed->rem[--elem_pos] = '\0';
                    break;
                }
                sscanf(sb->bed->start_str, "%" SCNu64, &sb->bed->start);
                sscanf(sb->bed->stop_str, "%" SCNu64, &sb->bed->stop);
                fprintf(stdout, "Debug: [%s] [%" PRIu64 "] [%" PRIu64 "] [%s]\n", sb->bed->chr, sb->bed->start, sb->bed->stop, sb->bed->rem);
                elem_pos = 0;
                sb->bed->token = chromosome_token;
                sb->next_out = (sb->next_out == 0) ? 1 : 0;
                pthread_mutex_lock(&sb->lock);
                sb->count--;
                pthread_cond_signal(&sb->new_space_cond);
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

    void Starch::init_sb(starch3::Starch::shared_buffer_t* sb) {
        sb->next_in = 0;
        sb->next_out = 0;
        sb->count = 0;
        sb->line = NULL;
        sb->line = static_cast<char*>( malloc(starch3::Starch::line_min_length) );
        if (!sb->line) {
            std::fprintf(stderr, "Error: Not enough memory for shared_buffer_t line character buffer\n");
            std::exit(ENOMEM);
        }
        sb->line_capacity = starch3::Starch::line_min_length;
        sb->bed = static_cast<bed_t*>( malloc(sizeof(bed_t)) );
        if (!sb->bed) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line components\n");
            std::exit(ENOMEM);
        }
        sb->bed->chr = NULL;
        sb->bed->chr = static_cast<char*>( malloc(starch3::Starch::field_min_length) );
        if (!sb->bed->chr) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line chromosome field component\n");
            std::exit(ENOMEM);
        }
        sb->bed->chr_capacity = starch3::Starch::field_min_length;
        sb->bed->start_str = NULL;
        sb->bed->start_str = static_cast<char*>( malloc(starch3::Starch::field_min_length) );
        if (!sb->bed->start_str) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line start field component\n");
            std::exit(ENOMEM);
        }
        sb->bed->start_str_capacity = starch3::Starch::field_min_length;
        sb->bed->stop_str = NULL;
        sb->bed->stop_str = static_cast<char*>( malloc(starch3::Starch::field_min_length) );
        if (!sb->bed->stop_str) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line stop field component\n");
            std::exit(ENOMEM);
        }
        sb->bed->stop_str_capacity = starch3::Starch::field_min_length;
        sb->bed->rem = NULL;
        sb->bed->rem = static_cast<char*>( malloc(starch3::Starch::field_min_length) );
        if (!sb->bed->rem) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line remainder field component\n");
            std::exit(ENOMEM);
        }
        sb->bed->rem_capacity = starch3::Starch::field_min_length;
        pthread_mutex_init(&sb->lock, NULL);
        pthread_cond_init(&sb->new_data_cond, NULL);
        pthread_cond_init(&sb->new_space_cond, NULL);
        sb->in_stream = get_in_stream();
#ifdef DEBUG
        std::fprintf(stderr, "--- starch3::Starch::init_sb() - shared_buffer_t* sb initialized ---\n");
#endif
    }

    void Starch::delete_sb(starch3::Starch::shared_buffer_t* sb) {
        pthread_mutex_destroy(&sb->lock);
        pthread_cond_destroy(&sb->new_data_cond);
        pthread_cond_destroy(&sb->new_space_cond);
        fclose(sb->in_stream);
        if (sb->line) {
            free(sb->line);
            sb->line = NULL;
            sb->line_capacity = 0;
        }
        if (sb->bed) {
            if (sb->bed->chr) {
                free(sb->bed->chr);
                sb->bed->chr = NULL;
                sb->bed->chr_capacity = 0;
            }
            if (sb->bed->start_str) {
                free(sb->bed->start_str);
                sb->bed->start_str = NULL;
                sb->bed->start_str_capacity = 0;
            }
            if (sb->bed->stop_str) {
                free(sb->bed->stop_str);
                sb->bed->stop_str = NULL;
                sb->bed->stop_str_capacity = 0;
            }
            if (sb->bed->rem) {
                free(sb->bed->rem);
                sb->bed->rem = NULL;
                sb->bed->rem_capacity = 0;
            }
        }
#ifdef DEBUG
        std::fprintf(stderr, "--- starch3::Starch::delete_sb() - shared_buffer_t* sb released ---\n");
#endif
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
