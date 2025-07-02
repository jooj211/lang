#ifndef NODE_HPP
#define NODE_HPP
class Visitor;
class Node {
public:
    virtual ~Node() = default;
    virtual void accept(Visitor* v) = 0;
};
#endif