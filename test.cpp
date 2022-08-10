#define _CRT_SECURE_NO_WARNINGS

#include "json.h"
#include <cstdio>
#include <memory>
#include <set>
#include <stack>

struct NegativeBuilder : json::Builder {
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
    bool setName(const char* name, size_t len) override {
		auto [_, inserted] = stack.top().insert(std::string(name, len));
		return inserted;
	}
    bool setValue() override {return true;}
    bool setValue(bool b) override {return true;}
    bool setValue(double d) override {return true;}
    bool setValue(const char* str, size_t len) override {return true;}
};
/*
struct Node {
	virtual ~Node() {}
};
struct Number : Node {
	double d;
	Number(double d) : d(d) {}
};
struct String : Node {
	std::string s;
	String(const char* s) : s(s) {}
};
struct Null : Node {};
struct True : Node {};
struct False : Node {};
struct Array : Node {
	std::vector<Node*> elements;
	Array() {}
	template <typename T, typename... Types>
	Array(Node* node, Types... ee) {
		elements.emplace_back(node);
		Array(ee...);
	}
	~Array() {
		for (auto& node : elements)
			delete node;
	}
};
struct Object : Node {
	using Pair = std::pair<std::string, Node*>;
	std::vector<Pair> members;
	Object() {}
	template <typename... Types>
	Object(const char* name, Node* node, Types... tt) {
		members.emplace_back(Pair(name, node));
		Object(tt...);
	}
	~Object() {
		for (auto& [_, node] : members)
			delete node;
	}
};
*/
struct PositiveBuilder : json::Builder {
    bool beginObject() override {
		return true;
	}
    void endObject() override {
	}
    bool beginArray() override {return true;}
    void endArray() override {}
    bool setName(const char* name, size_t len) override {
		return true;
	}
    bool setValue() override {return true;}
    bool setValue(bool b) override {return true;}
    bool setValue(double d) override {return true;}
    bool setValue(const char* str, size_t len) override {return true;}
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

    void doPositive(const char* str, PositiveBuilder& b) {
        m_index++;
        json::Parser parser(str, b);
        size_t offsetTest = 0;
        auto errorTest = parser.getError(offsetTest);
        JSONTest_VERIFY(errorTest == json::Error::Success);
    };

public:
    void negative(const char* str, json::Error error, size_t offset) {
        m_index++;
		NegativeBuilder b;
        json::Parser parser(str, b);
        size_t offsetTest = 0;
        auto errorTest = parser.getError(offsetTest);
        JSONTest_VERIFY(error == errorTest && offset == offsetTest);
    }

	void positive() {
		//auto o = new Object("a", new String("val"), "b", new Number(1.23));
		/*{
			PositiveBuilder v;
			auto o = new PositiveVisitor::Object;
			o->add("num", new PositiveVisitor::Number(-1.23e-4));
			o->add("name", new PositiveVisitor::String("string with \\\"quoted\\\" text"));
			v.root = o;
			doPositive(R"({"num" : -1.23e-4, "name" : "string with \"quoted\" text"})", v);
		}
		{
            auto o1 = preparePositive(R"({"a" : [1, 2]})");
            rvd::JSON::Object o2;
			rvd::JSON::Array* a = new rvd::JSON::Array;
			a->append(rvd::JSON::Value(1));
			a->append(rvd::JSON::Value(2));
            o2.append("a", rvd::JSON::Value(a));
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
		}*/
	}
};

int main(int argc, char* argv[]) {
	JSONTest test;
	test.negative("", json::Error::Object, 0);
	test.negative("{", json::Error::Name, 1);
	test.negative("{1}", json::Error::Name, 1);
	test.negative(R"({"})", json::Error::Name, 3);
	test.negative(R"({""})", json::Error::Colon, 3);
	test.negative(R"({"" :})", json::Error::Number, 5);
	test.negative(R"({"" : "})", json::Error::String, 8);
	test.negative(R"({"" : -})", json::Error::Number, 6);
	test.negative(R"({"" : .})", json::Error::Number, 6);
    test.negative(R"({"a" : 1, "a" : 2})", json::Error::Name, 12);
	test.negative(R"({"a" : 1 "b" : 2})", json::Error::Comma, 9);
    test.positive();
	return 0;
}
