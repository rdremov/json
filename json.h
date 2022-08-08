#pragma once

#include <cstring>

#include <vector>
#include <functional>

namespace rvd {

const char* skip_spaces(const char* str) {
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')
        str++;
    return str;
}

const char* string_end(const char* str) {
    bool protect = false;
    for (;;) {
        if (!*str)
            break;
        if (!protect) {
            if (*str == '"')
                break;
            if (*str == '\\')
                protect = true;
        } else
            protect = false;
        str++;
    }
    return str;
}

class JSON {
public:
    enum class Type {
        Null,
        False,
        True,
        Number,
        String,
        Array,
        Object
    };

    struct Error {
        enum Type {
            Success,
            ObjectBegin,
            ObjectEnd,
            ArrayBegin,
            ArrayEnd,
            StringBegin,
            StringEnd,
            Colon,
            Comma,
            NumberSignificand,
            NumberExp,
            NumberSign,
            NumberDecimal,
            NameNotUnique,
        } type{Success};
        size_t offset{};
    };

protected:
    struct Pool;
    using Index = size_t;

    struct Cntx {
        const char* ptr;
        const char* ptr0;
        Error& err;
        Pool& pool;

        void skipSpaces() {
            ptr = skip_spaces(ptr);
        }

        bool test(const char* str) {
            auto n = strlen(str);
            if (strncmp(str, ptr, n))
                return false;
            ptr += n;
            return true;
        }

        bool flag(Error::Type errType) {
            if (err.type != Error::Success)
                return false;
            skipSpaces();
            switch (errType) {
            case Error::Success: return true;
            case Error::ObjectBegin: if (*ptr == '{') {ptr++; return true;} break;
            case Error::ObjectEnd: if (*ptr == '}') {ptr++; return true;} break;
            case Error::ArrayBegin: if (*ptr == '[') {ptr++; return true;} break;
            case Error::ArrayEnd: if (*ptr == ']') {ptr++; return true;} break;
            case Error::StringBegin: case Error::StringEnd: if (*ptr == '"') { ptr++; return true;} break;
            case Error::Colon: if (*ptr == ':') {ptr++; return true;} break;
            }
            err.type = errType;
            err.offset = ptr - ptr0;
            return false;
        }

        Index parseString() {
            skipSpaces();
            if (!flag(Error::StringBegin))
                return 0;
            auto p0 = ptr;
            ptr = string_end(p0);
            if (!*ptr) {
                flag(Error::StringEnd);
                return 0;
            }
            auto len = ptr - p0;
            auto index = pool.newStr(len);
            auto p = &pool.strs[index];
            strncpy(p, p0, len);
            p[len] = 0;
            ptr++;
            return index;
        }

        const char* parseDigits(char first) {
            const char* b = nullptr;
            if ( *ptr >= first && *ptr <= '9') {
                b = ptr++;
                while (*ptr >= '0' && *ptr <= '9')
                    ptr++;
            }
            return b;
        }

        double parseInt(const char* begin, const char* end) {
            double d = 0;
            while (begin != end) {
                d *= 10;
                d += *begin - '0';
                begin++;
            }
            return d;
        }

        double parseNumber() {
            skipSpaces();
            bool neg = false;
            if ('-' == *ptr ) {
                neg = true;
                ptr++;
            }
            bool leadingZero = false;
            const char* bInt = nullptr;
            const char* eInt = nullptr;
            if ('0' == *ptr) {
                leadingZero = true;
                ptr++;
            } else {
                bInt = parseDigits('1');
                eInt = ptr;
            }
            bool decimal = false;
            if ('.' == *ptr) {
                decimal = true;
                ptr++;
            }
            const char* bFrac = nullptr;
            const char* eFrac = nullptr;
            if (decimal) {
                bFrac = parseDigits('0');
                eFrac = ptr;
                if (!(leadingZero || bInt) || !bFrac) {
                    flag(Error::NumberDecimal);
                    return 0;
                }
            }
            if (!(leadingZero || bInt || bFrac)) {
                flag(Error::NumberSignificand);
                return 0;
            }
            bool exp = false;
            if ('e' == *ptr || 'E' == *ptr) {
                exp = true;
                ptr++;
            }
            bool negExp = false;
            const char* bExp = nullptr;
            const char* eExp = nullptr;
            if (exp) {
                if ('-' == *ptr || '+' == *ptr) {
                    negExp = ('-' == *ptr);
                    ptr++;
                }
                bExp = parseDigits('1');
                eExp = ptr;
                if (!bExp) {
                    flag(Error::NumberExp);
                    return 0;
                }
            }
            if (!bFrac) {
                if (leadingZero && neg) {
                    flag(Error::NumberSign);
                    return 0;
                }
                if (decimal) {
                    flag(Error::NumberDecimal);
                    return 0;
                }
            }
            double dInt = 0;
            if (bInt)
                dInt = parseInt(bInt, eInt);
            double dFrac = 0;
            if (bFrac)
                dFrac = parseInt(bFrac, eFrac);
            double dExp = 0;
            if (bExp) {
                dExp = parseInt(bExp, eExp);
                if (negExp)
                    dExp = -dExp;
            }
            if (bFrac) {
                auto power = eFrac - bFrac;
                while (power--)
                    dFrac /= 10;
            }
            double d = dInt + dFrac;
            auto power = (int)dExp;
            if (power > 0) {
                while (power--)
                    d *= 10;
            } else {
                while (power++)
                    d /= 10;
            }
            if (neg)
                d = -d;
            return d;
        }
    };

