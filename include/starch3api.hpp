#ifndef STARCH3_H_
#define STARCH3_H_

#include <string>
#include <vector>
#include <new>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cinttypes>
#include <cstring>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include "bzlib.h"
#include "jansson.h"

namespace starch3
{
    class Starch 
    {
        typedef enum compression_method {
            k_bzip2 = 0,
            k_gzip,
            k_compression_method_undefined
        } compression_method_t;

        typedef enum bed_token {
            k_chromosome_token = 0,
            k_start_token,
            k_stop_token,
            k_remainder_token,
            k_bed_token_undefined
        } bed_token_t;

        typedef struct bed {
            char*   chr;
            size_t  chr_capacity;
            char*   start_str;
            size_t  start_str_capacity;
            int64_t start;
            char*   stop_str;
            size_t  stop_str_capacity;
            int64_t stop;
            char*   rem;
            size_t  rem_capacity;
            int     token;
        } bed_t;

        typedef struct transform_state {
            int64_t line_count;
            char*   last_chr;
            int64_t last_start;
            int64_t last_stop;
            int64_t last_coord_diff;
            char*   current_chr;
            int64_t current_start;
            int64_t current_stop;
            int64_t current_coord_diff;
            int64_t base_count_unique;
            int64_t base_count_nonunique;
        } transform_state_t;

        // cf. http://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf
        typedef struct shared_buffer {
            pthread_mutex_t lock;                       // protects the buffer and transformation state
            pthread_cond_t new_line_is_available;       // to note when the buffer has raw BED data ready to process
            pthread_cond_t new_line_is_empty;           // to note when the buffer is empty and ready to be filled with raw BED data
            pthread_cond_t new_chromosome_is_available; // to note when a record is parsed that contains a new chromosome field name
            char* in_line;                              // input line text
            size_t in_line_capacity;                    // input line capacity
            int next_in;                                // next available line for input
            int next_out;                               // next available line for output
            bool is_new_line_available;                 // is line occupied?
            bool is_new_chromosome_available;           // is new chromosome available?
            bool is_eof;                                // are we at the end of the input file stream?
            FILE* in_stream;                            // input file stream
            bed_t* bed;                                 // raw BED field components
            transform_state_t* tf_state;                // transformed BED state components
            char* tf_line;                              // tf line text
            size_t tf_line_capacity;                    // tf line text capacity
            char* tf_buffer;
            size_t tf_buffer_capacity;
            size_t tf_buffer_size;
        } shared_buffer_t;

    private:
        std::string _input_fn;
        std::string _note;
        bz_stream* _bz_stream_ptr;
        FILE* _in_stream;
        compression_method_t _compression_method;

    public:
        Starch();
        ~Starch();

        pthread_t produce_line_thread;
        pthread_t consume_line_thread;
        pthread_t consume_line_chr_thread;

        shared_buffer_t buffer;

        void initialize_shared_buffer(starch3::Starch::shared_buffer_t* b);
        void delete_shared_buffer(starch3::Starch::shared_buffer_t* b);
        FILE* get_in_stream(void);
        void initialize_in_stream(void);
        void set_in_stream(FILE* ri_stream);
        std::string get_input_fn(void);
        void set_input_fn(std::string s);
        void initialize_out_compression_stream(void);
        void delete_out_compression_stream(void);
        std::string get_note(void);
        void set_note(std::string s);
        Starch::compression_method_t get_compression_method(void);
        void set_compression_method(Starch::compression_method_t t);
        void initialize_bz_stream_ptr(void);
        void setup_bz_stream_callbacks(starch3::Starch* h);
        void delete_bz_stream_ptr(void);
        void bzip2_block_close_callback(void);
        void initialize_command_line_options(int argc, char** argv);
        void test_stdin_availability(void);
        void print_usage(FILE* wo_stream);
        void print_version(FILE* wo_stream);

        // client-specific
        static const compression_method_t client_starch_default_compression_method;
        static const std::string client_name;
        static const std::string client_version;
        static const std::string client_authors;
        std::string get_client_starch_opt_string(void);
        struct option* get_client_starch_long_options(void);
        std::string get_client_starch_name(void);
        std::string get_client_starch_version(void);
        std::string get_client_starch_authors(void);
        std::string get_client_starch_usage(void);
        std::string get_client_starch_description(void);
        std::string get_client_starch_io_options(void);
        std::string get_client_starch_general_options(void);

