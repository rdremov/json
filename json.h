﻿#pragma once

#include <cstdlib>

namespace json {

enum class Error {
    Success,
    Null,
    False,
    True,
    Number,
    String,
    Array,
    Object,
    Name,
    Colon,
    Comma
};

struct Builder {
    virtual ~Builder() {}
    virtual bool beginObject() = 0;
    virtual void endObject() = 0;
    virtual bool beginArray() = 0;
    virtual void endArray() = 0;
    virtual bool setName(const char*, size_t) = 0;
    virtual bool setValue() = 0;
    virtual bool setValue(bool) = 0;
    virtual bool setValue(double) = 0;
    virtual bool setValue(const char*, size_t) = 0;
};

class Parser {
    Error errType{Error::Success};
    size_t errOffset{};
    const char* ptr0;
    const char* ptr;
    Builder& build;

public:
    Parser(const char* s, Builder& b) : ptr0(s), ptr(s), build(b) {
        parseObject();
    }
    Error getError(size_t& offset) const {
        offset = errOffset;
        return errType;
    }

private:
    void skipSpaces() {
        while (' ' == *ptr || '\t' == *ptr || '\n' == *ptr || '\r' == *ptr)
            ptr++;
    }
    bool keyword(const char* str) {
		auto p = ptr;
		while (*str) {
            if (*p++ != *str++)
                return false;
        }
		ptr = p;
		return true;
    }
    void error(Error type) {
        errType = type;
        errOffset = ptr - ptr0;
    }
    const char* parseString() {
        skipSpaces();
        if ('"' != *ptr)
            return nullptr;
        auto p0 = ++ptr;
        bool normal = true;
        while (*ptr) {
            if (normal) {
                if ('"' == *ptr)
                    return p0;
                if ('\\' == *ptr)
                    normal = false;
            } else
                normal = true;
			ptr++;
        }
        return nullptr;
    }
    void parseObject() {
        skipSpaces();
        if ('{' != *ptr || !build.beginObject()) {
            error(Error::Object);
            return;
        }
        ptr++;
        skipSpaces();
        if ('}' == *ptr) {
            ptr++;
            build.endObject();
            return;
        }
        for (;;) {
            auto name = parseString();
            if (!name || !build.setName(name, ptr - name)) {
                error(Error::Name);
                return;
            }
            ptr++;
            skipSpaces();
            if (':' != *ptr) {
                error(Error::Colon);
                return;
            }
            ptr++;
            parseValue();
            if (errType != Error::Success)
                return;
            skipSpaces();
            if ('}' == *ptr) {
                ptr++;
                build.endObject();
                return;
            }
            if (',' != *ptr) {
                error(Error::Comma);
                return;
            }
            ptr++;
        }
    }
    void parseArray() {
        skipSpaces();
        if ('[' != *ptr || !build.beginArray()) {
            error(Error::Array);
            return;
        }
        ptr++;
        skipSpaces();
        if (']' == *ptr) {
            ptr++;
            build.endArray();
            return;
        }
        for (;;) {
            parseValue();
            if (errType != Error::Success)
                return;
            skipSpaces();
            if (']' == *ptr) {
                ptr++;
                build.endArray();
                return;
            }
            if (',' != *ptr) {
                error(Error::Comma);
                return;
            }
            ptr++;
        }
    }
    void parseValue() {
        skipSpaces();
        if ('[' == *ptr)
            parseArray();
        else if ('{' == *ptr)
            parseObject();
        else if ('"' == *ptr) {
            auto str = parseString();
            if (!str || !build.setValue(str, ptr - str)) {
                error(Error::String);
                return;
            }
            ptr++;
        } else if (keyword("true")) {
            if (!build.setValue(true))
                error(Error::True);
        } else if (keyword("false")) {
            if (!build.setValue(false))
                error(Error::False);
        } else if (keyword("null")) {
            if (!build.setValue())
                error(Error::Null);
        } else {
            char* end;
            auto d = strtod(ptr, &end);
            if (ptr == end || !build.setValue(d))
                error(Error::Number);
            ptr = end;
        }
    }
};
} // namespace json
