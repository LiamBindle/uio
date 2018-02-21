#ifndef UIO_H
#define UIO_H
/**
 * @file
 * @author Liam Bindle <liam.bindle@usask.ca>
 * @version 0.0
 * 
 * @mainpage
 * 
 * @section DESCRIPTION
 *
 * \par
 * uio is a portable I/O abstraction layer for low-performance systems such as 
 * microcontrollers.
 * 
 * @section LICENSE
 * 
 * \par
 * MIT License
 * 
 * \par
 * Copyright (c) 2018 Liam Bindle
 * 
 * \par
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * \par
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * \par
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include <cstring>

namespace uio {

    /// \cond DO_NOT_DOCUMENT
    template<class T>
    const T& min(const T& v1, const T& v2) { 
        return v1 < v2 ? v1 : v2;
    }
    /// \endcond

    /**
     * @brief Class to manage stream errors.
     * 
     * This class is used to set, clear, and manage stream errors that might 
     * be encountered during stream operations.
     */
    class streamerr {
    public:

        struct {
            unsigned char 
            uninitialized: 1,   ///< Stream has not been properly initialized.
            overflow: 1,        ///< A buffer has overflowed.
            reserved: 4;        ///< Application-layer error codes.
        } _flags; ///< The bitmap of error flags.

        /**
         * @brief Returns \c true if any error flags are set.
         * 
         * @returns \c true if any error flags are set, \c false otherwise.
         */
        bool any() const {
            char err = 0;
            for(size_t i = 0; i < sizeof(_flags); ++i) {
                err |= *(((const char*) &_flags) + i);
            }
            return err;
        }

        /**
         * @brief Clear all error flags.
         */
        inline void clear() {
            memset(&_flags, 0, sizeof(_flags));
        }

        /**
         * @brief Default constructor.
         */
        streamerr() {
            memset(&_flags, 0, sizeof(_flags));
        }
        
        /**
         * @brief Bitwise \ref _flags \c OR \c assignment \c operator.
         */
        void operator|=(const streamerr& other) {
            char* err = (char*) &_flags;
            for(size_t i = 0; i < sizeof(_flags); ++i) {
                *err++ |= *(((char*) &other._flags) + i);
            }
        }
    };

    /**
     * @brief Base class for all stream objects.
     */
    class stream_base {
    public:
        virtual ~stream_base() {}
    };

    /**
     * @brief A simple byte-buffer.
     * 
     * This class implements an efficient and lightweight byte-buffer.
     * 
     * @attention \ref setbuf \em must be called before this class can
     * be used.
     */
    class streambuf : public stream_base {
    public:
        /**
         * @brief Default constructor.
         */
        streambuf()  {
            _capacity = 0;
            _getpos = 0;
            _putpos = 0;
            _dump = false;
            _buf = NULL;
            _error._flags.uninitialized = true;
        }

        /**
         * @brief Get the number of bytes available to be read-out.
         * 
         * This function is equiavlent to \ref size minus the internal
         * \c sget cursor position.
         * 
         * @returns Number of bytes that can be read-out. 
         */
        inline size_t in_avail() const {
            return _putpos - _getpos;
        }

        /**
         * @brief Get the next byte out of the buffer.
         * 
         * @param[out] c Address to copy byte to. 
         * 
         * @returns 1 if a byte was copied to \a c, 0 otherwise (i.e. no bytes
         * left in the buffer). 
         */
        size_t sgetc(char* c) {
            if (_putpos - _getpos <= 0) {
                return (size_t) 0;
            } else {
                *c = *(_buf + (_getpos++));
                _dump = (_getpos == _putpos);
                return (size_t) 1;
            }
        }

        /**
         * @brief Copies up to \a len bytes into \a s from the buffer.
         * 
         * If there are fewer than \a len bytes in the buffer, 
         * \ref in_avail() bytes will be copied to \a s.
         * 
         * @param[out] s Address to begin copying to.
         * @param[in] len Maximum number of bytes to copy.
         * 
         * @returns The number of bytes copied to \a s.
         */
        size_t sgetn(char* s, size_t len) {
            size_t n = min(_putpos - _getpos, len);
            memcpy(s, _buf + _getpos, n);
            _getpos += n;
            _dump = (_getpos == _putpos);
            return n;
        }

        /**
         * @brief Append \a c to the buffer.
         * 
         * @param[in] c Byte to be appended to the buffer.
         * 
         * @returns 1 if the byte was appended, 0 otherwise (i.e. buffer is 
         * full).
         */
        size_t sputc(char c) {
            // check for dump
            if (_dump) {
                _putpos = 0;
                _getpos= 0;
                _dump = false;
            }
            
            // put if possible
            if (_capacity - _putpos <= 0) {
                return (size_t) 0; // not possible
            } else {
                *(_buf + (_putpos++)) = c;
                return (size_t) 1; // possible
            }
        }

        /**
         * @brief Copy \a len bytes from \a s to the back of the buffer.
         * 
         * @param[in] s Address to begin copying from.
         * @param[in] len Number of bytes to copy.
         * 
         * @returns The number of bytes copyied, beginning at \a s.
         */
        size_t sputn(const char* s, size_t len) {
            // check for dump
            if (_dump) {
                _putpos = 0;
                _getpos= 0;
                _dump = false;
            }

            // memcpy bytes
            size_t n = min(_capacity - _putpos, len);
            memcpy(_buf + _putpos, s, n);
            _putpos += n;
            return n;
        }

        /**
         * @brief Clear all buffer data and error flags.
         * 
         * @returns The number of bytes that were cleared from the buffer.
         */        
        size_t purge() {
            // hard reset buffer
            size_t n = _putpos - _getpos;
            _getpos = 0;
            _putpos = 0;
            _error.clear();
            _error._flags.uninitialized = (_buf == NULL);
            return n;
        }

        /**
         * @brief Initialize the buffer.
         * 
         * @attention This function \em must be called before the buffer can
         * be used.
         * 
         * @param buf Memory allocation for this buffer.
         * @param capacity Size of the buffer (i.e. size of \a buf).
         * 
         * @returns \c this
         */  
        inline streambuf* setbuf(char* buf, size_t capacity) {
            _buf = buf;
            _capacity = capacity;
            _getpos = 0;
            _putpos = 0;
            _dump = false;
            _error._flags.uninitialized = (_buf == NULL);
            return this;
        }

        /**
         * @brief Get the entire contents of the buffer.
         * 
         * Returns a pointer to the entire contents of the buffer. The 
         * contents of the buffer will be overwritten on the next call to 
         * \ref sputc or \ref sputn (this that calls to \ref size() are 
         * valid until the next call to \c sputc or \c sputn).
         * 
         * @see size
         * 
         * @returns Address of the first byte in the buffer.
         */
        inline void* dump() {
            _dump = true;
            return _buf;
        }

        /**
         * @brief Get the number of bytes in the buffer. 
         * 
         * This function is similar to \ref in_avail except it returns the 
         * total number of bytes that have been put into the buffer (as opposed
         * to \c in_avail which returns the number of bytes put into the buffer
         * minus internal cursors position).
         * 
         * @see dump
         * 
         * @returns The total number of bytes in the buffer.
         */
        inline size_t size() const {
            return _putpos;
        }

    private:
        size_t _capacity;
        size_t _getpos;
        size_t _putpos;
        bool _dump;
        char* _buf;
    public:
        streamerr _error; ///< The buffer's \ref streamerr. 
    };

    /**
     * @brief An input data object.
     * 
     * \ref istream is an object that supports the following operations:
     * \arg \ref gcount
     * \arg \ref get
     * \arg \ref sync 
     * 
     * @note Input data might not be automatically synchronized. Users 
     * should call \ref sync before \ref get.
     */
    class istream : public stream_base {
    public:
        /**
         * @brief Convenience function for writing input data to \c string.
         * 
         * \ref gcount characters are copied to \a s, as well as a null 
         * character. This means that \ref gcount plus 1 bytes are copied to
         * \a s.
         * 
         * @attention This function has no safe-guards to check that \a s 
         * is large enough to store the entire input data streams contents.
         * 
         * @param[out] s First address of character array.
         * 
         * @returns \c *this
         */
        virtual istream& operator>>(char* s) {
            size_t n = _ibuf.in_avail();
            _ibuf.sgetn(s, n);
            *(s + n) = '\0';
            return *this;
        }

        /**
         * @brief Returns the number of bytes that can be gotten using
         * \ref get.
         * 
         * @see get
         * 
         * @returns The number of bytes that can be read. 
         */
        virtual size_t gcount() {
            return _ibuf.in_avail();
        }

        /**
         * @brief Get the next byte from the input data stream.
         * 
         * @param[out] buf Address to copy the next input data byte to.  
         * 
         * @returns 1 if the next byte was copied, 0 otherwise (i.e. no more
         * bytes to be read).
         */
        virtual size_t get(char* buf) {
            return _ibuf.sgetc(buf);
        }

        /**
         * @brief Copy up to \a buflen bytes to \a buf.
         * 
         * If \a buflen is greater than the number of bytes that can be 
         * gotten (\ref gcount), then only \ref gcount bytes will be copied
         * to \a buf.
         * 
         * @param[out] buf First address to start copying bytes to.
         * @param[in] buflen Maximum number of bytes to be copied to \a buf.
         * 
         * @returns Number of bytes copied to \a buf.
         */
        virtual size_t get(char* buf, size_t buflen) {
            return _ibuf.sgetn(buf, buflen);
        }

        /**
         * @brief Pure virtual function to synchronize (update) input data 
         * stream.
         * 
         * This function updates the internal buffers (i.e. does whatever 
         * needs to happend to bring the internal buffers up to date).
         * 
         * @note It might be necessary to call this function before \ref get.
         * 
         * @returns \c *this
         */
        virtual istream& sync() = 0;
    protected:
        streambuf _ibuf; ///< Input data \ref streambuf.
    public:
        streamerr _ierror; ///< Input stream \ref streamerr.
    };

    /**
     * @brief An output data object.
     * 
     * \ref ostream is an object that supports the following operations:
     * \arg \ref put
     * \arg \ref flush 
     * 
     * @note Output data might not be automatically flushed. Users 
     * should call \ref flush when they want the internal buffers to be 
     * written out.
     */
    class ostream : public stream_base {
    public:
        /**
         * @brief Convenience function for writing output data from a 
         * \c string.
         * 
         * @note The null character at the end of \a s is \em not copied
         * to the buffer.
         * 
         * @note \c _oerror._flags.overflow is set if there is not enough
         * space in the output buffer.
         * 
         * @param[out] s Character array to write to output stream.
         * 
         * @returns \c *this
         */
        virtual ostream& operator<<(const char* s) {
            size_t n = strlen(s);
            if (n == _obuf.sputn(s, n)) {
                return *this;
            } else {
                _oerror._flags.overflow = true;
                _oerror |= _obuf._error;
                return *this;
            }
        }

        /**
         * @brief Write byte to output data stream.
         * 
         * @param[in] c The byte's value.
         * 
         * @note \c _oerror._flags.overflow is set if there is not enough
         * space in the output buffer.
         * 
         * @returns \c *this
         */
        virtual ostream& put(char c) {
            if (_obuf.sputc(c)) {
                return *this;
            } else {
                _oerror._flags.overflow = true;
                _oerror |= _obuf._error;
                return *this;
            }
        }

        /**
         * @brief Write \a n bytes from \a s to the output data stream.
         * 
         * @param[in] s The address of the first byte to be written.
         * @param[in] n The number of bytes to be written.
         * 
         * @note \c _oerror._flags.overflow is set if there is not enough
         * space in the output buffer.
         * 
         * @returns \c *this
         */
        virtual ostream& write(const char* s, size_t n) {
            if (n == _obuf.sputn(s, n)) {
                return *this;
            } else {
                _oerror._flags.overflow = true;
                _oerror |= _obuf._error;
                return *this;
            }
        }

        /**
         * @brief A pure virtual function to flush buffered data to output 
         * data stream. 
         * 
         * This function does whatever is necessary to output the buffered 
         * data.
         * 
         * @returns \c *this
         */
        virtual ostream& flush() = 0;
    protected:
        streambuf _obuf; ///< Output stream \ref streambuf.
    public:
        streamerr _oerror; ///< Output stream \ref streamerr.
    };

    /**
     * @brief An input and output data stream.
     */
    class iostream : public istream, public ostream {};
};

#endif // UIO_H