        static const int in_line_initial_length = 1024;
        static const int in_field_initial_length = 128;
        static const int tf_line_initial_length = 1024;
        static const int tf_buffer_initial_length = 1024;
        static const char field_delimiter = '\t';
        static const char line_delimiter = '\n';
        
        static void* produce_line(void* arg) {
            size_t in_line_pos = 0;
            char* new_line = NULL;
            shared_buffer_t* sb = static_cast<shared_buffer_t*>( arg );
            
            for (;;) {
                pthread_mutex_lock(&sb->lock);
                while (sb->is_new_line_available) {
                    pthread_cond_wait(&sb->new_line_is_empty, &sb->lock);
                }
                in_line_pos = 0;
                /* read a line of data into the buffer */
                do {  
                    if ((in_line_pos + 1) == sb->in_line_capacity) {
                        new_line = NULL;
                        new_line = static_cast<char*>( realloc(sb->in_line, sb->in_line_capacity * 2) );
                        if (!new_line) {
                            std::fprintf(stderr, "Error: Not enough memory for reallocation of shared_buffer_t line character buffer\n");
                            std::exit(ENOMEM);
                        }
                        sb->in_line = new_line;
                        sb->in_line_capacity *= 2;
                    }
                    if ((sb->in_line[in_line_pos++] = static_cast<char>( getc(sb->in_stream) )) == EOF) {
                        sb->next_in = (sb->next_in == 0) ? 1 : 0;
                        sb->is_new_line_available = true;
                        sb->is_new_chromosome_available = true;
                        sb->is_eof = true;
                        pthread_cond_signal(&sb->new_line_is_available);
                        pthread_cond_signal(&sb->new_chromosome_is_available);
                        pthread_mutex_unlock(&sb->lock);
                        std::fprintf(stdout, "Debug: Calling EOF from produce_line()\n");
                        pthread_exit(NULL);
                    }
                } while ((sb->in_line[in_line_pos-1] != line_delimiter));
                sb->next_in = (sb->next_in == 0) ? 1 : 0;
                sb->is_new_line_available = true;
                sb->is_new_chromosome_available = false;
                sb->is_eof = false;
                pthread_cond_signal(&sb->new_line_is_available);
                pthread_mutex_unlock(&sb->lock);
            }
        }
        
