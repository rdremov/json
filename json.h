#pragma once

#include <cstdlib>
#include <cstring>

#include <vector>
#include <string>

namespace rvd {

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
        const char* start;
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
            NumberInvalid,
            NotUnique,
        } type{Success};
        const char* location{};
        size_t offset() const {return location - start;}
    };

protected:
    struct Cntx {
        const char* ptr;
        Error& err;

        void skipSpaces() {
            while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')
                ptr++;
        }

        bool keyword(const char* str) {
            auto n = strlen(str);
            if (strncmp(str, ptr, n))
                return false;
            ptr += n;
            return true;
        }

        bool check(Error::Type errType) {
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
            err.location = ptr;
            return false;
        }

        const char* parseString() {
            skipSpaces();
            if (!check(Error::StringBegin))
                return nullptr;
            auto p0 = ptr;
            bool protect = false;
            for (;;) {
                if (!*ptr)
                    break;
                if (!protect) {
                    if (*ptr == '"')
                        break;
                    if (*ptr == '\\')
                        protect = true;
                } else
                    protect = false;
                ptr++;
            }
            if (!*ptr) {
                check(Error::StringEnd);
                return nullptr;
            }
            return p0;
        }
    };

public:
    struct Array;
    struct Object;

    class Value {
        Type type;
        union Data {
            double d{};
            char* s;
            Array* a;
            Object* o;
        } data;
    public:
        Value() : type(Type::Null) {}
        Value(Value& v) : type(v.type) {v.type = Type::Null; memcpy(&data, &v.data, sizeof(data));}
        Value(double d) : type(Type::Number) {data.d = d;}
        Value(const char* s) : type(Type::String) {data.s = strdup(s);}
        Value(Array* a) : type(Type::Array) {data.a = a;}
        Value(Object* o) : type(Type::Object) {data.o = o;}
        ~Value() {
            switch (type) {
            case Type::String: free(data.s); break;
            case Type::Array: delete data.a; break;
            case Type::Object: delete data.o; break;
            }
        }
        bool operator==(const Value& v) const {
            if (type != v.type)
                return false;
            switch (type) {
            case Type::Number: return data.d == v.data.d;
            case Type::String: return !strcmp(data.s, v.data.s);
            case Type::Array: return *data.a == *v.data.a;
            case Type::Object: return *data.o == *v.data.o;
            }
            return true;
        }
        void parse(Cntx& cntx);
    };

    template<class T>
    struct Container : std::vector<T> {
        bool unique() const;
        void parse(Cntx& cntx, char charEnd, Error::Type errorEnd);
    };

    struct Array {
        Container<Value> elements;
        void parse(Cntx& cntx) {elements.parse(cntx, ']',Error::ArrayEnd);}
        void append(Value& v) {elements.emplace_back(v);}
        bool operator==(const Array& a) const {return elements == a.elements;}
    };

    struct Object {
        struct Pair {
            std::string name;
            Value value;
            Pair() {}
            Pair(const char* name, Value& v) : name(name), value(v) {}
            bool operator==(const Pair& p) const {return name == p.name && value == p.value;}
            void parse(Cntx& cntx);
        };
        Container<Pair> members;
        void parse(Cntx& cntx) {members.parse(cntx, '}',Error::ObjectEnd);}
        void append(const char* name, Value& v) {members.emplace_back(name, v);}
        bool operator==(const Object& o) const {return members == o.members;}
    };

    JSON(const char* str) : err{str} {
        Cntx cntx{str, err};
        if (cntx.check(Error::ObjectBegin)) {
            object = new Object;
            object->parse(cntx);
        }
    }
    ~JSON() {delete object;}
    Error::Type getError(size_t& offset) const {
        offset = err.offset();
        return err.type;
    }
    Object* detach() {
        auto o = object;
        object = nullptr;
        return o;
    }
    
private:
    Error err;
    Object* object{};
};

bool JSON::Container<JSON::Value>::unique() const {
    return true;
}

bool JSON::Container<JSON::Object::Pair>::unique() const {
    size_t n = size();
    if (n > 1) {
        for (size_t i=0; i<n-1; i++) {
            if (back().name == at(i).name)
                return false;
        }
    }
    return true;
}

template<class T>
void JSON::Container<T>::parse(Cntx& cntx, char charEnd, Error::Type errorEnd) {
    bool separator = true;
    cntx.skipSpaces();
    while (*cntx.ptr) {
        if (*cntx.ptr == charEnd) {
            cntx.ptr++;
            return;
        }
        if (!separator) {
            cntx.check(Error::Comma);
            return;
        }
        push_back(T());
        back().parse(cntx);
        if (!cntx.check(Error::Success))
            return;
        if (!unique()) {
            cntx.check(Error::NotUnique);
            return;
        }
        cntx.skipSpaces();
        if (*cntx.ptr == ',') {
            separator = true;
            cntx.ptr++;
        } else
            separator = false;
    }
    cntx.check(errorEnd);
}

void JSON::Value::parse(Cntx& cntx) {
    cntx.skipSpaces();
    if ('[' == *cntx.ptr) {
        cntx.ptr++;
        type = Type::Array;
        data.a = new Array;
        data.a->parse(cntx);
    } else if ('{' == *cntx.ptr) {
        cntx.ptr++;
        type = Type::Object;
        data.o = new Object;
        data.o->parse(cntx);
    } else if ('"' == *cntx.ptr) {
        type = Type::String;
        auto b = cntx.parseString();
        if (b) {
            auto len = cntx.ptr - b;
            data.s = (char*) malloc(len + 1);
            strncpy(data.s, b, len);
            data.s[len] = 0;
            cntx.ptr++;
        }
    } else if (cntx.keyword("true")) {
        type = Type::True;
    } else if (cntx.keyword("false")) {
        type = Type::False;
    } else if (cntx.keyword("null")) {
        type = Type::Null;
    } else {
        type = Type::Number;
        char* e;
        data.d = strtod(cntx.ptr, &e);
        if (cntx.ptr == e)
            cntx.check(Error::NumberInvalid);
        else
            cntx.ptr = e;
    }
}

void JSON::Object::Pair::parse(Cntx& cntx) {
    auto b = cntx.parseString();
    if (b) {
        name.assign(b, cntx.ptr++ - b);
        if (cntx.check(Error::Colon))
            value.parse(cntx);
    }
}

} // namespace rvd
