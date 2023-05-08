#ifndef _RING_H
#define _RING_H

#define RING_BUFFER_SIZE 100

class Ring {

    private:
        unsigned char write_pointer = 0;
        unsigned char read_pointer = 0;
        unsigned char buffer[RING_BUFFER_SIZE];


    public:

        Ring() {
	        read_pointer = 0;
	        write_pointer = 0;
        };

        unsigned char size(void) {
            return RING_BUFFER_SIZE;
        };

        unsigned char available(void) {
            return (RING_BUFFER_SIZE + write_pointer - read_pointer) % RING_BUFFER_SIZE;
        };

        void clear(void) {
            read_pointer = write_pointer = 0;
        };

        int read(void) {
            if (read_pointer == write_pointer) return -1;
            unsigned char c = buffer[read_pointer];
            read_pointer = (read_pointer + 1) % RING_BUFFER_SIZE;
            return c;
        };

        int peek(void) {
            if (read_pointer == write_pointer) return -1;
            unsigned char c = buffer[read_pointer];
            return c;
        };

        int append(unsigned char c) { 
            if ((write_pointer + 1) % RING_BUFFER_SIZE == read_pointer) return -1;
            buffer[write_pointer] = c;
            write_pointer = (write_pointer + 1) % RING_BUFFER_SIZE;
            return 0;
        };

        int replace(unsigned char c) { 
            if (read_pointer == write_pointer) return -1;
            buffer[read_pointer] = c;
            return 0;
        };

        int prepend(unsigned char c) { 
            if ((RING_BUFFER_SIZE + read_pointer - 1) % RING_BUFFER_SIZE == write_pointer) return -1;
            read_pointer = (RING_BUFFER_SIZE + read_pointer - 1) % RING_BUFFER_SIZE;
            buffer[read_pointer] = c;
            return 0;
        };

};

#endif // _RING_H