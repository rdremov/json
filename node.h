/* Roman Dremov 2022 basic C++ json memory model for testing v1 */

#pragma once

#include <string>
#include <vector>
#include <utility>

namespace json {

struct Node {
	virtual ~Node() {}
	virtual bool operator==(const Node*) const = 0;
};

struct Number : Node {
	double d;
	Number(double d) : d(d) {}
	bool operator==(const Node* node) const override {
		auto p = dynamic_cast<const Number*>(node);
		return p && d == p->d;
	}
};

struct String : Node {
	std::string s;
	String(const char* s) : s(s) {}
	String(const char* s, size_t len) : s(s, len) {}
	bool operator==(const Node* node) const override {
		auto p = dynamic_cast<const String*>(node);
		return p && s == p->s;
	}
};

struct Null : Node {
	bool operator==(const Node* node) const override {
		return dynamic_cast<const Null*>(node);
	}
};

struct True : Node {
	bool operator==(const Node* node) const override {
		return dynamic_cast<const True*>(node);
	}
};

struct False : Node {
	bool operator==(const Node* node) const override {
		return dynamic_cast<const False*>(node);
	}
};

template<class T>
bool equal(const std::vector<T>& v1, const std::vector<T>& v2) {
	if (v1.size() != v2.size())
		return false;

	return true;
}

struct Array : Node {
	std::vector<Node*> elements;
	Array(std::initializer_list<Node*> il) : elements(il) {}
	~Array() {
		for (auto& node : elements)
			delete node;
	}
	bool operator==(const Node* node) const override {
		auto p = dynamic_cast<const Array*>(node);
		if (!p || elements.size() != p->elements.size())
			return false;
		for (size_t i=0; i<elements.size(); i++) {
			if (!elements[i]->operator==(p->elements[i]))
				return false;
		}
		return true;
	}
};

struct Member : std::pair<std::string, Node*> {
	Member(std::string&& str, Node* node) : std::pair<std::string, Node*>(str, node) {}
	bool operator==(const Member& m) const {
		return first == m.first && second->operator==(m.second);
	}
};

struct Object : Node {
	std::vector<Member> members;
	Object(std::initializer_list<Member> il) : members(il) {}
	~Object() {
		for (auto& [_, node] : members)
			delete node;
	}
	bool operator==(const Node* node) const override {
		auto p = dynamic_cast<const Object*>(node);
		return p && members == p->members;
	}
};

} // namespace json