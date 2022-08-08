#include "json.h"
#include "cassert"

using namespace rvd;

static void test_error(const char* str, JSON::Error::Type error, size_t offset) {
	JSON json(str);
	size_t offsetTest = 0;
	auto errorTest = json.getError(offsetTest);
	assert(error == errorTest);
	assert(offset == offsetTest);
}

static void test_success(const char* str) {
	JSON json(str);
	size_t offsetTest = 0;
	auto errorTest = json.getError(offsetTest);
	assert(errorTest == JSON::Error::Success);
}

int main(int argc, char* argv[]) {
	// positive tests
	test_success(R"({"num" : -1.23e-4, "name" : "string with \"quoted\" text"})");

	// negative tests
	test_error("", JSON::Error::ObjectBegin, 0);
	test_error("{", JSON::Error::ObjectEnd, 1);
	test_error("{1}", JSON::Error::StringBegin, 1);
	test_error(R"({"})", JSON::Error::StringEnd, 3);
	test_error(R"({""})", JSON::Error::Colon, 3);
	test_error(R"({"" :})", JSON::Error::NumberSignificand, 5);
	test_error(R"({"" : "})", JSON::Error::StringEnd, 8);
	test_error(R"({"" : -})", JSON::Error::NumberSignificand, 7);
	test_error(R"({"" : .})", JSON::Error::NumberDecimal, 7);

	return 0;
}
