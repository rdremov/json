#define _CRT_SECURE_NO_WARNINGS

#include "json.h"
#include <cstdio>
#include <memory>

using namespace rvd;

class JSONTest {
	int m_index{0};
	#define JSONTest_VERIFY(expr) verify(expr, #expr)

	void verify(bool success, const char* desc) {
		if (success)
			printf("%d. SUCCESS\n", m_index);
		else
			printf("%d. FAILURE: %s\n", m_index, desc);
	}

    std::unique_ptr<JSON::Object> preparePositive(const char* str) {
        m_index++;
        JSON json(str);
        size_t offsetTest = 0;
        auto errorTest = json.getError(offsetTest);
        JSONTest_VERIFY(errorTest == JSON::Error::Success);
        return std::unique_ptr<JSON::Object>(json.detach());
    };

public:
    void negative(const char* str, JSON::Error::Type error, size_t offset) {
        m_index++;
        JSON json(str);
        size_t offsetTest = 0;
        auto errorTest = json.getError(offsetTest);
        JSONTest_VERIFY(error == errorTest && offset == offsetTest);
    }

	void positive() {
		{
			auto o1 = preparePositive(R"({"num" : -1.23e-4, "name" : "string with \"quoted\" text"})");
			JSON::Object o2;
			o2.append("num", JSON::Value(-1.23e-4));
			o2.append("name", JSON::Value("string with \\\"quoted\\\" text"));
			JSONTest_VERIFY(*o1 == o2);
		}
		{
            auto o1 = preparePositive(R"({"a" : [1, 2]})");
            JSON::Object o2;
			JSON::Array* a = new JSON::Array;
			a->append(JSON::Value(1));
			a->append(JSON::Value(2));
            o2.append("a", JSON::Value(a));
            JSONTest_VERIFY(*o1 == o2);
		}
		{
            auto o1 = preparePositive(R"({"o" : {"x" : 1, "y" : 2}})");
			auto o = new JSON::Object;
			o->append("x", JSON::Value(1));
            o->append("y", JSON::Value(2));
            JSON::Object o2;
			o2.append("o", JSON::Value(o));
            JSONTest_VERIFY(*o1 == o2);
		}
	}
};

int main(int argc, char* argv[]) {
	JSONTest test;
	test.negative("", JSON::Error::ObjectBegin, 0);
	test.negative("{", JSON::Error::ObjectEnd, 1);
	test.negative("{1}", JSON::Error::StringBegin, 1);
	test.negative(R"({"})", JSON::Error::StringEnd, 3);
	test.negative(R"({""})", JSON::Error::Colon, 3);
	test.negative(R"({"" :})", JSON::Error::NumberInvalid, 5);
	test.negative(R"({"" : "})", JSON::Error::StringEnd, 8);
	test.negative(R"({"" : -})", JSON::Error::NumberInvalid, 6);
	test.negative(R"({"" : .})", JSON::Error::NumberInvalid, 6);
    test.negative(R"({"a" : 1, "a" : 2})", JSON::Error::NotUnique, 17);
	test.negative(R"({"a" : 1 "b" : 2})", JSON::Error::Comma, 9);
    test.positive();
	return 0;
}