        static void* consume_line(void* arg) { 
            size_t in_line_pos = 0;
            size_t in_elem_pos = 0;
            shared_buffer_t* sb = static_cast<shared_buffer_t*>( arg );
            char* new_field = NULL;
            
            sb->bed->token = k_chromosome_token;
            
            for (;;) {
                pthread_mutex_lock(&sb->lock);
                while (!sb->is_new_line_available) {
                    pthread_cond_wait(&sb->new_line_is_available, &sb->lock);
                }
                in_line_pos = 0;
                sb->bed->chr[in_elem_pos] = '\0';
                sb->bed->start_str[in_elem_pos] = '\0';
                sb->bed->stop_str[in_elem_pos] = '\0';
                sb->bed->rem[in_elem_pos] = '\0';
                /* process next line of text from the buffer */
                do {
                    if ((sb->in_line[in_line_pos] == field_delimiter) && (sb->bed->token != k_remainder_token)) {
                        in_elem_pos = 0;
                        sb->bed->token++;
                        in_line_pos++;
                    }
                    if (sb->in_line[in_line_pos] == EOF) {
                        if (sb->bed->chr) {
                            free(sb->bed->chr);
                            sb->bed->chr = NULL;
                        }
                        sb->is_eof = true;
                        std::fprintf(stdout, "Debug: Calling EOF from consume_line()\n");
                        pthread_cond_signal(&sb->new_chromosome_is_available);
                        pthread_mutex_unlock(&sb->lock);
                        pthread_exit(NULL);
                    }

                    switch (sb->bed->token) {
                    case k_chromosome_token:
                        if ((in_elem_pos + 1) == sb->bed->chr_capacity) {
                            new_field = NULL;
                            new_field = static_cast<char*>( realloc(sb->bed->chr, sb->bed->chr_capacity * 2) );
                            if (!new_field) {
                                std::fprintf(stderr, "Error: Not enough memory for reallocation of buffer BED line chromosome field component\n");
                                std::exit(ENOMEM);
                            }
                            sb->bed->chr = new_field;
                            sb->bed->chr_capacity *= 2;
                        }
                        sb->bed->chr[in_elem_pos] = sb->in_line[in_line_pos++];
                        sb->bed->chr[++in_elem_pos] = '\0';
                        break;
                    case k_start_token:
                        if ((in_elem_pos + 1) == sb->bed->start_str_capacity) {
                            new_field = NULL;
                            new_field = static_cast<char*>( realloc(sb->bed->start_str, sb->bed->start_str_capacity * 2) );
                            if (!new_field) {
                                std::fprintf(stderr, "Error: Not enough memory for reallocation of buffer BED line start field component\n");
                                std::exit(ENOMEM);
                            }
                            sb->bed->start_str = new_field;
                            sb->bed->start_str_capacity *= 2;
                        }
                        sb->bed->start_str[in_elem_pos] = sb->in_line[in_line_pos++];
                        sb->bed->start_str[++in_elem_pos] = '\0';
                        break;
                    case k_stop_token:
                        if ((in_elem_pos + 1) == sb->bed->stop_str_capacity) {
                            new_field = NULL;
                            new_field = static_cast<char*>( realloc(sb->bed->stop_str, sb->bed->stop_str_capacity * 2) );
                            if (!new_field) {
                                std::fprintf(stderr, "Error: Not enough memory for reallocation of buffer BED line stop field component\n");
                                std::exit(ENOMEM);
                            }
                            sb->bed->stop_str = new_field;
                            sb->bed->stop_str_capacity *= 2;
                        }
                        sb->bed->stop_str[in_elem_pos] = sb->in_line[in_line_pos++];
                        sb->bed->stop_str[++in_elem_pos] = '\0';
                        break;
                    case k_remainder_token:
                        if ((in_elem_pos + 1) == sb->bed->rem_capacity) {
                            new_field = NULL;
                            new_field = static_cast<char*>( realloc(sb->bed->rem, sb->bed->rem_capacity * 2) );
                            if (!new_field) {
                                std::fprintf(stderr, "Error: Not enough memory for reallocation of buffer BED line remainder field component\n");
                                std::exit(ENOMEM);
                            }
                            sb->bed->rem = new_field;
                            sb->bed->rem_capacity *= 2;
                        }
                        sb->bed->rem[in_elem_pos] = sb->in_line[in_line_pos++];
                        sb->bed->rem[++in_elem_pos] = '\0';
                        break;
                    }
                } while (sb->in_line[in_line_pos-1] != line_delimiter);
                switch (sb->bed->token) {
                case k_stop_token:
                    sb->bed->stop_str[--in_elem_pos] = '\0';
                    break;
                case k_remainder_token:
                    sb->bed->rem[--in_elem_pos] = '\0';
                    break;
                }
                sscanf(sb->bed->start_str, "%" SCNd64, &sb->bed->start);
                sscanf(sb->bed->stop_str, "%" SCNd64, &sb->bed->stop);
                in_elem_pos = 0;
                sb->bed->token = k_chromosome_token;
                sb->next_out = (sb->next_out == 0) ? 1 : 0;
                if ((sb->tf_state->current_chr == NULL) || (std::strcmp(sb->bed->chr, sb->tf_state->current_chr) != 0)) {
                    sb->is_new_chromosome_available = true;
                    sb->is_new_line_available = false;
                    pthread_cond_signal(&sb->new_chromosome_is_available);
                }
                else {
                    fprintf(stdout, "Debug: [%s] [%" PRId64 "] [%" PRId64 "] [%s]\n", sb->bed->chr, sb->bed->start, sb->bed->stop, sb->bed->rem);
                    update_transformation_state(sb);
                    sb->is_new_chromosome_available = false;
                    sb->is_new_line_available = false;
                    pthread_cond_signal(&sb->new_line_is_empty);
                }
                pthread_mutex_unlock(&sb->lock);
            }
        }

