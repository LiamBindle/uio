
#include <cstring>

namespace uio {

    template<class T>
    const T& min(const T& v1, const T& v2) { 
        return v1 < v2 ? v1 : v2;
    }

    class streamerr {
    public:
        struct {
            unsigned char uninitialized: 1, overflow: 1, reserved: 4;
        } _flags;
        bool any() const {
            char err = 0;
            for(size_t i = 0; i < sizeof(_flags); ++i) {
                err |= *(((const char*) &_flags) + i);
            }
            return err;
        }

        inline void clear() {
            memset(&_flags, 0, sizeof(_flags));
        }

        streamerr() {
            memset(&_flags, 0, sizeof(_flags));
        }
        
        void operator|=(const streamerr& other) {
            char* err = (char*) &_flags;
            for(size_t i = 0; i < sizeof(_flags); ++i) {
                *err++ |= *(((char*) &other._flags) + i);
            }
        }
    };

    class stream_base {
    public:
        virtual ~stream_base() {}
    };

    class streambuf : public stream_base {
    public:
        streambuf()  {
            _capacity = 0;
            _getpos = 0;
            _putpos = 0;
            _dump = false;
            _buf = NULL;
            _error._flags.uninitialized = true;
        }

        inline size_t in_avail() const {
            return _putpos - _getpos;
        }

        size_t sgetc(char* c) {
            if (_putpos - _getpos <= 0) {
                return (size_t) 0;
            } else {
                *c = *(_buf + (_getpos++));
                _dump = (_getpos == _putpos);
                return (size_t) 1;
            }
        }
        size_t sgetn(char* s, size_t len) {
            size_t n = min(_putpos - _getpos, len);
            memcpy(s, _buf + _getpos, n);
            _getpos += n;
            _dump = (_getpos == _putpos);
            return n;
        }

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
        size_t purge() {
            // hard reset buffer
            size_t n = _putpos - _getpos;
            _getpos = 0;
            _putpos = 0;
            _error.clear();
            _error._flags.uninitialized = (_buf == NULL);
            return n;
        }
        inline streambuf* setbuf(char* buf, size_t capacity) {
            _buf = buf;
            _capacity = capacity;
            _getpos = 0;
            _putpos = 0;
            _dump = false;
            _error._flags.uninitialized = (_buf == NULL);
            return this;
        }

        inline void* dump() {
            _dump = true;
            return _buf;
        }

        inline size_t size() {
            return _putpos;
        }

    private:
        size_t _capacity;
        size_t _getpos;
        size_t _putpos;
        bool _dump;
        char* _buf;
    public:
        streamerr _error;
    };

    class istream : public stream_base {
    public:
        virtual istream& operator>>(char* s) {
            size_t n = _ibuf.in_avail();
            if (n != _ibuf.sgetn(s, n)) {
                _ierror._flags.overflow = true;
                _ierror |= _ibuf._error;
            }
            *(s + n) = '\0';
            return *this;
        }
        virtual size_t gcount() {
            return _ibuf.in_avail();
        }
        virtual size_t get(char* buf) {
            return _ibuf.sgetc(buf);
        }
        virtual size_t get(char* buf, size_t buflen) {
            return _ibuf.sgetn(buf, buflen);
        }

        virtual istream& sync() = 0;
    protected:
        streambuf _ibuf;
    public:
        streamerr _ierror;
    };

    class ostream : public stream_base {
    public:
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
        virtual ostream& put(char c) {
            if (_obuf.sputc(c)) {
                return *this;
            } else {
                _oerror._flags.overflow = true;
                _oerror |= _obuf._error;
                return *this;
            }
        }
        virtual ostream& write(const char* s, size_t n) {
            if (n == _obuf.sputn(s, n)) {
                return *this;
            } else {
                _oerror._flags.overflow = true;
                _oerror |= _obuf._error;
                return *this;
            }
        }
        virtual ostream& flush() = 0;
    protected:
        streambuf _obuf;
    public:
        streamerr _oerror;
    };

    class iostream : public istream, public ostream {};
};