    union Data{
        double d{};
        Index s;
        Index a;
        Index o;
    };

    struct Value {
        Type type{Type::Null};
        Data data;
        void parse(Cntx& cntx) {
            cntx.skipSpaces();
            if ('[' == *cntx.ptr) {
                type = Type::Array;
                data.a = cntx.pool.newArray();
                cntx.pool.arrs[data.a].parse(cntx);

            } else if ('{' == *cntx.ptr) {
                type = Type::Object;
                data.o = cntx.pool.newObject();
                cntx.pool.objs[data.o].parse(cntx);
            } else if ('"' == *cntx.ptr) {
                type = Type::String;
                data.s = cntx.parseString();
            } else if (cntx.test("true")) {
                type = Type::True;
            } else if (cntx.test("false")) {
                type = Type::False;
            } else if (cntx.test("null")) {
                type = Type::Null;
            } else {
                type = Type::Number;
                data.d = cntx.parseNumber();
            }
        }
    };

    template<class T, char CHAREND, Error::Type ERREND>
    struct Container : std::vector<T> {
        void parse(Cntx& cntx) {
            bool separator = true;
            cntx.skipSpaces();
            for (;;) {
                if (*cntx.ptr == CHAREND || *cntx.ptr == 0 || !separator)
                    break;
                push_back(T());
                back().parse(cntx, *this);
                if (!cntx.flag(Error::Success))
                    return;
                cntx.skipSpaces();
                if (*cntx.ptr == ',') {
                    separator = true;
                    cntx.ptr++;
                } else
                    separator = false;
            }
            cntx.flag(ERREND);
        }
    };

    struct Array {
        struct Element {
            Value value;
            void parse(Cntx& cntx, Container<Element,']',Error::ArrayEnd>& cont) {
                value.parse(cntx);
            }
        };
        Container<Element,']',Error::ArrayEnd> elements;
        void parse(Cntx& cntx) {elements.parse(cntx);}
    };

    struct Object {
        struct Pair {
            Index name{};
            Value value;
            void parse(Cntx& cntx, Container<Pair,'}',Error::ObjectEnd>& cont) {
                auto index = cntx.parseString();
                if (!cntx.flag(Error::Success))
                    return;
                auto size = cont.size();
                if (size > 0) {
                    for (size_t i=0; i<size-1; i++) {
                        if (!strcmp(&cntx.pool.strs[index], &cntx.pool.strs[cont[i].name])) {
                            cntx.flag(Error::NameNotUnique);
                            return;
                        }
                    }
                }
                name = index;
                if (cntx.flag(Error::Colon))
                    value.parse(cntx);
            }
        };
        Container<Pair,'}',Error::ObjectEnd> members;
        void parse(Cntx& cntx) {members.parse(cntx);}
    };

    struct Pool {
        std::vector<char> strs;
        std::vector<Object> objs;
        std::vector<Array> arrs;
        Index newStr(size_t size) {
            auto oldSize = strs.size();
            strs.resize(oldSize + size + 1);
            return oldSize;
        }
        Index newObject() {
            auto oldSize = objs.size();
            objs.resize(oldSize + 1);
            return oldSize;
        }
        Index newArray() {
            auto oldSize = arrs.size();
            arrs.resize(oldSize + 1);
            return oldSize;
        }
        Pool() {
            strs.reserve(1024);
            objs.reserve(128);
            arrs.reserve(128);
        }
    };

    void parse(Cntx& cntx) {
        if (cntx.flag(Error::ObjectBegin)) {
            object = cntx.pool.newObject();
            cntx.pool.objs[object].parse(cntx);
        }
    }

public:
    JSON(const char* str) {
        Cntx cntx{str, str, err, pool};
        parse(cntx);
    }

    Error::Type getError(size_t& offset) const {
        offset = err.offset;
        return err.type;
    }
    
private:
    Error err;
    Pool pool;
    Index object{};
};

} // namespace rvd