        static void* consume_line_chr(void* arg) {
            shared_buffer_t* sb = static_cast<shared_buffer_t*>( arg );
            for (;;) {
                if (sb->is_eof) {
                    std::fprintf(stdout, "Debug: Calling EOF from consume_line_chr()\n");
                    fprintf(stderr, "[%s]\n", sb->tf_buffer);
                    pthread_exit(NULL);
                }
                pthread_mutex_lock(&sb->lock);
                while (!sb->is_new_chromosome_available) {
                    pthread_cond_wait(&sb->new_chromosome_is_available, &sb->lock);
                }
                update_str(&sb->tf_state->last_chr, sb->tf_state->current_chr);
                update_str(&sb->tf_state->current_chr, sb->bed->chr);
                std::fprintf(stdout, "Debug: Chromosome state updated (was [%s] - now [%s])\n", sb->tf_state->last_chr, sb->tf_state->current_chr);
                sb->is_new_chromosome_available = false;
                sb->is_new_line_available = true;
                pthread_cond_signal(&sb->new_line_is_available);
                pthread_mutex_unlock(&sb->lock);
            }
        }

        static void append_tf_line_to_buffer(shared_buffer_t* sb) {
            char* new_tf_buffer = NULL;
            size_t tf_line_len = strlen(sb->tf_line);
            /* resize tf_buffer, if necessary */
            if (sb->tf_buffer_capacity < (sb->tf_buffer_size + tf_line_len)) {
                std::fprintf(stderr, "resizing...\n");
                new_tf_buffer = NULL;
                new_tf_buffer = static_cast<char*> ( realloc(sb->tf_buffer, sb->tf_buffer_capacity * 2) );
                if (!new_tf_buffer) {
                    std::fprintf(stderr, "Error: Not enough memory for reallocation of transformation buffer\n");
                    std::exit(ENOMEM);
                }
                sb->tf_buffer = new_tf_buffer;
                sb->tf_buffer_capacity *= 2;
            }
            /* copy data to end of buffer */
            std::fprintf(stdout, "_____\nbefore [%s]\nbuffer size [%zu]\n_____\n", sb->tf_buffer, sb->tf_buffer_size);
            std::fprintf(stdout, "copying [%s] length [%zu]\n", sb->tf_line, tf_line_len);
            std::memcpy(sb->tf_buffer + sb->tf_buffer_size, sb->tf_line, tf_line_len);
            sb->tf_buffer_size += tf_line_len;
            std::fprintf(stdout, "after [%s] [%zu]\n_____\n", sb->tf_buffer, sb->tf_buffer_size);
        }

