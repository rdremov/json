#define _CRT_SECURE_NO_WARNINGS

#include "json.h"
#include "node.h"
#include <cstdio>
#include <memory>
#include <set>
#include <stack>

using namespace json;

struct NegativeBuilder : Builder {
	std::stack<std::set<std::string>> stack;
    bool beginObject() override {
		stack.push(std::set<std::string>());
		return true;
	}
    void endObject() override {
		stack.pop();
	}
    bool beginArray() override {return true;}
    void endArray() override {}
    bool setName(const char* s, size_t len) override {
		auto [_, inserted] = stack.top().insert(std::string(s, len));
		return inserted;
	}
    bool setValue() override {return true;}
    bool setValue(bool) override {return true;}
    bool setValue(double) override {return true;}
    bool setValue(const char*, size_t) override {return true;}
};

struct PositiveBuilder : Builder {
	std::stack<Node*> stack;
	std::string name;
	Object* object{};

	~PositiveBuilder() {delete object;}
    bool beginObject() override {
		auto o = new Object{};
		if (stack.empty())
			object = o;
		else
			set(o);
		stack.push(o);
		return true;
	}
    void endObject() override {stack.pop();}
    bool beginArray() override {
		auto a = new Array{};
		set(a);
		stack.push(a);
		return true;
	}
    void endArray() override {stack.pop();}
    bool setName(const char* s, size_t len) override {name.assign(s, len); return true;}
    bool setValue() override {return set(new Null);}
    bool setValue(bool b) override {return b ? set(new True) : set(new False);}
	bool setValue(double d) override {return set(new Number{d});}
	bool setValue(const char* s, size_t len) override {return set(new String{s, len});}

	bool set(Node* node) {
		auto parent = stack.top();
		if (auto o = dynamic_cast<Object*>(stack.top()))
			o->members.push_back(Member(std::move(name), node));
		else if (auto a = dynamic_cast<Array*>(stack.top()))
			a->elements.push_back(node);
		else
			return false;
		return true;
	}
};

class JSONTest {
	int m_index{0};
	#define JSONTest_VERIFY(expr) verify(expr, #expr)

	void verify(bool success, const char* desc) {
		if (success)
			printf("%d. SUCCESS\n", m_index);
		else
			printf("%d. FAILURE: %s\n", m_index, desc);
	}

    void doPositive(const char* str, Object* o) {
        m_index++;
		PositiveBuilder b;
        Parser parser(str, b);
        size_t offsetTest = 0;
        auto errorTest = parser.getError(offsetTest);
        JSONTest_VERIFY(errorTest == Error::Success && *o == b.object);
		delete o;
    };

public:
    void negative(const char* str, Error error, size_t offset) {
        m_index++;
		NegativeBuilder b;
        Parser parser(str, b);
        size_t offsetTest = 0;
        auto errorTest = parser.getError(offsetTest);
        JSONTest_VERIFY(error == errorTest && offset == offsetTest);
    }

	void positive() {
		{
			auto o = new Object{
				Member{"num", new Number{-1.23e-4}},
				Member{"name", new String{"string with \\\"quoted\\\" text"}}
				};
			doPositive(R"({"num" : -1.23e-4, "name" : "string with \"quoted\" text"})", o);
		}
		{
			auto o = new Object{Member{"a", new Array{new Number{1}, new Number{2}}}};
			doPositive(R"({"a" : [1, 2]})", o);
		}
		{
			auto o = new Object{Member{"o", new Object{
				Member{"x", new Number{1}},
				Member{"y", new Number{2}}
			}}};
			doPositive(R"({"o" : {"x" : 1, "y" : 2}})", o);
		}
	}
};

int main(int argc, char* argv[]) {
	JSONTest test;
    test.positive();
	test.negative("", Error::Object, 0);
	test.negative("{", Error::Name, 1);
	test.negative("{1}", Error::Name, 1);
	test.negative(R"({"})", Error::Name, 3);
	test.negative(R"({""})", Error::Colon, 3);
	test.negative(R"({"" :})", Error::Number, 5);
	test.negative(R"({"" : "})", Error::String, 8);
	test.negative(R"({"" : -})", Error::Number, 6);
	test.negative(R"({"" : .})", Error::Number, 6);
    test.negative(R"({"a" : 1, "a" : 2})", Error::Name, 12);
	test.negative(R"({"a" : 1 "b" : 2})", Error::Comma, 9);
	return 0;
}