        static void update_transformation_state(shared_buffer_t* sb) {
            char* new_tf_line = NULL;
            size_t tf_line_len = 0;
            int64_t last_start_diff = 0;
            int64_t rem_len = 0;
            sb->tf_state->current_start = sb->bed->start;
            sb->tf_state->current_stop = sb->bed->stop;
            sb->tf_state->current_coord_diff = sb->bed->stop - sb->bed->start;
            /* resize tf_line, if necessary */
            if (sb->tf_state->current_coord_diff != sb->tf_state->last_coord_diff) {
                sb->tf_state->last_coord_diff = sb->tf_state->current_coord_diff;
                tf_line_len = static_cast<size_t>( 2 + n_digits(sb->tf_state->current_coord_diff) );
                if (sb->tf_line_capacity < tf_line_len) {
                    new_tf_line = NULL;
                    new_tf_line = static_cast<char*> ( realloc(sb->tf_line, sb->tf_line_capacity * 2) );
                    if (!new_tf_line) {
                        std::fprintf(stderr, "Error: Not enough memory for reallocation of transformation buffer line\n");
                        std::exit(ENOMEM);
                    }
                    sb->tf_line = new_tf_line;
                    sb->tf_line_capacity *= 2;
                }
                sprintf(sb->tf_line, "p%" PRId64 "\n", sb->tf_state->current_coord_diff);
                sb->tf_line[tf_line_len] = '\0';
                fprintf(stdout, "Encoding: [%s]\n", sb->tf_line);
                append_tf_line_to_buffer(sb);
            }
            /* encode data */
            if (sb->tf_state->last_stop != 0) {
                last_start_diff = sb->tf_state->current_start - sb->tf_state->last_stop;
                rem_len = (sb->bed->rem) ? static_cast<int64_t>( strlen(sb->bed->rem) ) : 0;
                tf_line_len = static_cast<size_t>( n_digits(last_start_diff) + 2 + rem_len );
                if (sb->tf_line_capacity < tf_line_len) {
                    new_tf_line = NULL;
                    new_tf_line = static_cast<char*> ( realloc(sb->tf_line, sb->tf_line_capacity * 2) );
                    if (!new_tf_line) {
                        std::fprintf(stderr, "Error: Not enough memory for reallocation of transformation buffer line\n");
                        std::exit(ENOMEM);
                    }
                    sb->tf_line = new_tf_line;
                    sb->tf_line_capacity *= 2;
                }
                if (rem_len > 0) {
                    sprintf(sb->tf_line, "%" PRId64 "%c%s\n", last_start_diff, field_delimiter, sb->bed->rem);
                }
                else {
                    sprintf(sb->tf_line, "%" PRId64 "\n", last_start_diff);
                }
                fprintf(stdout, "Encoding: [%s]\n", sb->tf_line);
                append_tf_line_to_buffer(sb);
            }
            else {
                rem_len = (sb->bed->rem) ? static_cast<int64_t>( strlen(sb->bed->rem) ) : 0;
                tf_line_len = static_cast<size_t>( n_digits(sb->tf_state->current_start) + 2 + rem_len );
                if (sb->tf_line_capacity < tf_line_len) {
                    new_tf_line = NULL;
                    new_tf_line = static_cast<char*> ( realloc(sb->tf_line, sb->tf_line_capacity * 2) );
                    if (!new_tf_line) {
                        std::fprintf(stderr, "Error: Not enough memory for reallocation of transformation buffer line\n");
                        std::exit(ENOMEM);
                    }
                    sb->tf_line = new_tf_line;
                    sb->tf_line_capacity *= 2;
                }
                if (rem_len > 0) {
                    sprintf(sb->tf_line, "%" PRId64 "%c%s\n", sb->tf_state->current_start, field_delimiter, sb->bed->rem);
                }
                else {
                    sprintf(sb->tf_line, "%" PRId64 "\n", sb->tf_state->current_start);
                }
                fprintf(stdout, "Encoding: [%s]\n", sb->tf_line);
                append_tf_line_to_buffer(sb);
            }
            sb->tf_state->last_start = sb->tf_state->current_start;
            sb->tf_state->last_stop = sb->tf_state->current_stop;
        }

        static void initialize_transformation_state(starch3::Starch::transform_state_t** tfs) {
            (*tfs)->line_count = 0;
            (*tfs)->last_start = 0;
            (*tfs)->last_stop = 0;
            (*tfs)->last_coord_diff = 0;
            (*tfs)->last_chr = NULL;
            (*tfs)->current_start = 0;
            (*tfs)->current_stop = 0;
            (*tfs)->current_coord_diff = 0;
            (*tfs)->current_chr = NULL;
            (*tfs)->base_count_unique = 0;
            (*tfs)->base_count_nonunique = 0;
#ifdef DEBUG
            std::fprintf(stderr, "--- starch3::Starch::reset_transformation_state() ---\n");
#endif
        }

        static void delete_transformation_state(starch3::Starch::transform_state_t** tfs) {
            if ((*tfs)->last_chr)    { free((*tfs)->last_chr);    }
            if ((*tfs)->current_chr) { free((*tfs)->current_chr); }
        }

        static inline void update_str(char** dest, char* src) {
            if (*dest) {
                free(*dest);
                *dest = NULL;
            }
            if (!src) {
                return;
            }
            *dest = static_cast<char *>( malloc(std::strlen(src) + 1) );
            if (!*dest) {
                std::fprintf(stderr, "Error: Not enough memory for allocation of transformation buffer chromosome\n");
                std::exit(ENOMEM);
            }
            std::strncpy(*dest, src, std::strlen(src) + 1);
        }

        static inline int64_t n_digits(int64_t i) {
            if (i < 0) i = -i;
            if (i <         10) return 1;
            if (i <        100) return 2;
            if (i <       1000) return 3;
            if (i <      10000) return 4;
            if (i <     100000) return 5;
            if (i <    1000000) return 6;      
            if (i <   10000000) return 7;
            if (i <  100000000) return 8;
            if (i < 1000000000) return 9;
            return 10;
        }

        static void bzip2_block_close_static_callback(void* s);
    };

    void Starch::initialize_shared_buffer(starch3::Starch::shared_buffer_t* sb) {
        sb->next_in = 0;
        sb->next_out = 0;
        sb->is_new_line_available = false;
        sb->is_new_chromosome_available = false;
        sb->is_eof = false;
        sb->in_line = NULL;
        sb->in_line = static_cast<char*>( malloc(starch3::Starch::in_line_initial_length) );
        if (!sb->in_line) {
            std::fprintf(stderr, "Error: Not enough memory for shared_buffer_t line character buffer\n");
            std::exit(ENOMEM);
        }
        sb->in_line_capacity = starch3::Starch::in_line_initial_length;
        sb->tf_line = NULL;
        sb->tf_line = static_cast<char*>( malloc(starch3::Starch::tf_line_initial_length) );
        if (!sb->tf_line) {
            std::fprintf(stderr, "Error: Not enough memory for shared_buffer_t transformed line character buffer\n");
            std::exit(ENOMEM);
        }
        sb->tf_line_capacity = starch3::Starch::tf_line_initial_length;
        sb->bed = static_cast<bed_t*>( malloc(sizeof(bed_t)) );
        if (!sb->bed) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line components\n");
            std::exit(ENOMEM);
        }
        sb->bed->chr = NULL;
        sb->bed->chr = static_cast<char*>( malloc(starch3::Starch::in_field_initial_length) );
        if (!sb->bed->chr) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line chromosome field component\n");
            std::exit(ENOMEM);
        }
        sb->bed->chr_capacity = starch3::Starch::in_field_initial_length;
        sb->bed->start_str = NULL;
        sb->bed->start_str = static_cast<char*>( malloc(starch3::Starch::in_field_initial_length) );
        if (!sb->bed->start_str) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line start field component\n");
            std::exit(ENOMEM);
        }
        sb->bed->start_str_capacity = starch3::Starch::in_field_initial_length;
        sb->bed->stop_str = NULL;
        sb->bed->stop_str = static_cast<char*>( malloc(starch3::Starch::in_field_initial_length) );
        if (!sb->bed->stop_str) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line stop field component\n");
            std::exit(ENOMEM);
        }
        sb->bed->stop_str_capacity = starch3::Starch::in_field_initial_length;
        sb->bed->rem = NULL;
        sb->bed->rem = static_cast<char*>( malloc(starch3::Starch::in_field_initial_length) );
        if (!sb->bed->rem) {
            std::fprintf(stderr, "Error: Could not allocate space for buffer BED line remainder field component\n");
            std::exit(ENOMEM);
        }
        sb->bed->rem_capacity = starch3::Starch::in_field_initial_length;
        pthread_mutex_init(&sb->lock, NULL);
        pthread_cond_init(&sb->new_line_is_available, NULL);
        pthread_cond_init(&sb->new_line_is_empty, NULL);
        pthread_cond_init(&sb->new_chromosome_is_available, NULL);
        sb->in_stream = get_in_stream();
        sb->tf_state = NULL;
        sb->tf_state = static_cast<transform_state_t*>( malloc(sizeof(transform_state_t)) );
        if (!sb->tf_state) {
            std::fprintf(stderr, "Error: Not enough memory for shared_buffer_t transformation state\n");
            std::exit(ENOMEM);
        }
        this->initialize_transformation_state(&sb->tf_state);
        sb->tf_buffer = NULL;
        sb->tf_buffer = static_cast<char*>( malloc(sizeof(tf_buffer_initial_length)) );
        if (!sb->tf_buffer) {
            std::fprintf(stderr, "Error: Not enough memory for shared_buffer_t transformation buffer\n");
            std::exit(ENOMEM);
        }
        sb->tf_buffer_capacity = tf_buffer_initial_length;
        sb->tf_buffer_size = 0;

#ifdef DEBUG
        std::fprintf(stderr, "--- starch3::Starch::initialize_shared_buffer() ---\n");
#endif
    }

    void Starch::delete_shared_buffer(starch3::Starch::shared_buffer_t* sb) {
#ifdef DEBUG
        std::fprintf(stderr, "--- starch3::Starch::delete_shared_buffer() - START ---\n");
#endif
        pthread_mutex_destroy(&sb->lock);
        pthread_cond_destroy(&sb->new_line_is_available);
        pthread_cond_destroy(&sb->new_line_is_empty);
        pthread_cond_destroy(&sb->new_chromosome_is_available);
        fclose(sb->in_stream);
        if (sb->in_line) {
            free(sb->in_line);
            sb->in_line = NULL;
            sb->in_line_capacity = 0;
        }
        if (sb->tf_line) {
            free(sb->tf_line);
            sb->tf_line = NULL;
            sb->tf_line_capacity = 0;
        }
        this->delete_transformation_state(&sb->tf_state);
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
            free(sb->bed);
            sb->bed = NULL;
        }
        if (sb->tf_buffer) {
            free(sb->tf_buffer);
            sb->tf_buffer = NULL;
            sb->tf_buffer_capacity = 0;
            sb->tf_buffer_size = 0;
        }
#ifdef DEBUG
        std::fprintf(stderr, "--- starch3::Starch::delete_shared_buffer() - END ---\n");
#endif
    }

    FILE* Starch::get_in_stream(void) {
        return _in_stream;
    }

    void Starch::initialize_in_stream(void) {
        FILE* in_fp = NULL;
        in_fp = this->get_input_fn().empty() ? stdin : fopen(this->get_input_fn().c_str(), "r");
        if (!in_fp) {
            std::fprintf(stderr, "Error: Input file handle could not be created\n");
            std::exit(ENODATA); /* No message is available on the STREAM head read queue (POSIX.1) */
        }
        this->set_in_stream(in_fp);
    }

    void Starch::set_in_stream(FILE* is) {
        _in_stream = is;
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

    void Starch::initialize_out_compression_stream(void) {
        switch (this->get_compression_method()) {
        case k_bzip2:
            this->initialize_bz_stream_ptr();
            this->setup_bz_stream_callbacks(this);
            break;
        case k_gzip:
            std::fprintf(stderr, "Error: This method is unsupported at this time\n");
            std::exit(ENOSYS);
        case k_compression_method_undefined:
            std::fprintf(stderr, "Error: This method is undefined\n");
            std::exit(ENOSYS);
            break;
        }
    }

    void Starch::delete_out_compression_stream(void) {
        switch (this->get_compression_method()) {
        case k_bzip2:
            this->delete_bz_stream_ptr();
            break;
        case k_gzip:
            std::fprintf(stderr, "Error: This method is unsupported at this time\n");
            std::exit(ENOSYS);
            break;
        case k_compression_method_undefined:
            std::fprintf(stderr, "Error: This method is undefined\n");
            std::exit(ENOSYS);
            break;
        }
    }
    
    std::string Starch::get_note(void) {
        return _note;
    }
    
    void Starch::set_note(std::string s) {
        _note = s;
    }

    Starch::compression_method_t Starch::get_compression_method(void) {
        return _compression_method;
    }

    void Starch::set_compression_method(Starch::compression_method_t t) {
        _compression_method = t;
    }

    void Starch::initialize_bz_stream_ptr(void) { 
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
            std::fprintf(stderr, "--- starch3::Starch::initialize_bz_stream_ptr() ---\n");
#endif
            break;
        }
    }

    void Starch::setup_bz_stream_callbacks(starch3::Starch* h) {
        if (!_bz_stream_ptr)
            return;
        _bz_stream_ptr->handler = &h;
        _bz_stream_ptr->block_close_functor = bzip2_block_close_static_callback;
    }

    void Starch::delete_bz_stream_ptr(void) {
        if (!_bz_stream_ptr)
            return;
        // release all memory associated with the compression stream before ptr deletion
        int end_res = BZ2_bzCompressEnd(_bz_stream_ptr);
        switch (end_res) {
        case BZ_PARAM_ERROR:
            std::fprintf(stderr, "Error: Could not release internals of bz_stream pointer\n");
            std::exit(EINVAL);
        case BZ_OK:
#ifdef DEBUG
            std::fprintf(stderr, "--- starch3::Starch::delete_bz_stream_ptr() ---\n");
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
        this->set_note(std::string());
        this->set_compression_method(k_compression_method_undefined);
    }

    Starch::~Starch() {
    }

    extern Starch* self;
}

#endif // STARCH3_H_
